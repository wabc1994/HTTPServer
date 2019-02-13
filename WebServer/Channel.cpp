// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Channel.h"
#include "Util.h"
#include "Epoll.h"
#include "EventLoop.h"
#include <unistd.h>
#include <queue>
#include <cstdlib>
#include <iostream>
using namespace std;

//1.首先我们给定Channel所属的loop以及其要处理的fd
//2.接着我们开始注册fd上需要监听的事件，如果是常用事件(读写等)的话，我们可以直接调用接口enable***来注册对应fd上的事件，与之对应的是disable*用来销毁特定的事件
//3.在然后我们通过set***Callback来事件发生时的回调

Channel::Channel(EventLoop *loop):
    loop_(loop),
    events_(0),
    lastEvents_(0)
{ }

Channel::Channel(EventLoop *loop, int fd):
    loop_(loop),
    fd_(fd), 
    events_(0),
    lastEvents_(0)
{ }

Channel::~Channel()
{
    //loop_->poller_->epoll_del(fd, events_);
    //close(fd_);
}

int Channel::getFd()
{
    return fd_;
}
void Channel::setFd(int fd)
{
    fd_ = fd;
}

void Channel::handleRead()
{
    // readHandler_ 是callback 变量
    if (readHandler_)
    {
        readHandler_();
    }
}

// 在该w文件描述符上面注册事件   上面就行
void Channel::handleWrite()
{
    //
    if (writeHandler_)
    {
        writeHandler_();
    }
}

void Channel::handleConn()
{
    if (connHandler_)
    {
        connHandler_();
    }
}