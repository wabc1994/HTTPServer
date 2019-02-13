// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "base/Thread.h"
#include "Epoll.h"
#include "base/Logging.h"
#include "Channel.h"
#include "base/CurrentThread.h"
#include "Util.h"
#include <vector>
#include <memory>
#include <functional>

#include <iostream>
using namespace std;

// poller 是对epoll 系列函数的封装

// epoll 系列主要有三个函数

// epoll_create epoll_ctl (然后对fd,有三种模式add,del mod 三种模式可以供我们进行选择)(注册文件描述符以及相应的)   epoll_wait  返回准备就绪的文件描述符


//one loop per thread意味着每个线程只能有一个EventLoop对象，用变量
class EventLoop {
public:
    typedef std::function<void()> Functor;

    EventLoop();

    ~EventLoop();


    void loop();  // 是否在进行循环，  完成一次循环就代表完成一次reactor 模式
    void quit();   // 是否退出循环的基本可能性


    void runInLoop(Functor &&cb);

    void queueInLoop(Functor &&cb);

    // 声明为const 类的函数，子类不能就行重载，重写， 该函数不能被修改
    // 判断当前的

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    //已经赋值就说明当前线程已经创建过EventLoop对象,判断当前线程是否已经创建过EventLoop
    void assertInLoopThread() {
        assert(isInLoopThread());
    }

     // 优雅关闭
    void shutdown(shared_ptr<Channel> channel) {
        shutDownWR(channel->getFd());
    }

    // EventLoop当中的removeFromPoller， updatePoller addToPoller
    // 其实就是标准Reactor 模式当中的 removeHandler--removeFromPoller, registerHandler(addToPoller模式) 或者
    void removeFromPoller(shared_ptr<Channel> channel)
    //把当前事件从EventLoop中移除
    {
        //shutDownWR(channel->getFd());
        poller_->epoll_del(channel);
    }

    void updatePoller(shared_ptr<Channel> channel, int timeout = 0) {
        poller_->epoll_mod(channel, timeout);
    }

    void addToPoller(shared_ptr<Channel> channel, int timeout = 0) {
        // channel 其实就是socket 或者 fd, 监听的事件  ， timeout  代表设置的过期时间
        poller_->epoll_add(channel, timeout);
    }

private:
    // 声明顺序 wakeupFd_ > pwakeupChannel_
    bool looping_;
    // 这里面的poller 直接指向一个 Epoll
    shared_ptr<Epoll> poller_;   // 包含一个poller对象，其中poller是对I/O复用的分装，这里面我采用了epoll当中的ET模式

    int wakeupFd_;   // 就是eventfd
    bool quit_;
    bool eventHandling_; // 是否在处理准备就绪事件， 如果正在
    mutable MutexLock mutex_;  //自定义RAII C++实现范围互斥锁
    std::vector<Functor> pendingFunctors_;  // 每个fd或者socket 对应上面的处理函数，
    bool callingPendingFunctors_;  //是否调用处理函数 该种情况
    const pid_t threadId_;    // 当前eventloop 线程 id
    shared_ptr<Channel> pwakeupChannel_;

    void wakeup();

    void handleRead();

    void doPendingFunctors();
}

