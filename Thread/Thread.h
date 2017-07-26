// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "Atomic.h"
#include "Types.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <pthread.h>

namespace muduo
{

class Thread : boost::noncopyable
{
 public:
  typedef boost::function<void ()> ThreadFunc;

  explicit Thread(const ThreadFunc&, const string& name = string());
#ifdef __GXX_EXPERIMENTAL_CXX0X__
  explicit Thread(ThreadFunc&&, const string& name = string());
#endif
  ~Thread();

  void start();//开始
  int join(); // return pthread_join() 阻塞

  bool started() const { return started_; }//返回是否开始
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return *tid_; }//
  const string& name() const { return name_; }//返回线程名

  static int numCreated() { return numCreated_.get(); }//初始化numCreated

 private:
  void setDefaultName();//设置默认名

  bool       started_;//是否开始运行
  bool       joined_; //是否阻塞
  pthread_t  pthreadId_;//线程ID
  boost::shared_ptr<pid_t> tid_;//CurrentThread中的ID
  ThreadFunc func_;//线程函数
  string     name_;//线程名

  static AtomicInt32 numCreated_;//初始化32位原子整数
};

}
#endif
