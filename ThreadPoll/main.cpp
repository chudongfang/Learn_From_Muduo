#include "ThreadPool.h"
#include "CountDownLatch.h"

#include "CurrentThread.h"


//#include "Logging.h"

#include <boost/bind.hpp>
#include <stdio.h>

void print()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string& str)
{
//    LOG_INFO << str;
    printf("%s\n",str.c_str());
    usleep(100*1000);
}
void print_pid()
{
        printf("回调函数：tid=%d\n",::getpid());
}
void test(int maxSize)
{
//    LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
    muduo::ThreadPool pool("MainThreadPool");
    pool.setMaxQueueSize(maxSize);
    //设置回调函数
    pool.setThreadInitCallback(print_pid);
    pool.start(5);
//    LOG_WARN << "Adding";
    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(boost::bind(printString, std::string(buf)));
    }
//    LOG_WARN << "Done";
    //设置倒计时
    muduo::CountDownLatch latch(1);
    pool.run(boost::bind(&muduo::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
}

int main()
{
    //test(0);
    //test(1);
    //test(5);
    test(10);
    //test(-2);
}
