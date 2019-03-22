#include "EventLoop.h"
#include "base/Logging.h"
#include "Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;


// 该变量的作用是进行标记作用，标记一个线程是否创建了一个EventLoop 对象，如果
__thread EventLoop* t_loopInThisThread = 0;

// 前提客户端请求当中当中我们这么一下步骤 read, decode, compute encode write，
//其中read或者write 这类型是I/O时间 在Loop当中进行处理，  并且在调用poller 函数时我们代码还是会阻塞的，为了在线程阻塞的时候还能处理一些任务，
// //我们将而decode encode 或者compute 等计算任务也让线程来做


//[Muduo网络库源码分析（三）线程间使用eventfd通信和EventLoop::runInLoop系列函数](https://blog.csdn.net/NK_test/article/details/51138359)

int createEventfd()
{

   // eventfd 可以完美取代 pipe去通知(唤醒)其他的进程(线程)。
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }
    return evtfd;
}


// 构造函数
EventLoop::EventLoop()
:   looping_(false),
//一个事件循环在构造函数当中自动创建一个Epoll对象
    poller_(new Epoll()),
    wakeupFd_(createEventfd()),   // 唤醒线程使用Eventfd模式
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),

    // 一个EventLoop当中的主线程mainReactor, 去唤醒另外一个pwakeupChannel 当中的SubReactor线程，
    pwakeupChannel_(new Channel(this, wakeupFd_))
{
    if (t_loopInThisThread)
    {
        //LOG << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 采用边缘触发机制
    //pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);

    // pwakeupChannel 就是一个channel 通道，包含一个fd
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epoll_add(pwakeupChannel_, 0);
}

void EventLoop::handleConn()
{
    //poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET | EPOLLONESHOT), 0);
    updatePoller(pwakeupChannel_, 0);
}


EventLoop::~EventLoop()
{
    //wakeupChannel_->disableAll();
    //wakeupChannel_->remove();
    close(wakeupFd_);    // 析构函数 直接关闭 eventfd
    // 当前线程没有创建 EventLoop 对象
    t_loopInThisThread = NULL;
}


//使用eventfd唤醒

//前面说到唤醒IO线程，EventLoop阻塞在poll函数上，怎么去唤醒及时它？
//以前的做法是利用pipe,向pipe中写一个字节，监视在这个pipe的读事件的poll函数就会立刻返回。在muduo中，采用了linux中eventfd调用

void EventLoop::wakeup()
{
    uint64_t one = 1;
    // eventfd进行唤醒功能
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
    if (n != sizeof one)
    {
        LOG<< "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}


//这里面的handleRead() 不是正常的I/O处理，部分，跟Channel和Httpdata 的handleRead是不一样的，这是线程之间的通信机制

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    //pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    // 设计通道上面的fd 关系的时间   关系读事件， 采用边缘触发的模式
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}


// 为了使IO线程在空闲时也能处理一些计算任务
// 在I/O线程中执行某个回调函数，该函数可以跨线程调用

void EventLoop::runInLoop(Functor&& cb)
{

    // 如果是当前IO线程调用runInLoop，则同步调用cb

    if (isInLoopThread())
        cb();
    else
        // 如果是其它线程调用runInLoop，则异步地将cb添加到队列,让IO线程处理
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb)
{
    {
        // 安全上锁，不用释放锁， MutexLockGuard 是释放锁
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

// 事件循环，该函数不能跨线程调用
// 只能在创建该对象的线程中调用


// EventLoop::loop()调用Poller::poll()获得当前活动事件的Channel列表，然后依次调用每个Channel的handleEvent()函数




void EventLoop::loop()
{

    //  在该函数中会循环执行以下过程:调用poller_:poll(), 通过此调用获得一个vectoractiveChannels的就绪事件集合，再遍历该容器， 执行每个channel
    // 的channel::handleEvent() 完成相应的就绪事件对调，然后执行pendingFunctors_排队的函数，
    //
    // 上述一次循环就是在一次Reactor 模式当中完成的
    assert(!looping_);
    assert(isInLoopThread());
    // 设置一些状态变量
    looping_ = true;
    quit_ = false;
    //LOG_TRACE << "EventLoop " << this << " start looping";
    std::vector<SP_Channel> ret;
    // 不断地进行循环操作， 判断只要当前
    while (!quit_)
    {
        //cout << "doing" << endl;
        ret.clear();

        // 采用一个vector来保存准备就绪事件的情况
        // 这里面试可能能阻塞select, epoll等是阻塞的，
        ret = poller_->poll();
        eventHandling_ = true;
        // 调用处理函数, m
        for (auto &it : ret)
            it->handleEvents();    // 处理I/O 任务
        eventHandling_ = false;
        // 然后再调用doPendingFunctors
        // // 执行pending Functors_中的任务回调
        //    // 这种设计使得IO线程也能执行一些计算任务，避免了IO线程在不忙时长期阻塞在IO multiplexing调用中
        doPendingFunctors();   // 非I/O 任务  // 正在调用计算函数   // 为了让IO线程也能执行一些计算任务 合理利用CPU
      // 一个I/O
        // 最后处理过期事件， 在时间循环
        poller_->handleExpired();
    }
    looping_ = false;
}


// 该函数只会被当前IO线程调用    // 为了让IO线程也能执行一些计算任务 合理利用CPU
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 正在调用计算函数

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}



// 该函数可以跨线程调用
void EventLoop::quit()
{
    quit_ = true;  // 退出 IO 线程 让IO线程的loop循环退出 从而退出了IO线程
    if (!isInLoopThread())
    {
        wakeup();

        // 如果调用quit的不是当前的I/O线程，那么就需要唤醒 I/O线程，wakpUp 当前线程
    }
}