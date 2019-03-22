#pragma once
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>

class Epoll
{
public:
    Epoll();
    ~Epoll();
    // 从这里面我们就可以看粗 Channel 是如何与epoll 系列函数进行关注的， 然后我们可以查看下 timeout 参数最终是设置在哪里的
    void epoll_add(SP_Channel request, int timeout);
    void epoll_mod(SP_Channel request, int timeout);
    void epoll_del(SP_Channel request);
    std::vector<std::shared_ptr<Channel>> poll();
    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
    void add_timer(std::shared_ptr<Channel> request_data, int timeout);
    int getEpollFd()
    {
        return epollFd_;
    }
    void handleExpired();
private:
    static const int MAXFDS = 100000;
    int epollFd_;
    std::vector<epoll_event> events_;
    std::shared_ptr<Channel> fd2chan_[MAXFDS];
    std::shared_ptr<HttpData> fd2http_[MAXFDS];
    TimerManager timerManager_;
};