// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

//提供原子操作


namespace muduo
{

namespace detail
{
template<typename T>
class AtomicIntegerT : boost::noncopyable
{
 public:
  //构造函数，初始化value
  AtomicIntegerT()
    : value_(0)
  {
  }
  //作者给了复制构造函数的原型
  // uncomment if you need copying and assignment
  //
  // AtomicIntegerT(const AtomicIntegerT& that)
  //   : value_(that.get())
  // {}
  //
  // AtomicIntegerT& operator=(const AtomicIntegerT& that)
  // {
  //   getAndSet(that.get());
  //   return *this;
  // }
  //

  //{ if (*ptr == oldval) { *ptr = newval; } return oldval; }
  //返回0
  T get()
  {
    // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  //返回value更新前的值，即返回value
  T getAndAdd(T x)
  {
    // in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
    return __sync_fetch_and_add(&value_, x);
  }

  //value 加 x
  T addAndGet(T x)
  {
    return getAndAdd(x) + x;
  }
  //value 加 1
  T incrementAndGet()
  {
    return addAndGet(1);
  }
  //value 减 1
  T decrementAndGet()
  {
    return addAndGet(-1);
  }
  //value 加 x
  void add(T x)
  {
    getAndAdd(x);
  }
  //value 加 1
  void increment()
  {
    incrementAndGet();
  }
  //value 加 1
  void decrement()
  {
    decrementAndGet();
  }

  //将value设置为newValie，返回操作前value的值
  T getAndSet(T newValue)
  {
    // in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
    return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
  volatile T value_;
};
}

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;//声明32位整数
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;//声明64位整数
}

#endif  // MUDUO_BASE_ATOMIC_H
