// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "CountDownLatch.h"


// 按照陈硕 muduo 类库当中的是
// 主要是三个 mutex
//  condition ， 条件变量要跟mutex 结合起来使用
// count_ 计数器

// mutex 锁
CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
    count_(count)
{ }



void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while (count_ > 0)
        condition_.wait();
}


void CountDownLatch::countDown()
{

    //MutexLockGuard 和 MutexLock 配合使用
    // 先定义好一个mutex 然后放到 MutexLockGuard 当中
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
        condition_.notifyAll();
}