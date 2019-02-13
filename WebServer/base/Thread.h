// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "CountDownLatch.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>


class Thread : noncopyable
{
public:
    typedef std::function<void ()> ThreadFunc;
    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
    ~Thread();
    void start();
    int join();
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

private:
    void setDefaultName();
    bool started_;   // 线程是否启动
    bool joined_;
    pthread_t pthreadId_;  // 线程的pthread_t
    pid_t tid_;   // 线程的pid
    ThreadFunc func_;   // 线程的回调函数
    std::string name_;   // 线程的默认名字
    CountDownLatch latch_;    // 确保线程当中的函数跑起来
};