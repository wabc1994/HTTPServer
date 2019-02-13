// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <functional>


AsyncLogging::AsyncLogging(std::string logFileName_,int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(logFileName_),
    // 线程函数
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_(),  // 缓存区数组
    latch_(1)
{
    // 在构造函数中latch_的值为1,初始条件变量
    // 线程运行之后将latch_的减为0
    assert(logFileName_.size() > 1);
    currentBuffer_->bzero();   //
    nextBuffer_->bzero();
    buffers_.reserve(16);
}



//所有LOG_*最终都会调用append函数  , 前台I/O线程调用，往buffer当中写入日志消息
void AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);
    if (currentBuffer_->avail() > len)
        // 如果当前buffer还有空间，就添加到当前日志
        currentBuffer_->append(logline, len);
    else  // 当前buffer不够了
    {
        // 把当前buffer添加到buffer列表中
        buffers_.push_back(currentBuffer_);   //
        currentBuffer_.reset();
        if (nextBuffer_)
            currentBuffer_ = std::move(nextBuffer_);
        else
            //如果前端写入速度太快了，一下子把两块缓冲都用完了，那么只好分配一块新的buffer,作当前缓冲，这是极少发生的情况 新建一个缓存区
            currentBuffer_.reset(new Buffer);
        currentBuffer_->append(logline, len);

        // 通知日志线程，有数据可写 ，调用threadFunc
        // 也就是说，只有当缓冲区满了之后才会将数据写入日志文件中，
        cond_.notify();
    }
}

// 日志线程从缓冲区当中读取数据，往磁盘文件当中写入数据
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);   // 磁盘当中的名字output
    BufferPtr newBuffer1(new Buffer);      //这两个是后台线程的buffer
    BufferPtr newBuffer2(new Buffer);
    //就是将一个指针变量代表的内容清零，它是memset的变体，但意义一样
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;   //用来和前台线程的buffers_进行swap.

    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            // 缓冲区有前端线程和日志线程共同进行操作，所以也是互斥变量，临界区问题
            MutexLockGuard lock(mutex_);
            // 如果buffer为空，那么表示没有数据需要写入文件，那么就等待指定的时间（注意这里没有用倒数计数器）
            if (buffers_.empty())  // unusual usage!
            {
                // 条件变量进行等待
                cond_.waitForSeconds(flushInterval_);
            }

            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();

            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());


        // 如果将要写入文件的buffer列表中buffer的个数大于25，那么将多余数据删除,
        // 消息堆积
        //前端陷入死循环，拼命发送日志消息，超过后端的处理能力
        //这是典型的生产速度超过消费速度，会造成数据在内存中的堆积
        //严重时引发性能问题(可用内存不足),
        //或程序崩溃(分配内存失败)

        if (buffersToWrite.size() > 25)
        {
            //char buf[256];
            // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
            //          Timestamp::now().toFormattedString().c_str(),
            //          buffersToWrite.size()-2);
            //fputs(buf, stderr);
            //output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }


        // output 是LogFile
        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
