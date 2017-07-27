// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/weak_ptr.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{
    namespace CurrentThread
    {
        //线程id
        __thread int t_cachedTid = 0;


        //线程名
        __thread char t_tidString[32];
        __thread int t_tidStringLength = 6;

        __thread const char* t_threadName = "unknown";

        //判断int和pid_t类型是否相同
        const bool sameType = boost::is_same<int, pid_t>::value;
        //如果类型不相同则断言false，编译出错  ？？
        BOOST_STATIC_ASSERT(sameType);
    }

    namespace detail
    {
//通过系统调用syscall获取当前线程
        pid_t gettid()
        {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }
//更新当前线程信息
        void afterFork()
        {
            muduo::CurrentThread::t_cachedTid = 0;
            muduo::CurrentThread::t_threadName = "main";
            CurrentThread::tid();
            // no need to call pthread_atfork(NULL, NULL, &afterFork);
        }

// 该类用于初始化CurrentThread信息为当先主线程信息，并且在创建子线程时更新CurrentThread信息
        class ThreadNameInitializer
        {
        public:
            ThreadNameInitializer()
            {
                muduo::CurrentThread::t_threadName = "main";
                //获取当前ID信息
                CurrentThread::tid();
                //子进程执行afterfork
                //pthread_atfork(fork前函数，父进程执行的函数，子线程执行的函数)
                pthread_atfork(NULL, NULL, &afterFork);
            }
        };

//初始CurrentThread信息
        ThreadNameInitializer init;
//以ThreadData的数据形式向子线程传参
        struct ThreadData
        {
            typedef muduo::Thread::ThreadFunc ThreadFunc;
            ThreadFunc func_;
            string name_;
            //智能指针，弱引用，不对引用计数器加一
            //用来观察
            boost::weak_ptr<pid_t> wkTid_;

            ThreadData(const ThreadFunc& func,
                       const string& name,
                       const boost::shared_ptr<pid_t>& tid)
                    : func_(func),
                      name_(name),
                      wkTid_(tid)
            { }

            void runInThread()
            {

                //得到当前线程ID
                pid_t tid = muduo::CurrentThread::tid();

                //获取所管理的对象的强引用指针。
                boost::shared_ptr<pid_t> ptid = wkTid_.lock();
                //修改pid_为当前进程ID
                if (ptid)
                {
                    *ptid = tid;
                    ptid.reset();
                }

                muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
                //设置进程名
                ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
                try
                {
                    //运行函数
                    func_();
                    //运行完成修改CurrentThread中线程名
                    muduo::CurrentThread::t_threadName = "finished";
                }
                catch (const Exception& ex)
                {
                    //对于其抛出的异常类，显示erro
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
                    abort();
                }
                catch (const std::exception& ex)
                {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    abort();
                }
                catch (...)
                {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                    throw; // rethrow
                }
            }
        };
//子线程函数
//开始线程
        void* startThread(void* obj)
        {
            ThreadData* data = static_cast<ThreadData*>(obj);
            data->runInThread();
            delete data;
            return NULL;
        }

    }
}

using namespace muduo;
//获取当前线程ID，并赋值给变量
void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}
//判断是否是主线程
bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}


//调用系统调用实现延时
/*
void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL);
}
*/




AtomicInt32 Thread::numCreated_;

//主线程函数
Thread::Thread(const ThreadFunc& func, const string& n)//构造函数，初始化线程
        : started_(false),
          joined_(false),
          pthreadId_(0),
          tid_(new pid_t(0)),
          func_(func),
          name_(n)
{
    setDefaultName();
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
Thread::Thread(ThreadFunc&& func, const string& n)
        : started_(false),
          joined_(false),
          pthreadId_(0),
          tid_(new pid_t(0)),
          func_(std::move(func)),
          name_(n)
{
    setDefaultName();
}

#endif

//主线程函数
Thread::~Thread()//如何子线程未完成，将其与父线程分离
{
    if (started_ && !joined_)
    {
        pthread_detach(pthreadId_);
    }
}


//主线程函数
//给线程对象命名
void Thread::setDefaultName()
{
    int num = numCreated_.incrementAndGet();//原子操作
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

//主线程函数
void Thread::start()//开启线程
{
    assert(!started_);
    started_ = true;
    // FIXME: move(func_)
    detail::ThreadData* data = new detail::ThreadData(func_, name_, tid_);
    if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
    {
        started_ = false;
        delete data; // or no delete?
        //LOG_SYSFATAL << "Failed in pthread_create";

    }
}

//主线程函数
int Thread::join()//线程阻塞
{
    assert(started_);
    //assert的作用是现计算表达式 expression ，如果其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

