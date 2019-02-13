// @Author Lin Ya
// @Email xxbbb@vip.qq.com
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
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    bool started_;

    //
    std::shared_ptr<Channel> acceptChannel_;  // 这个通道是给MainReactor 使用的
    int port_;
    int listenFd_;
    //
    static const int MAXFDS = 100000;
};