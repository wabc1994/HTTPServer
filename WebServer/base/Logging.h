// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "LogStream.h"
#include <pthread.h>
#include <string.h>
#include <string>
#include <stdio.h>

class AsyncLogging;


class Logger
{
public:
    Logger(const char *fileName, int line);
    ~Logger();
    LogStream& stream() { return impl_.stream_; }

    static void setLogFileName(std::string fileName)
    {
        logFileName_ = fileName;
    }
    static std::string getLogFileName()
    {
        return logFileName_;
    }

private:
    class Impl
    {
    public:
        Impl(const char *fileName, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string basename_;
    };
    Impl impl_;
    static std::string logFileName_;
};


// 宏定义 使用方式LOG << "打印日志消息"

/* 将日志信息存在缓冲区中，使用LOG_WARN是会返回Logger().stream()，就是返回这个LogStream */
//LogStream stream_; LogStream封装Buffer


// 提供给外面的接口形式

#define LOG Logger(__FILE__, __LINE__).stream()


/*
 * __FILE__:返回所在文件名
 * __LINE__:返回所在行数
 * __func__:返回所在函数名
 *
 * 这些都是无名对象，当使用LOG_* << "***"时，
 * 1.构造Logger类型的临时对象，返回LogStream类型变量
 * 2.调用LogStream重载的operator<<操作符，将数据写入到LogStream的Buffer中
 * 3.当前语句结束，Logger临时对象析构，调用Logger析构函数，将LogStream中的数据输出
 */

// 宏定义，在编译期间获取 文件的名字和行号、函数名等信息