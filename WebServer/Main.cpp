#include "EventLoop.h"
#include "Server.h"
#include "base/Logging.h"
#include <getopt.h>
#include <string>

int main(int argc, char *argv[])
{
    int threadNum = 4;
    int port = 80;
    std::string logPath = "./WebServer.log";

    // parse args
    int opt;
    const char *str = "t:l:p:";
    while ((opt = getopt(argc, argv, str))!= -1)
    {
        switch (opt)
        {
            case 't':
            {
                threadNum = atoi(optarg);
                break;
            }
            case 'l':
            {
                logPath = optarg;
                if (logPath.size() < 2 || optarg[0] != '/')
                {
                    printf("logPath should start with \"/\"\n");
                    abort();
                }
                break;
            }
            case 'p':
            {
                port = atoi(optarg);
                break;
            }
            default: break;
        }
    }
    Logger::setLogFileName(logPath);
    // STL库在多线程上应用
    #ifndef _PTHREADS
        LOG << "_PTHREADS is not defined !";
    #endif
    EventLoop mainLoop;
    // 主线程是刚开始就要自己创建，
    Server myHTTPServer(&mainLoop, threadNum, port);
    myHTTPServer.start();

    // 子反应器的线程启动必须的条件必须是主反应器已经启动好，并且已经在运行当中了， mainLoop.loop()
    // 该种情况
    mainLoop.loop();
    return 0;
}
