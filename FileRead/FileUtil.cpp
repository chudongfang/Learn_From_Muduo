// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
#include "FileUtil.h"
//#include <muduo/base/Logging.h> // strerror_tl
#include <boost/static_assert.hpp>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
using namespace muduo;


//构造函数
FileUtil::AppendFile::AppendFile(StringArg filename)
        : fp_(::fopen(filename.c_str(), "ae")),  // 'e' for O_CLOEXEC
          writtenBytes_(0)
{
    assert(fp_);
    //设置缓冲区
    ::setbuffer(fp_, buffer_, sizeof buffer_);
    // posix_fadvise POSIX_FADV_DONTNEED ?
}
//析构函数
FileUtil::AppendFile::~AppendFile()
{
    //关闭文件
    ::fclose(fp_);
}


//向文件中写长度len的logline
void FileUtil::AppendFile::append(const char* logline, const size_t len)
{
    //写入文件
    size_t n = write(logline, len);
    size_t remain = len - n;
    while (remain > 0)
    {
        size_t x = write(logline + n, remain);
        //如果写入失败，报错
        if (x == 0)
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "AppendFile::append() failed %s\n", stdin);
            }
            break;
        }
        //n代表已经写入的数
        n += x;
        //remain代表还需写入的数
        remain = len - n; // remain -= x
    }
    //已经写入文件的总字节数
    writtenBytes_ += len;
}


//刷新文件流
void FileUtil::AppendFile::flush()
{
    ::fflush(fp_);
}


size_t FileUtil::AppendFile::write(const char* logline, size_t len)
{
    // #undef fwrite_unlocked
    //写文件的不加锁的版本，线程不安全
    return ::fwrite_unlocked(logline, 1, len, fp_);
}


//读文件构造函数，在此期间打开文件，并记录文件描述符
FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
        : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
          err_(0)
{
    //清空缓冲区
    buf_[0] = '\0';
    if (fd_ < 0)
    {
        err_ = errno;
    }
}


//析构函数，关闭文件描述符
FileUtil::ReadSmallFile::~ReadSmallFile()
{
    if (fd_ >= 0)
    {
        ::close(fd_); // FIXME: check EINTR
    }
}

// return errno
// 获取文件信息，并把文件内容读入到content中
template<typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                          String* content,
                                          int64_t* fileSize,
                                          int64_t* modifyTime,
                                          int64_t* createTime)
{
    //编译期间断言  long int 是否占八位
    BOOST_STATIC_ASSERT(sizeof(off_t) == 8);
    assert(content != NULL);
    int err = err_;
    if (fd_ >= 0)
    {
        //清空
        content->clear();
        if (fileSize)
        {
            //获取文件信息到statbut
            struct stat statbuf;
            if (::fstat(fd_, &statbuf) == 0)
            {
                //是否是常规文件
                if (S_ISREG(statbuf.st_mode))
                {
                    *fileSize = statbuf.st_size;
                    //申请空间
                    content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
                }
                    //是否是一个目录
                else if (S_ISDIR(statbuf.st_mode))
                {
                    err = EISDIR;
                }
                //或取文件内容最后被修改的时间
                if (modifyTime)
                {
                    *modifyTime = statbuf.st_mtime;
                }
                //获取文件状态（属性）改变时间
                if (createTime)
                {
                    *createTime = statbuf.st_ctime;
                }
            }
            else
            {
                err = errno;
            }
        }
        while (content->size() < implicit_cast<size_t>(maxSize))
        {
            //确定要读的字节数
            size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
            //long
            ssize_t n = ::read(fd_, buf_, toRead);
            if (n > 0)
            {
                content->append(buf_, n);
            }
            else
            {
                if (n < 0)
                {
                    err = errno;
                }
                break;
            }
        }
    }
    return err;
}

//把文件中的内容读进缓冲区，并给size赋值为文件的大小
int FileUtil::ReadSmallFile::readToBuffer(int* size)
{
    int err = err_;
    if (fd_ >= 0)
    {
        //pread()  reads  up to count bytes from file descriptor fd at offset offset (from the start of the
        //file) into the buffer starting at buf.  The file offset is not changed.
        ssize_t n = ::pread(fd_, buf_, sizeof(buf_)-1, 0);
        if (n >= 0)
        {
            if (size)
            {
                *size = static_cast<int>(n);
            }
            buf_[n] = '\0';
        }
        else
        {
            err = errno;
        }
    }
    return err;
}
template int FileUtil::readFile(StringArg filename,
                                int maxSize,
                                string* content,
                                int64_t*, int64_t*, int64_t*);
template int FileUtil::ReadSmallFile::readToString(
        int maxSize,
        string* content,
        int64_t*, int64_t*, int64_t*);
#ifndef MUDUO_STD_STRING
template int FileUtil::readFile(StringArg filename,
                                int maxSize,
                                std::string* content,
                                int64_t*, int64_t*, int64_t*);
template int FileUtil::ReadSmallFile::readToString(
        int maxSize,
        std::string* content,
        int64_t*, int64_t*, int64_t*);
#endif
