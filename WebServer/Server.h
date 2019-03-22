#pragma once
#include "EventLoop.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"
#include <memory>


class Server
{
public:
    Server(EventLoop *loop, int threadNum, int port);
    ~Server() { }
    EventLoop* getLoop() const { return loop_; }
    void start();
    void handNewConn();
    //处理本次连接 直接调用
    void handThisConn() { loop_->updatePoller(acceptChannel_); }

private:
    EventLoop *loop_;
    int threadNum_;
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;// 线程池采用独一unique_ptr指针来进行即可
    bool started_;

    //
    std::shared_ptr<Channel> acceptChannel_;  // 这个通道是给MainReactor 使用的
    int port_;
    int listenFd_;   // 监听文件描述符
    //
    static const int MAXFDS = 100000;
};