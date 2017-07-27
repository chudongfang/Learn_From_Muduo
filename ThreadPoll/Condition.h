// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "Mutex.h"

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

    class Condition : boost::noncopyable
    {
    public:
        //构造函数，初始化条件变量
        explicit Condition(MutexLock& mutex)
                : mutex_(mutex)
        {
            MCHECK(pthread_cond_init(&pcond_, NULL));
        }
        //析构函数，释放条件变量
        ~Condition()
        {
            MCHECK(pthread_cond_destroy(&pcond_));
        }
        //解除锁对应的线程ID，并等待条件变量
        void wait()
        {
            MutexLock::UnassignGuard ug(mutex_);
            MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
        }

        // returns true if time out, false otherwise.
        //时间等待
        bool waitForSeconds(double seconds);
        //唤醒一个等待条件变量的线程
        void notify()
        {
            MCHECK(pthread_cond_signal(&pcond_));
        }
        //发广播形式唤醒所有等待该环境变量的线程
        void notifyAll()
        {
            MCHECK(pthread_cond_broadcast(&pcond_));
        }

    private:
        MutexLock& mutex_;  //互斥锁
        pthread_cond_t pcond_;//条件变量
    };

}
#endif  // MUDUO_BASE_CONDITION_H
