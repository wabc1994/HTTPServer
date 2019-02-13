// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Thread.h"
#include "CurrentThread.h"
#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
using namespace std;


namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "default";
}


pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

// 为了在线程中保留name,tid这些数据
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;//函数适配接收的函数
    string name_;   // 线程的名字
    pid_t* tid_;   // 线程标识符
    CountDownLatch* latch_;    // 线程的同步器类

    ThreadData(const ThreadFunc &func, const string& name, pid_t *tid, CountDownLatch *latch)
    :   func_(func),
        name_(name),
        tid_(tid),
        latch_(latch)
    { }


    // 问题是没有看到回调函数在哪里
    void runInThread()
    {
        // 获取线程的tid
        *tid_ = CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;

        // 缓存该线程的名字
        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);

        func_();
        CurrentThread::t_threadName = "finished";
    }
};


// 线程的入口函数，调用runInThread函数
void *startThread(void* obj)
{
    ThreadData* data = static_cast<ThreadData*>(obj);
    //
    data->runInThread();
    delete data;
    return NULL;
}


Thread::Thread(const ThreadFunc &func, const string &n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n),
    latch_(1)
{
    setDefaultName();
}


// 线程之间的析构函数
Thread::~Thread()
{
    if (started_ && !joined_)
        pthread_detach(pthreadId_);
}


// 线程的默认名字
void Thread::setDefaultName()
{
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}


// 启动线程
void Thread::start()
{
    assert(!started_);
    started_ = true;
    //创建线程，startThread为线程的入口函数
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, data))
    {
        started_ = false;
        delete data;
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}


// 底层还是使用的pthread_t
int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}