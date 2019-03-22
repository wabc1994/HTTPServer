#pragma once
#include "noncopyable.h"
#include <pthread.h>
#include <cstdio>


// 自定义RAII C++实现范围互斥锁 , 主要有两个地方使用了RAII方法， 对pthread_mutex_t mutex进行一个封装成为mutex 和一个条件变量
// 其中构造函数当中调用pthread_mutex_init,在析构函数当中调用pthread_mutex_destroy()
// 在析构函数当中


// 这样定义定义之后，

//https://blog.csdn.net/liuxuejiang158blog/article/details/10953305
class MutexLock: noncopyable
{
public:
    MutexLock()
    {
        pthread_mutex_init(&mutex, NULL);
    }
    ~MutexLock()
    {
        pthread_mutex_destroy(&mutex);
    }

    //
    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_t *get()
    {
        return &mutex;
    }
private:
    pthread_mutex_t mutex;

// 友元类不受访问权限影响
private:
    friend class Condition;
};


// MutexLockGuard 包括一个mutex变量然后对 MutexLock 进行封装, MutexLock实际上是对 pthrea_mutex_t 变量的封装


//下面是RAII类的标准设计方法
class MutexLockGuard: noncopyable   //RAII class
{
public:
    explicit MutexLockGuard(MutexLock &_mutex):
    mutex(_mutex)
    {
        mutex.lock();   //构造时加锁

    }
    ~MutexLockGuard()
    {
        mutex.unlock(); //析构时解锁
    }
private:
    MutexLock &mutex;
};

// 这也是避免死锁的方式，
// 禁止复制

// 使用的时候，是先定义一个mutex 对象，然后 Mutex mutex， MutexLockGurad lock(mutex)