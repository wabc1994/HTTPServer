// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "noncopyable.h"
#include <assert.h>
#include <string.h>
#include <string>


class AsyncLogging;
const int kSmallBuffer = 4000;   //4M
const int kLargeBuffer = 4000 * 1000;

template<int SIZE>

// 固定大小缓冲区
class FixedBuffer: noncopyable
{
public:
    FixedBuffer()
    :   cur_(data_)
    { }

    ~FixedBuffer()
    { }

    void append(const char* buf, size_t len)
    {
        if (avail() > static_cast<int>(len))
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }//已使用的缓冲区大小

    char* current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    void bzero() { memset(data_, 0, sizeof data_); }


private:
    const char* end() const { return data_ + sizeof data_; }

    char data_[SIZE];   // 字符数组
    char* cur_;   // 当前的缓存区已有数据大小
};



class LogStream : noncopyable
{
public:
    /* 缓冲区的类型，是个固定大小的缓冲区，由字符数组实现 */
    typedef FixedBuffer<kSmallBuffer> Buffer;

    LogStream& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }
    /* 重载的operator<<函数，将日志信息存放在缓冲区中 */
    LogStream& operator<<(const char* str)
    {
        if (str)
            buffer_.append(str, strlen(str));
        else
            buffer_.append("(null)", 6);
        return *this;
    }

    LogStream& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    LogStream& operator<<(const std::string& v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck();

    template<typename T>
    void formatInteger(T);

    /* 用于存储日志信息的缓冲区 */
    Buffer buffer_;

    static const int kMaxNumericSize = 32;
};