// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "Condition.h"
#include "Mutex.h"

#include <boost/noncopyable.hpp>

namespace muduo
{

    class CountDownLatch : boost::noncopyable
    {
    public:
        //构造函数，初始化倒计时变量
        explicit CountDownLatch(int count);
        //等待count_为0
        void wait();
        //变量减一
        void countDown();
        //返回变量值
        int getCount() const;

    private:
        mutable MutexLock mutex_;//互斥锁，mutable修饰符表示其可以在任何情况下变化
        Condition condition_;//条件变量
        int count_; //计时数
    };

}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
