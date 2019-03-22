// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThread.h"
#include <functional>


EventLoopThread::EventLoopThread()
:   loop_(NULL),
    exiting_(false),
    thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
    mutex_(),
    cond_(mutex_)
{ }

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {
        // 停止当前的I/O事件循环，让并让当前的处理完成
        loop_->quit();
        thread_.join();  // 等待线程退出
    }
}


// 启动线程 该线程成为IO线程
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        // 一直等到threadFun在Thread里真正跑起来
        while (loop_ == NULL)   // loop为空一直等待 直到有IO事件发生
            cond_.wait(); // 进行循环
    }
    return loop_;
}


// 线程函数  释放线程池所申请的内存资源。
void EventLoopThread::threadFunc()
{
    EventLoop loop;
    // I/O线程的关键

      // 任何一个线程，只要创建并运行了EventLoop，都称之为IO线程

    // 确保I/0线程里面使用的EventLoop 是在本地线程创建的

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    //
    loop.loop();
    //assert(exiting_);
    loop_ = NULL;
}