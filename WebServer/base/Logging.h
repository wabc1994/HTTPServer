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
#define LOG Logger(__FILE__, __LINE__).stream()