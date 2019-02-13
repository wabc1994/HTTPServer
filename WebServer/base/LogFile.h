// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"
#include <memory>
#include <string>

// TODO 提供自动归档功能

//LogFile类负责日志的滚动，例如日大小达到指定大小、到达某一个时间点都会新建一个日志。
//日志的名字为：日志名+日期+时间+主机名+线程ID+.log
class LogFile : noncopyable
{
public:
    // 每被append flushEveryN次，flush一下，会往文件写，只不过，文件也是带缓冲区的
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);

    const std::string basename_;    //文件名字
    const int flushEveryN_;   ;//写入日志间隔

    int count_;   //写入日志次数，配合checkEveryN_
    std::unique_ptr<MutexLock> mutex_;  //对一个锁的基本封装
    std::unique_ptr<AppendFile> file_;  //LogFile还是对AppendFile的封装
};