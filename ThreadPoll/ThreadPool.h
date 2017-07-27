// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include "Types.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{

    class ThreadPool : boost::noncopyable
    {
    public:
        typedef boost::function<void ()> Task;

        //构造函数，设置默认线程池名
        explicit ThreadPool(const string& nameArg = string("ThreadPool"));
        ~ThreadPool();

        // Must be called before start().
        //设置线程池最大线程数量
        void setMaxQueueSize(int maxSize) {
            assert(maxSize>=0);
            maxQueueSize_ = maxSize; 
        }

        //设置线程初始化回调函数
        void setThreadInitCallback(const Task& cb)
        { threadInitCallback_ = cb; }

        //开启线程池
        void start(int numThreads);

        //停止线程池
        void stop();

        //返回线程池名
        const string& name() const
        { return name_; }

        //返回队列大小
        size_t queueSize() const;

        // Could block if maxQueueSize > 0
        //向任务队列放入任务
        void run(const Task& f);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        void run(Task&& f);
#endif

    private:
        bool isFull() const;//判断线程池是否满
        void runInThread(); //子进程函数，在子进程中执行任务
        Task take();        //从任务队列中取任务

        mutable MutexLock mutex_;//互斥锁，保证数据互斥访问
        Condition notEmpty_;     //条件变量，是否为空
        Condition notFull_;      //条件变量，是否满
        string name_;            //线程名字
        Task threadInitCallback_;//线程初始化化回调函数

        boost::ptr_vector<muduo::Thread> threads_;//线程队列，利用指针容器实现
        std::deque<Task> queue_;    //双向队列
        size_t maxQueueSize_;       //最大队列容量
        bool running_;              //线程池是否运行
    };

}

#endif
