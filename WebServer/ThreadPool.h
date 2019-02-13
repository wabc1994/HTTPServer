// @Author Lin Ya
// @Email xxbbb@vip.qq.com

// This file has not been used
#pragma once
#include "Channel.h"
#include <pthread.h>
#include <functional>
#include <memory>
#include <vector>

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;        //  如果没有指定参数，那么就是4个， 最多只能有1024个
const int MAX_QUEUE = 65535;    // 阻塞队列的大小

typedef enum
{
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} ShutDownOption;


// 任务queu 存储的结构
struct ThreadPoolTask
{
    std::function<void(std::shared_ptr<void>)> fun;
    std::shared_ptr<void> args;
};


class ThreadPool
{
private:
    static pthread_mutex_t lock;      // 线程池包含一个锁
    static pthread_cond_t notify;      // 和一个条件变量
     // 自己呢不定义线程， 直接实现pthread_t 线程体
    static std::vector<pthread_t> threads;
    static std::vector<ThreadPoolTask> queue;
    static int thread_count;
    static int queue_size;
    static int head;       // head 和tail 都是整形变量
    // tail 指向尾节点的下一节点
    static int tail;
    static int count;
    static int shutdown;
    static int started;
public:
    static int threadpool_create(int _thread_count, int _queue_size);
    static int threadpool_add(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun);
    static int threadpool_destroy(ShutDownOption shutdown_option = graceful_shutdown);
    static int threadpool_free();
    static void *threadpool_thread(void *args);
};