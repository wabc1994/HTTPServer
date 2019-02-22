// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Util.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

const int MAX_BUFF = 4096;

//1.小于0，出错

//2.等于0，对方关闭

//3.大于0，但是小于count，对方关闭，只有

//4.count，代表读满count个字节




//当剩余长度大于0的时候就一直读啊读
//
//   1. 当read的返回值小于0的时候，做异常检测
//  2. 当read的返回值等于0的时候，退出循环
//  3. 当read的返回值大于0的时候，拿剩余长度减read的返回值，拿到新的剩余长度，读的入口指针加上read的返回值，进入步骤1
//  4. 返回参数n减去剩余长度，即实际读取的总长度



//readn在未出错或者fd没有关闭的情况下，会读满count个字节。
ssize_t readn(int fd, void *buff, size_t n)
{
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char *ptr = (char*)buff;
    while (nleft > 0)
    {
        if ((nread = read(fd, ptr, nleft)) < 0)
        {
            if (errno == EINTR)
                nread = 0;
            else if (errno == EAGAIN)
            {
                return readSum;
            }
            else
            {
                return -1;
            }  
        }
        else if (nread == 0)
            break;
        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }
    return readSum;
}

ssize_t readn(int fd, std::string &inBuffer, bool &zero)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true)
    {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0)
        {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                return readSum;
            }  
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if (nread == 0)
        {
            //printf("redsum = %d\n", readSum);
            zero = true;
            break;
        }
        //printf("before inBuffer.size() = %d\n", inBuffer.size());
        //printf("nread = %d\n", nread);
        readSum += nread;
        //buff += nread;
        inBuffer += std::string(buff, buff + nread);
        //printf("after inBuffer.size() = %d\n", inBuffer.size());
    }
    return readSum;
}


ssize_t readn(int fd, std::string &inBuffer)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true)
    {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0)
        {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                return readSum;
            }  
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if (nread == 0)
        {
            //printf("redsum = %d\n", readSum);
            break;
        }
        //printf("before inBuffer.size() = %d\n", inBuffer.size());
        //printf("nread = %d\n", nread);
        readSum += nread;
        //buff += nread;
        inBuffer += std::string(buff, buff + nread);
        //printf("after inBuffer.size() = %d\n", inBuffer.size());
    }
    return readSum;
}

// 要么读取n 字节的数据，要么读取失败，只有两种可能性， 要么读取成功，要么读取失败，本质也是通过循环



//当要写入的剩余长度大于0的时候就一直写啊写
  //      当write的返回值小于0的时候，做异常检测
    //    当write的返回值等于0的时候，出错退出程序
      //  当write的返回值大于0的时候，拿剩余长度减去write的返回值，拿到新的剩余长度，写的入口指针加上write的返回值，进入步骤1
        //返回参数n的值，即期望写入的总长度

ssize_t writen(int fd, void *buff, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char *ptr = (char*)buff;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0)
            {
                if (errno == EINTR)
                {
                    nwritten = 0;
                    continue;
                }
                else if (errno == EAGAIN)
                {
                    return writeSum;
                }
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return writeSum;
}

ssize_t writen(int fd, std::string &sbuff)
{
    size_t nleft = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const char *ptr = sbuff.c_str();
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0)
            {
                if (errno == EINTR)
                {
                    nwritten = 0;
                    continue;
                }
                else if (errno == EAGAIN)
                    break;
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    if (writeSum == static_cast<int>(sbuff.size()))
        sbuff.clear();
    else
        sbuff = sbuff.substr(writeSum);
    return writeSum;
}

void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}

int setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

void setSocketNodelay(int fd) 
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

void setSocketNoLinger(int fd) 
{
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER,(const char *) &linger_, sizeof(linger_));
}

void shutDownWR(int fd)
{
    shutdown(fd, SHUT_WR);
    //printf("shutdown\n");
}

int socket_bind_listen(int port)
{
    // 检查port值，取正确区间范围
    if (port < 0 || port > 65535)
        return -1;

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    // 消除bind时"Address already in use"错误
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        return -1;

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // 开始监听，最大等待队列长为LISTENQ
    if(listen(listen_fd, 2048) == -1)
        return -1;

    // 无效监听描述符
    if(listen_fd == -1)
    {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}