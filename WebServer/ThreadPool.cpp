

// This file has not been used
#include "ThreadPool.h"

pthread_mutex_t ThreadPool::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::notify = PTHREAD_COND_INITIALIZER;
std::vector<pthread_t> ThreadPool::threads;
std::vector<ThreadPoolTask> ThreadPool::queue;
int ThreadPool::thread_count = 0;
int ThreadPool::queue_size = 0;
int ThreadPool::head = 0;
int ThreadPool::tail = 0;
int ThreadPool::count = 0;
int ThreadPool::shutdown = 0;
int ThreadPool::started = 0;


//

// 阻塞队列， 大小是1024个， 有界的阻塞队列，无论是线程的大小和还是阻塞队列大小，底层的
int ThreadPool::threadpool_create(int _thread_count, int _queue_size)
{
    bool err = false;
    do
    {
        if(_thread_count <= 0 || _thread_count > MAX_THREADS || _queue_size <= 0 || _queue_size > MAX_QUEUE) 
        {
            _thread_count = 4;
            _queue_size = 1024;
        }
    
        thread_count = 0;
        queue_size = _queue_size;
        head = tail = count = 0;
        shutdown = started = 0;

        threads.resize(_thread_count);
        queue.resize(_queue_size);
    
        /* Start worker threads */
        for(int i = 0; i < _thread_count; ++i) 
        {
            if(pthread_create(&threads[i], NULL, threadpool_thread, (void*)(0)) != 0) 
            {
                //threadpool_destroy(pool, 0);
                return -1;
            }
            ++thread_count;
            ++started;
        }
    } while(false);
    
    if (err) 
    {
        //threadpool_free(pool);
        return -1;
    }
    return 0;
}

// 往线程池当中添加一个任务



int ThreadPool::threadpool_add(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun)
{
    int next, err = 0;
    // 获得锁失败
    if(pthread_mutex_lock(&lock) != 0)
        return THREADPOOL_LOCK_FAILURE;
    do 
    {

        //进入队列操作
       // （1）把值存在rear所在的位置；

      //  （2）rear=（rear+1）%maxsize ，其中maxsize代表数组的长度；
        next = (tail + 1) % queue_size;
        // 队列满
        if(count == queue_size) 
        {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        // 已关闭
        if(shutdown)
        {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        queue[tail].fun = fun;
        queue[tail].args = args;
        tail = next;
        ++count;
        
        /* pthread_cond_broadcast */
        if(pthread_cond_signal(&notify) != 0) 
        {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while(false);

    if(pthread_mutex_unlock(&lock) != 0)
        err = THREADPOOL_LOCK_FAILURE;
    return err;
}


int ThreadPool::threadpool_destroy(ShutDownOption shutdown_option)
{
    printf("Thread pool destroy !\n");
    int i, err = 0;

    if(pthread_mutex_lock(&lock) != 0) 
    {
        return THREADPOOL_LOCK_FAILURE;
    }
    do 
    {
        if(shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        shutdown = shutdown_option;

        if((pthread_cond_broadcast(&notify) != 0) ||
           (pthread_mutex_unlock(&lock) != 0)) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        for(i = 0; i < thread_count; ++i)
        {
            if(pthread_join(threads[i], NULL) != 0)
            {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while(false);

    if(!err) 
    {
        threadpool_free();
    }
    return err;
}

int ThreadPool::threadpool_free()
{
    if(started > 0)
        return -1;
    pthread_mutex_lock(&lock);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notify);
    return 0;
}


// 从线程池任务队列当中出元素
void *ThreadPool::threadpool_thread(void *args)
{
    while (true)
    {
        ThreadPoolTask task;
        // 对阻塞队列 vector 大小修改我主要是采用 锁来保证同步

        // 其实在这里面的话我们可以尝试像java那样使用ArrayBlockingQueue ,

        // head 类似于 takeIndex，  tail 类似于 putIndex, 然后我们队这两个变量的修改我们可以使用compareAndSwap来做，
        //
        pthread_mutex_lock(&lock);
        // count 代表任务队列当中的任务数目
        while((count == 0) && (!shutdown)) 
        {
            pthread_cond_wait(&notify, &lock);
        }
        if((shutdown == immediate_shutdown) ||
           ((shutdown == graceful_shutdown) && (count == 0)))
        {
            break;
        }
        // 下面的函数就是要从阻塞队列当中取出一个任务来，从对头取出来 ，取任务的时候要保证线程安全
        task.fun = queue[head].fun;
        task.args = queue[head].args;
        queue[head].fun = NULL;
        queue[head].args.reset();

        // head  为何要这样做， 因为始终要维持head 数字大小， 其实就是一循环队列
        head = (head + 1) % queue_size;
        --count;   //然后任务队列当中数目减少一个
        pthread_mutex_unlock(&lock);

        //  线程开始执行任务
        (task.fun)(task.args);
    }
    // 线程执行完毕后， 数目减少一个
    --started;
    pthread_mutex_unlock(&lock);
    printf("This threadpool thread finishs!\n");
    pthread_exit(NULL);
    return(NULL);
}