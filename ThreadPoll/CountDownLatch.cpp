// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "CountDownLatch.h"
#include <cstdio>
using namespace muduo;
//析构函数，对其进行初始化
CountDownLatch::CountDownLatch(int count)
        : mutex_(),
          condition_(mutex_),
          count_(count)
{
}
//等待变量到0
void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while (count_ > 0)
    {
        condition_.wait();
    }
}

//对变量减一，如果其为0，则唤醒等待的进程
void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    printf("%d-1\n",count_);
    --count_;
    if (count_ == 0)
    {
        condition_.notifyAll();
    }
}
//返回计数递减变量值
int CountDownLatch::getCount() const
{
    MutexLockGuard lock(mutex_);
    return count_;
}

