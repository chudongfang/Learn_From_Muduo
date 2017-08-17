// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H
#include "copyable.h"
#include "StringPiece.h"
#include "Types.h"
#include "Endian.h"
#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>
//#include <unistd.h>  // ssize_t
namespace muduo
{
    namespace net
    {
/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
        class Buffer : public muduo::copyable
        {
        public:
            // kCheapPrepend 和 kInitialSize，定义了 prependable 的初始大小和 writable 的初始大小
            static const size_t kCheapPrepend = 8;
            static const size_t kInitialSize = 1024;
            //初始化指针位置
            explicit Buffer(size_t initialSize = kInitialSize)
                    : buffer_(kCheapPrepend + initialSize),
                      readerIndex_(kCheapPrepend),
                      writerIndex_(kCheapPrepend)
            {
                assert(readableBytes() == 0);
                assert(writableBytes() == initialSize);
                assert(prependableBytes() == kCheapPrepend);
            }
            // implicit copy-ctor, move-ctor, dtor and assignment are fine
            // NOTE: implicit move-ctor is added in g++ 4.6
            // swap Buffer
            void swap(Buffer& rhs)
            {
                buffer_.swap(rhs.buffer_);
                std::swap(readerIndex_, rhs.readerIndex_);
                std::swap(writerIndex_, rhs.writerIndex_);
            }
            //返回可读的字节数
            size_t readableBytes() const
            { return writerIndex_ - readerIndex_; }
            //返回可写的字节数
            size_t writableBytes() const
            { return buffer_.size() - writerIndex_; }
            //返回已读的数目
            size_t prependableBytes() const
            { return readerIndex_; }

            //返回读指针
            const char* peek() const
            { return begin() + readerIndex_; }

            //在写入的数据中从头寻找crlf串
            const char* findCRLF() const
            {
                // FIXME: replace with memmem()?
                const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
                return crlf == beginWrite() ? NULL : crlf;
            }
            //在写入的数据中从start寻找crlf串
            const char* findCRLF(const char* start) const
            {
                assert(peek() <= start);
                assert(start <= beginWrite());
                // FIXME: replace with memmem()?
                const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
                return crlf == beginWrite() ? NULL : crlf;
            }


            //memchr
            //
            //Searches within the first num bytes of the block of memory
            //pointed by ptr for the first occurrence of value (interpreted as an unsigned char),
            //and returns a pointer to it.
            //在写入的数据中从头寻找第一个出现的'\n'
            const char* findEOL() const
            {
                const void* eol = memchr(peek(), '\n', readableBytes());
                return static_cast<const char*>(eol);
            }

            //在写入的数据中从start寻找第一个出现的'\n'
            const char* findEOL(const char* start) const
            {
                assert(peek() <= start);
                assert(start <= beginWrite());
                const void* eol = memchr(start, '\n', beginWrite() - start);
                return static_cast<const char*>(eol);
            }

            // retrieve returns void, to prevent
            // string str(retrieve(readableBytes()), readableBytes());
            // the evaluation of two functions are unspecified
            //读指针后移len位，如果大于可读，则重新初始化buffer指针位置`
            void retrieve(size_t len)
            {
                assert(len <= readableBytes());
                if (len < readableBytes())
                {
                    readerIndex_ += len;
                }
                else
                {
                    retrieveAll();
                }
            }

            //移动读指针到end
            void retrieveUntil(const char* end)
            {
                assert(peek() <= end);
                assert(end <= beginWrite());
                retrieve(end - peek());
            }

            //读指针后移一个64位的数
            void retrieveInt64()
            {
                retrieve(sizeof(int64_t));
            }

            //读指针后移一个32位的数
            void retrieveInt32()
            {
                retrieve(sizeof(int32_t));
            }

            //读指针后移一个16位的数
            void retrieveInt16()
            {
                retrieve(sizeof(int16_t));
            }

            //读指针后移一个8位的数
            void retrieveInt8()
            {
                retrieve(sizeof(int8_t));
            }

            //初始化指针位置
            void retrieveAll()
            {
                readerIndex_ = kCheapPrepend;
                writerIndex_ = kCheapPrepend;
            }

            //读完所有内容到string中，并返回
            string retrieveAllAsString()
            {
                return retrieveAsString(readableBytes());
            }

            //读取长度为len的字符串，并返回
            string retrieveAsString(size_t len)
            {
                assert(len <= readableBytes());
                string result(peek(), len);
                retrieve(len);
                return result;
            }
            StringPiece toStringPiece() const
            {
                return StringPiece(peek(), static_cast<int>(readableBytes()));
            }
            void append(const StringPiece& str)
            {
                append(str.data(), str.size());
            }

            // the functions copy
            //
            //template<class InputIterator, class OutputIterator>
            //OutputIterator copy ( InputIterator first, InputIterator last, OutputIterator result )
            //{
            //    while (first!=last) *result++ = *first++;
            //      return result;
            //}

            //首先判断其是否有足够空间，然后copy ，最后更新指针
            void append(const char* /*restrict*/ data, size_t len)
            {
                ensureWritableBytes(len);
                std::copy(data, data+len, beginWrite());
                hasWritten(len);
            }

            void append(const void* /*restrict*/ data, size_t len)
            {
                append(static_cast<const char*>(data), len);
            }

            //确认可写的字节数满足len个字节数的要求，如果len大于其可写字节数，则分配内存
            void ensureWritableBytes(size_t len)
            {
                if (writableBytes() < len)
                {
                    makeSpace(len);
                }
                assert(writableBytes() >= len);
            }
            //返回写指针
            char* beginWrite()
            { return begin() + writerIndex_; }
            //返回写指针
            const char* beginWrite() const
            { return begin() + writerIndex_; }

            //把写指针向后移动len
            void hasWritten(size_t len)
            {
                assert(len <= writableBytes());
                writerIndex_ += len;
            }
            //把写指针向前移动len
            void unwrite(size_t len)
            {
                assert(len <= readableBytes());
                writerIndex_ -= len;
            }
            ///
            /// Append int64_t using network endian
            ///

            //写入一个64位的数
            void appendInt64(int64_t x)
            {
                int64_t be64 = sockets::hostToNetwork64(x);
                append(&be64, sizeof be64);
            }
            ///
            /// Append int32_t using network endian
            ///

            //写入一个32位的数
            void appendInt32(int32_t x)
            {
                int32_t be32 = sockets::hostToNetwork32(x);
                append(&be32, sizeof be32);
            }
            //写入一个16位的数
            void appendInt16(int16_t x)
            {
                int16_t be16 = sockets::hostToNetwork16(x);
                append(&be16, sizeof be16);
            }
            //写入一个16位的数
            void appendInt8(int8_t x)
            {
                append(&x, sizeof x);
            }
            ///
            /// Read int64_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            //读取一个64位的数
            int64_t readInt64()
            {
                int64_t result = peekInt64();
                retrieveInt64();
                return result;
            }
            ///
            /// Read int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            //读取一个32位的数
            int32_t readInt32()
            {
                int32_t result = peekInt32();
                retrieveInt32();
                return result;
            }
            //读取一个16位的数
            int16_t readInt16()
            {
                int16_t result = peekInt16();
                retrieveInt16();
                return result;
            }
            //读取一个8位的数
            int8_t readInt8()
            {
                int8_t result = peekInt8();
                retrieveInt8();
                return result;
            }
            ///
            /// Peek int64_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int64_t)
            //读取并返回一个Int64
            int64_t peekInt64() const
            {
                assert(readableBytes() >= sizeof(int64_t));
                int64_t be64 = 0;
                ::memcpy(&be64, peek(), sizeof be64);
                return sockets::networkToHost64(be64);
            }

            ///
            /// Peek int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            //读取并返回一个Int32
            int32_t peekInt32() const
            {
                assert(readableBytes() >= sizeof(int32_t));
                int32_t be32 = 0;
                ::memcpy(&be32, peek(), sizeof be32);
                return sockets::networkToHost32(be32);
            }
            //读取并返回一个Int16
            int16_t peekInt16() const
            {
                assert(readableBytes() >= sizeof(int16_t));
                int16_t be16 = 0;
                ::memcpy(&be16, peek(), sizeof be16);
                return sockets::networkToHost16(be16);
            }
            //读取并返回一个Int8
            int8_t peekInt8() const
            {
                assert(readableBytes() >= sizeof(int8_t));
                int8_t x = *peek();
                return x;
            }
            ///
            /// Prepend int64_t using network endian
            ///
            //将int64放入prepend
            void prependInt64(int64_t x)
            {
                int64_t be64 = sockets::hostToNetwork64(x);
                prepend(&be64, sizeof be64);
            }
            ///
            /// Prepend int32_t using network endian
            ///

            //将int32放入prepend
            void prependInt32(int32_t x)
            {
                int32_t be32 = sockets::hostToNetwork32(x);
                prepend(&be32, sizeof be32);
            }
            //将int16放入prepend
            void prependInt16(int16_t x)
            {
                int16_t be16 = sockets::hostToNetwork16(x);
                prepend(&be16, sizeof be16);
            }

            //将int8放入prepend
            void prependInt8(int8_t x)
            {
                prepend(&x, sizeof x);
            }

            //将长度为len的data放入到prepend中
            void prepend(const void* /*restrict*/ data, size_t len)
            {
                assert(len <= prependableBytes());
                readerIndex_ -= len;
                const char* d = static_cast<const char*>(data);
                std::copy(d, d+len, begin()+readerIndex_);
            }

            //Requests the container to reduce its capacity to fit its size.
            void shrink(size_t reserve)
            {
                // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
                Buffer other;
                other.ensureWritableBytes(readableBytes()+reserve);
                other.append(toStringPiece());
                swap(other);
            }

            //返回当前buffer分配的内存可以容纳的字符数
            size_t internalCapacity() const
            {
                return buffer_.capacity();
            }
            /// Read data directly into buffer.
            ///
            /// It may implement with readv(2)
            /// @return result of read(2), @c errno is saved
//            ssize_t readFd(int fd, int* savedErrno);


        private:
            //返回缓冲区开始时的指针
            char* begin()
            { return  &*buffer_.begin(); }
            const char* begin() const
            { return &*buffer_.begin(); }

            //当内存不够用时，重新分配内存
            void makeSpace(size_t len)
            {
                if (writableBytes() + prependableBytes() < len + kCheapPrepend)
                {
                    // FIXME: move readable data
                    //分配足够多的内存
                    buffer_.resize(writerIndex_+len);
                }
                else
                {
                    // move readable data to the front, make space inside buffer
                    assert(kCheapPrepend < readerIndex_);
                    //先把已有的数据移到前面去，腾出 writable 空间
                    size_t readable = readableBytes();
                    std::copy(begin()+readerIndex_,
                              begin()+writerIndex_,
                              begin()+kCheapPrepend);
                    readerIndex_ = kCheapPrepend;
                    writerIndex_ = readerIndex_ + readable;
                    assert(readable == readableBytes());
                }
            }
        private:
            std::vector<char> buffer_;
            size_t readerIndex_;
            size_t writerIndex_;
            static const char kCRLF[];
        };
    }
}
#endif  // MUDUO_NET_BUFFER_H
