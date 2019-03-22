
#include "EventLoopThreadPool.h"

//EventLoopThreadPool（IO线程池)

// 在面试的时候，我们可以将EventLoopThreadPool 叫做为I/O线程池

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
:   baseLoop_(baseLoop),
    started_(false),
    numThreads_(numThreads),   //
    // 刚开始的使用我们是将next_初始化为0,
    next_(0)
{
    if (numThreads_ <= 0)
    {
        LOG << "numThreads_ <= 0";
        abort();
    }
}

void EventLoopThreadPool::start()
{
    baseLoop_->assertInLoopThread();  // 先确保mainReactor 已经该该线程当中进行一个循环操作，
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        //线程池当中的每一个I/O线程都是使用share_ptr进行封装的
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        // 放进入线程的vector
        threads_.push_back(t);
        //
        loops_.push_back(t->startLoop());
    }
}



//这里面就是mainEventLoop 和subEventLoop 之间如何进行任务分配的， 主要采用的是轮旋算法
EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();  //
    // 确保第一个
    assert(started_);
    EventLoop *loop = baseLoop_;   // 关于这里面为何要设置成为baseLoop，主要是如果vector<EventLoop>为空的话，即使服务器当中没有SubReactor的话

    // 我们可以直接返回MainReactor

    if (!loops_.empty())
    {
        // next 代表开头一个元素
        loop = loops_[next_];  //  loops 代表任务
      //  std::vector<EventLoop*> loops_;  //采用向量的方式
        next_ = (next_ + 1) % numThreads_;
    }
    return loop;
}