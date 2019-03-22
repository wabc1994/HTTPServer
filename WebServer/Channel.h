#pragma once
#include "Timer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <sys/epoll.h>


class EventLoop;
class HttpData;

// 每个channel对 对应一个EventLoop ，channel 通道类负责进行事件分发器，每个channel 只属于一个EventLoop， 每个channel 只负责一个文件
// 文件描述符fd,的I/0事件分发，但是其不拥有fd

class Channel
{
private:
    // callback 采用回调函数进行封装
    typedef std::function<void()> CallBack;
    EventLoop *loop_;   // chnannel关联一个loop
    int fd_;  //   文件描述符

   // events_是关心的IO事件，由用户设置；revents_是目前活动的事件，由EventLoop/Poller设置；
    __uint32_t events_;   // 文件描述上面的注册事件， 又
    __uint32_t revents_;   // 文件描述符的就绪事件，由poller：poll设置
    __uint32_t lastEvents_;

    // 方便找到上层持有该Channel的对象   , 一个channenl对象关联一个httpData
    std::weak_ptr<HttpData> holder_;

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

    CallBack readHandler_;   // callback
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;  // 处理连接

public:
    Channel(EventLoop *loop);
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int getFd();
    void setFd(int fd);

    void setHolder(std::shared_ptr<HttpData> holder)
    {
        holder_ = holder;
    }
    std::shared_ptr<HttpData> getHolder()
    {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    // 具体的回调函数是在HttpRequst 给出的情况
    // readHandler_ shi
    void setReadHandler(CallBack &&readHandler)
    {
        readHandler_ = readHandler;
    }
    void setWriteHandler(CallBack &&writeHandler)
    {
        writeHandler_ = writeHandler;
    }
    void setErrorHandler(CallBack &&errorHandler)
    {
        errorHandler_ = errorHandler;
    }
    void setConnHandler(CallBack &&connHandler)
    {
        connHandler_ = connHandler;
    }

    void handleEvents()
    {
        events_ = 0;
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            events_ = 0;
            return;
        }
        if (revents_ & EPOLLERR)
        {
            if (errorHandler_) errorHandler_();
            events_ = 0;
            return;
        }
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
        {
            handleRead();
        }
        if (revents_ & EPOLLOUT)
        {
            handleWrite();
        }
        handleConn();
    }
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    void setRevents(__uint32_t ev)
    {
        revents_ = ev;
    }

    void setEvents(__uint32_t ev)
    {
        events_ = ev;
    }
    __uint32_t& getEvents()
    {
        return events_;
    }

    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    __uint32_t getLastEvents()
    {
        return lastEvents_;
    }

};

typedef std::shared_ptr<Channel> SP_Channel;