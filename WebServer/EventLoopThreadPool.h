// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "base/noncopyable.h"
#include "EventLoopThread.h"
#include "base/Logging.h"
#include <memory>
#include <vector>

class EventLoopThreadPool : noncopyable
{
public:
    // baseLoop其实就是 mainReactor， numThreads 才是subReactor当中涉及到的线程池数目多少
    //
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads);

    //
    ~EventLoopThreadPool()
    {
        LOG << "~EventLoopThreadPool()";
    }
    void start();

    EventLoop

private:
    EventLoop* baseLoop_;  // MainReactor
    bool started_;
    int numThreads_;   // 线程池当中线程池的数目
    int next_;
    // 线程池还是采用简单的vector
    std::vector<std::shared_ptr<EventLoopThread>> threads_;   // EventLoop线程池，
    std::vector<EventLoop*> loops_;   //任务队列就是循环事件  eventloop是一个对象，thread 是一个对象，eventloop和thread 结合起来就是
    // eventloopthread ,然后eventloopthread 当中 包括 基于vector<eventLoop> 和vector<>
};