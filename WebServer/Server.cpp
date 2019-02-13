// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Server.h"
#include "base/Logging.h"
#include "Util.h"
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//  HttpServer（http服务器封装）

//服务器创建EventLoopThreadPool,该线程池负责分配个SubReactor

// 并且服务器也绑定MainReactor, 但是MainReactor 是由Main.cpp 创建的
Server::Server(EventLoop *loop, int threadNum, int port)
:   loop_(loop),
    threadNum_(threadNum),
    // loop_ 是mainReactor
    eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
    started_(false),
    acceptChannel_(new Channel(loop_)),
    port_(port),
    listenFd_(socket_bind_listen(port_))
{

    // 服务器进行监听得到的第一个套接字描述符
    acceptChannel_->setFd(listenFd_);


    handle_for_sigpipe();
    if (setSocketNonBlocking(listenFd_) < 0)
    {
        perror("set socket non block failed");
        abort();
    }
}

void Server::start()
{
    // 服务器开启的话，线程池就开启，然后接受到第一个请求
    eventLoopThreadPool_->start();
    //acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    // 第一fd 肯定是读事件
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));
    acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));
    // 第一个channel 注册到MainReactor上面去
    loop_->addToPoller(acceptChannel_, 0);
    started_ = true;
}

// 处理接下来的请求
void Server::handNewConn()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    // 处理新的连接 ，
    while((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
    {
        // 新来的连接采用线程池的方式进行处理
        EventLoop *loop = eventLoopThreadPool_->getNextLoop();
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);
        // cout << "new connection" << endl;
        // cout << inet_ntoa(client_addr.sin_addr) << endl;
        // cout << ntohs(client_addr.sin_port) << endl;
        /*
        // TCP的保活机制默认是关闭的
        int optval = 0;
        socklen_t len_optval = 4;
        getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
        cout << "optval ==" << optval << endl;
        */
        // 限制服务器的最大并发连接数
        if (accept_fd >= MAXFDS)
        {
            close(accept_fd);    //如果超过了服务器最大的并发数，服务器可以直接关闭
            continue;
        }
        // 设为非阻塞模式
        if (setSocketNonBlocking(accept_fd) < 0)
        {
            LOG << "Set non block failed!";
            //perror("Set non block failed!");
            return;
        }

        setSocketNodelay(accept_fd);
        //setSocketNoLinger(accept_fd);
         // 对客户端的请求进行一个
        shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
        req_info->getChannel()->setHolder(req_info);
        loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
    }
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}