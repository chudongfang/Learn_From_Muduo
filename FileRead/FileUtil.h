// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_BASE_FILEUTIL_H
#define MUDUO_BASE_FILEUTIL_H

#include "StringPiece.h"
#include <boost/noncopyable.hpp>

namespace muduo
{

namespace FileUtil
{

//用来读取小文件
// read small file < 64KB
class ReadSmallFile : boost::noncopyable
{
 public:
  //构造函数
  ReadSmallFile(StringArg filename);
  ~ReadSmallFile();

  // return errno
  //读文件到缓冲区，并获取文件信息
  template<typename String>
  int readToString(int maxSize,
                   String* content,
                   int64_t* fileSize,
                   int64_t* modifyTime,
                   int64_t* createTime);

  /// Read at maxium kBufferSize into buf_
  // return errno
  //读文件到缓冲区，并将size值置为文件大小
  int readToBuffer(int* size);

  //返回缓冲区指针，不能通过其对数据进行修改
  const char* buffer() const { return buf_; }
  //设置缓冲区大小
  static const int kBufferSize = 64*1024;

 private:
  //文件描述符
  int fd_;
  int err_;
  //缓冲区
  char buf_[kBufferSize];
};
//读文件内容，并且获取文件信息
// read the file content, returns errno if error happens.
template<typename String>
int readFile(StringArg filename,
             int maxSize,
             String* content,
             int64_t* fileSize = NULL,
             int64_t* modifyTime = NULL,
             int64_t* createTime = NULL)
{
  ReadSmallFile file(filename);
  return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

//
// not thread safe
class AppendFile : boost::noncopyable
{
 public:
  explicit AppendFile(StringArg filename);

  ~AppendFile();

  //向文件追加长度n的logline
  void append(const char* logline, const size_t len);
  //清空文件流
  void flush();
  //返回已经写入的字节数
  size_t writtenBytes() const { return writtenBytes_; }

 private:
  //线程不安全的写文件函数
  size_t write(const char* logline, size_t len);

  FILE* fp_;//文件结构体
  //缓冲区
  char buffer_[64*1024];
  //要写的数据量
  size_t writtenBytes_;
};
}

}

#endif  // MUDUO_BASE_FILEUTIL_H

