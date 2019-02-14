// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "CountDownLatch.h"
#include "MutexLock.h"
#include "Thread.h"
#include "LogStream.h"
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>

class AsyncLogging : noncopyable
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if (running_)
            stop();
    }
    void append(const char* logline, int len);


    // 开始启动异步日志
    void start()
    {
        // 在构造函数中latch_的值为1
        // 线程运行之后将latch_的减为0
        running_ = true;
        thread_.start();

        //必须等到latch_变为0才能从start函数中返回，这表明初始化已经完成,
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }


private:
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    typedef std::shared_ptr<Buffer> BufferPtr;
    const int flushInterval_;   // 超时时间，在flushInterval_秒内，缓冲区没写满，仍将缓冲区的数据写到文件中

    bool running_;      //
    std::string basename_;
    Thread thread_;   // 执行该异步日志记录器的线程
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_;   //采用双缓冲区的功能  // 当前的缓冲区
    BufferPtr nextBuffer_;       // 预备缓存区
    BufferVector buffers_;    // 缓冲区队列，待写入文件，所以可以写入的都放在这里面
    CountDownLatch latch_;      // 倒计时计数器初始化为1，用于指示什么时候日志记录器才能开始正常工作
};