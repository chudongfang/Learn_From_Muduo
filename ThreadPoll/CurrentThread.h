// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include <stdint.h>

namespace muduo
{
    namespace CurrentThread
    {
        // internal
        //记录当前线程信息
        extern __thread int t_cachedTid;
        extern __thread char t_tidString[32];
        extern __thread int t_tidStringLength;
        extern __thread const char* t_threadName;
        void cacheTid();

        //得到当前线程ID
        inline int tid()
        {
            //性能优化，t_cachedTid==0 的情况很少发生。
            if (__builtin_expect(t_cachedTid == 0, 0))
            {
                cacheTid();
            }
            return t_cachedTid;
        }

        inline const char* tidString() // for logging
        {
            return t_tidString;
        }

        inline int tidStringLength() // for logging
        {
            return t_tidStringLength;
        }
        //记录线程名字
        inline const char* name()
        {
            return t_threadName;
        }
        //判断是否是主线程
        bool isMainThread();

        void sleepUsec(int64_t usec);
    }
}

#endif
