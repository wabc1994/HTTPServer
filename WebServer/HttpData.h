// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "Timer.h"
#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <unistd.h>

class EventLoop;
class TimerNode;
class Channel;

// 处理过程
enum ProcessState
{
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,    // 处理请求数据
    STATE_ANALYSIS,
    STATE_FINISH
};

enum URIState
{
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};

enum HeaderState
{
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum AnalysisState
{
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR
};

enum ParseState
{
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum ConnectionState
{
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED    
};

enum HttpMethod
{
    // 主要实现三种方法
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};

enum HttpVersion
{
    HTTP_10 = 1,
    HTTP_11
};

class MimeType
{
private:
    static void init();
    static std::unordered_map<std::string, std::string> mime;
    MimeType();
    MimeType(const MimeType &m);

public:
    static std::string getMime(const std::string &suffix);

private:
    static pthread_once_t once_control;
};

// httpData 代表一个客户端请求， 在这俩面我们要进行封装，为他分配一个fd Eventloop，channel,timer 定时器谁来处理这个请求 哪个I/O线程， 定时器该设置为多少的基本情况
//

class HttpData: public std::enable_shared_from_this<HttpData>
{
public:
    HttpData(EventLoop *loop, int connfd);
    ~HttpData() { close(fd_); }
    void reset();
    void seperateTimer();


    // 将一个定时器跟一个客户端关联起来

    void linkTimer(std::shared_ptr<TimerNode> mtimer)
    {
        // shared_ptr重载了bool, 但weak_ptr没有
        timer_ = mtimer; 
    }
    std::shared_ptr<Channel> getChannel() { return channel_; }
    EventLoop *getLoop() { return loop_; }
    void handleClose();
    void newEvent();

private:
    EventLoop *loop_;
    std::shared_ptr<Channel> channel_;
    int fd_;
    std::string inBuffer_;
    std::string outBuffer_; // 输出缓冲区
    bool error_;
    ConnectionState connectionState_;   // 处理连接状态，

    HttpMethod method_;
    HttpVersion HTTPVersion_;
    std::string fileName_;
    std::string path_;
    int nowReadPos_;
    ProcessState state_;      // 处理请求状态
    ParseState hState_;   // 处理请求头部的状态
    bool keepAlive_;
    // requst header
    std::map<std::string, std::string> headers_;
    std::weak_ptr<TimerNode> timer_;    // 每个请求关联一个过期时间，过期时间为何采用weak_ptr 指针来表示

    void handleRead();    // 处理read
    void handleWrite();   // 处理写操作w
    void handleConn();      // 请求连接 TCP连接
    void handleError(int fd, int err_num, std::string short_msg);
    URIState parseURI();
    HeaderState parseHeaders();
    AnalysisState analysisRequest();
};