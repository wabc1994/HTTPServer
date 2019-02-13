#include "threadpool.h"


// 创建线程池，有thred_count个线程池，容纳quque_size个的任务队列，flags 参数没有使用
threadpool_t *threadpool_create(int thread_count, int queue_size, int flags)
{
    threadpool_t *pool;
    int i;
    //(void) flags;
    do
    {
        if(thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE) {
            return NULL;
        }
    
        if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) 
        {
            break;
        }
    
        /* Initialize */
        pool->thread_count = 0;
        pool->queue_size = queue_size;
        pool->head = pool->tail = pool->count = 0;
        pool->shutdown = pool->started = 0;
    
        /* Allocate thread and task queue
         * 申请线程数组和任务队列所需的内存，后续可以*/
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
        //
        pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);
    
        /* Initialize mutex and conditional variable first */
        if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
           (pthread_cond_init(&(pool->notify), NULL) != 0) ||
           (pool->threads == NULL) ||
           (pool->queue == NULL)) 
        {
            break;
        }
    
        /* Start worker threads */
        for(i = 0; i < thread_count; i++) {
            if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0) 
            {
                threadpool_destroy(pool, 0);
                return NULL;
            }
            pool->thread_count++;
            pool->started++;
        }
    
        return pool;
    } while(false);
    
    if (pool != NULL) 
    {
        threadpool_free(pool);
    }
    return NULL;
}


// 将任务添加到线程池，
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags)
{
    //printf("add to thread pool !\n");
    int err = 0;
    int next;
    //(void) flags;
    if(pool == NULL || function == NULL)
    {
        return THREADPOOL_INVALID;
    }
    if(pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return THREADPOOL_LOCK_FAILURE;
    }
    next = (pool->tail + 1) % pool->queue_size;
    do 
    {
        /* Are we full ? */
        if(pool->count == pool->queue_size) {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        /* Are we shutting down ? */
        if(pool->shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        /* Add task to queue 每一个任务都有一个函数，和执行的参数*/
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;
        
        /* pthread_cond_broadcast */
        if(pthread_cond_signal(&(pool->notify)) != 0) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while(false);

    if(pthread_mutex_unlock(&pool->lock) != 0) {
        err = THREADPOOL_LOCK_FAILURE;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    // 在主进程当中销毁工作线程的前提，是让他们执行完毕，join()
    printf("Thread pool destroy !\n");
    int i, err = 0;

    if(pool == NULL)
    {
        return THREADPOOL_INVALID;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) 
    {
        return THREADPOOL_LOCK_FAILURE;
    }

    do 
    {
        /* Already shutting down */
        if(pool->shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }

        pool->shutdown = (flags & THREADPOOL_GRACEFUL) ?
            graceful_shutdown : immediate_shutdown;

        /* Wake up all worker threads */
        if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        /* Join all worker thread
         * 主线程等待其他执行任务当中的线程执行完
         * */
        for(i = 0; i < pool->thread_count; ++i)
        {
            if(pthread_join(pool->threads[i], NULL) != 0)
            {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while(false);

    /* Only if everything went well do we deallocate the pool */
    if(!err) 
    {
        // 还是调用free 函数来做
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0)
    {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) 
    {
        free(pool->threads);
        free(pool->queue);
 
        /* Because we allocate pool->threads after initializing the
           mutex and condition variable, we're sure they're
           initialized. Let's lock the mutex just in case. */
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);    
    return 0;
}



// 在这里面如何具体如何执行我们的I/O任务情况

static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    // for 自旋锁，主要的作用
    for(;;)
    {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while((pool->count == 0) && (!pool->shutdown)) 
        {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if((pool->shutdown == immediate_shutdown) ||
           ((pool->shutdown == graceful_shutdown) &&
            (pool->count == 0)))
        {
            break;
        }

        /* Grab our task */
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        /* Unlock */
        pthread_mutex_unlock(&(pool->lock));

        /* Get to work */
        (*(task.function))(task.argument);
    }

    --pool->started;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}