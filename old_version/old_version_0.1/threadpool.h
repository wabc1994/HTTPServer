#ifndef THREADPOOL
#define THREADPOOL
#include "requestData.h"
#include <pthread.h>

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

// 类似于Java当中线程池中线程的最大数量MaxmiumPoolsize，还有就是有界队列的问题

const int MAX_THREADS = 1024;  //最多的线程数目 是1024个
const int MAX_QUEUE = 65535;      //任务队列数目是 65535

typedef enum 
{
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

//数据结构主要有两个任务，线程做的事情，类似于java 当中runnable 接口，就是做做的事情

typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.// 通知工作线程数量的基本情况
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
struct threadpool_t
{
    pthread_mutex_t lock;    //  互斥锁
    pthread_cond_t notify;    // 条件变量 ，达到条件变量的时候就会唤醒线程
    pthread_t *threads;    //  线程数组的起始指针
    threadpool_task_t *queue;   // 任务队列数组的起始指针
    int thread_count;       //   线程数量
    int queue_size;       // 任务队列长队
    int head;   // 当前任务队列长度
    int tail;    // 当前任务队列尾部
    int count;   // 当前待执行的任务数量
    int shutdown;   // 线程池当前是否关闭
    int started;   //  正在执行的线程数
};

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);

// 分别对外暴露的接口
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);
int threadpool_destroy(threadpool_t *pool, int flags);
int threadpool_free(threadpool_t *pool);
static void *threadpool_thread(void *threadpool);

#endif