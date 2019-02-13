#include "epoll.h"
#include <sys/epoll.h>
#include <errno.h>
#include "threadpool.h"

// 声明一个结构体
struct epoll_event* events;

//1. struct epoll_event
//        结构体epoll_event被用于注册所感兴趣的事件和回传所发生待处理的事件，定义如下：
//typedef union epoll_data {
//    void *ptr;
//    int fd;
//    __uint32_t u32;
//    __uint64_t u64;
//} epoll_data_t;//保存触发事件的某个文件描述符相关的数据
//struct epoll_event {
//    __uint32_t events;      /* epoll event */
//    epoll_data_t data;      /* User data variable */
//};

int epoll_init()
{
    // 可以监听的最多
    int epoll_fd = epoll_create(LISTENQ + 1);
    if(epoll_fd == -1)
        return -1;
    //events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    //  主要是这个MAXEVENTS
    events = new epoll_event[MAXEVENTS];
    return epoll_fd;
}

// 注册新描述符

// 还是调用ctl 然后第二参数是EPOLL_CTL_ADD(在epoll_fd )
int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    //printf("add to epoll %d\n", fd);
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_add error");
        return -1;
    }
    return 0;
}

// 修改描述符状态
int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        perror("epoll_mod error");
        return -1;
    } 
    return 0;
}

// 从epoll中删除描述符
int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("epoll_del error");
        return -1;
    } 
    return 0;
}

// 返回活跃事件数，只是在这里面返回而已，并没有进行一个处理，

// max_events 代表每次能处理的最大数目
// d
int my_epoll_wait(int epoll_fd, struct epoll_event* events, int max_events, int timeout)
{
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    if (ret_count < 0)
    {
        perror("epoll wait error");
    }
    return ret_count;
}