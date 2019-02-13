// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;


// 构造函数的话首先是打开一个文件

//   open()要求的是一个char*字符串 将string类型的filename 转换为char*
AppendFile::AppendFile(string filename)
:   fp_(fopen(filename.c_str(), "ae"))
{
    // 用户提供缓冲区
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
    //关闭文件
    fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len)
{
    size_t n = this->write(logline, len);
    size_t remain = len - n;
    // 如果一次读不完，就可以分为两次读取操作
    while (remain > 0)
    {
        size_t x = this->write(logline + n, remain);
        if (x == 0)
        {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        n += x;
        remain = len - n;
    }
}

void AppendFile::flush()
{
    fflush(fp_); // 将一些一些内容
}


//
size_t AppendFile::write(const char* logline, size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);
}