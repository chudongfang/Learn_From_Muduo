// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "ThreadPool.h"

#include "Exception.h"

#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>

using namespace muduo;


//构造函数
ThreadPool::ThreadPool(const string& nameArg)
        : mutex_(),//初始化锁
          notEmpty_(mutex_),//初始化条件变量
          notFull_(mutex_),
          name_(nameArg),  //初始化线程池名
          maxQueueSize_(0),
          running_(false)
{
}
//析构函数，停止线程池
ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
    running_ = true;
    //设置指针容器大小
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof id, "%d", i+1);
        //新建立进程，并用bind绑定函数进行传参
        threads_.push_back(new muduo::Thread(
                boost::bind(&ThreadPool::runInThread, this), name_+id));
        //运行线程
        threads_[i].start();
    }
    //初始化回调函数
    if (numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop()
{
    {
        //加互斥锁，当其出当前{}后会自动析构，并解锁
        MutexLockGuard lock(mutex_);
        running_ = false;
        //以广播的形式唤醒等待notEmpty条件变量的线程
        notEmpty_.notifyAll();
    }
    //遍历这个容器，并在每个线程中执行join函数，对线程进程阻塞
    for_each(threads_.begin(),
             threads_.end(),
             boost::bind(&muduo::Thread::join, _1));
}
//返回队列长度
size_t ThreadPool::queueSize() const
{
    //MutexLockGuard类对mutex_加锁，当函数结束时，其会自动利用析构函数解锁
    MutexLockGuard lock(mutex_);
    return queue_.size();
}


//主线程函数
//不断添加task到task队列
void ThreadPool::run(const Task& task)
{
    //如果线程池中没有线程，由主线程执行task
    if (threads_.empty())
    {
        task();
    }
    else
    {
        MutexLockGuard lock(mutex_);
        //如果线程池满，则阻塞等待子线程对task进行处理
        while (isFull())
        {
            notFull_.wait();
        }
        assert(!isFull());
        
        //向线程队列中添加task
        queue_.push_back(task);
        //唤醒子线程处理task
        notEmpty_.notify();
    }
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
void ThreadPool::run(Task&& task)
{
    if (threads_.empty())
    {
        task();
    }
    else
    {
        MutexLockGuard lock(mutex_);
        while (isFull())
        {
            notFull_.wait();
        }
        assert(!isFull());

        queue_.push_back(std::move(task));
        notEmpty_.notify();
    }
}
#endif

//子线程函数
//从任务队列（queue）中取task
ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup

    //子进程等待任务队列中的task
    while (queue_.empty() && running_)
    {
        notEmpty_.wait();
    }
    Task task;

    //从任务队列中取task
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        //唤醒主进程放入task，保证了task及时被放入
        if (maxQueueSize_ > 0)
        {
            notFull_.notify();
        }
    }
    return task;
}

//判断队列是否满
bool ThreadPool::isFull() const
{
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

//子线程函数
//利用回调函数初始化线程
//从任务队列中取task并执行
//抛出异常
void ThreadPool::runInThread()
{
    try
    {
        if (threadInitCallback_)
        {
            threadInitCallback_();
        }
        while (running_)
        {
            Task task(take());
            if (task)
            {
                task();
            }
        }
    }
    catch (const Exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

