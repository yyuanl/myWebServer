#ifndef __LOG_H__
#define __LOG_H__
#include <iostream>
#include <stdio.h>
#include "./blockQueue.h"
using namespace std;
class Log{
public:
    static Log *getInstance(){//单例模式，外部创建日志实例唯一接口
        static Log instance;
        return &instance;
    }
private:  //保证单例模式
    Log();
    Log(const Log &s){}
    Log& operator=(const Log &s){return *this;}
public:
    bool init(const char *fileName,int logBufSize = 8192,int logLines=5000000, int maxQueueSize = 0);
    void writeLog(int level, const char *format, ...);
    void flush(void);
    static void *flushLogThread(void *args); // 异步写入日志的线程函数
private:
    virtual ~Log();
    void *asyncWriteLog(); //异步写入日志方法
private:
    char dirName[128]; //日志目录名
    char logName[128]; //log文件名
    int maxLines; //日志对打行数
    int logBufferSize;  //输出内容大小
    long long count;  //日志行数
    int today; //当前时间？
    FILE *fp; //打开的文件指针,往文件中写日志文本内容
    char *buf; //输出内容缓冲数组
    blockQueue<string> *logQueue; //组塞队列，用于异步写入日志
    bool async;  //是否异步
    locker m_mutex;

};

#define LOG_DEBUG(format, ...) Log::getInstance()->writeLog(0,format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::getInstance()->writeLog(1,format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::getInstance()->writeLog(2,format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::getInstance()->writeLog(3,format, ##__VA_ARGS__)  
#endif