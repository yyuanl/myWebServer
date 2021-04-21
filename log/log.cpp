#include "./log.h"
#include "./blockQueue.h"
#include <string.h>
#include <string>
#include <sys/time.h>
#include <stdarg.h>
#include <iostream>
#define YYL_DEBUG  //代码调试开关
using namespace std;
Log::Log(){
    count = 0;
    async = false;
}
bool Log::init(const char *fileName,int logBufSize,int logLines, int maxQueueSize){
    if(maxQueueSize >=1 ){
        async = true;
        logQueue = new blockQueue<string>(maxQueueSize);
        pthread_t td;
        pthread_create(&td, NULL, flushLogThread,NULL); // 开异步线程完成日志写入任务
    }
    logBufferSize = logBufSize;
    buf = new char[logBufferSize];
    memset(buf, '\0', logBufferSize);
    maxLines = logLines;

    time_t t = time(NULL); //time_t 时间类
    tm *systm = localtime(&t); // tm时间结构体，包含年月日等
    tm mytm = *systm;

    const char *p = strrchr(fileName, '/');
    char logFullName[256] = {0};  //日志文件完整名：（目录名）年_月_日文件名
    
    if(p == NULL){  //传入文件名
        snprintf(logFullName,
                255, 
                "%d_%02d_%02d_%s", 
                mytm.tm_year + 1900,
                mytm.tm_mon + 1,
                mytm.tm_mday,
                fileName);
    }else{ // 传入完整文件路径
        strcpy(logName,p + 1); // 初始化文件名
        strncpy(dirName,fileName,p - fileName + 1); // 初始化目录名
        snprintf(logFullName,
                255, 
                "%s%d_%02d_%02d_%s", 
                dirName,
                mytm.tm_year + 1900,
                mytm.tm_mon + 1,
                mytm.tm_mday,
                logName);
    }

    today = mytm.tm_mday; // 初始化天
    fp = fopen(logFullName, "a");// 打开日志文件，初始化文件指针
    if(fp == NULL){
        return false;
    }
    return true;
}
void level2Str(char *str,int level){
    switch(level){
        case 0:
            strcpy(str, "[debug]:");
            break;
        case 1:
            strcpy(str, "[info]:");
            break;
        case 2:
            strcpy(str,"[warn]");
            break;
        case 3:
            strcpy(str, "[erro]");
            break;
        default:
            strcpy(str, "[info]:");
    }
}

/*系统信息格式化输出，时间格式化+内容格式化*/
void Log::writeLog(int level, const char *format, ...){
    struct timeval now = {0,0}; //  秒， 微秒结构体，距1970秒数
    gettimeofday(&now,NULL);
    time_t t = now.tv_sec;
    struct tm *systm = localtime(&t);
    struct tm mytm = *systm;
    char s[16] = {0};
    level2Str(s,level);

    m_mutex.lock();
    count++;
    /*
    已有的日志文件不是今天创建的，则需要重新创建一个日志文件
    日志行数是最大行数的倍数，则需要重开一个日志文件
    */
    if(today != mytm.tm_mday || count % maxLines == 0){
        fflush(fp);
        fclose(fp);
        char newLog[256] = {0}; // 新日志名
        char timeStr[16] = {0}; // 日志名中时间部分
        snprintf(timeStr,
                16, 
                "%d_%02d_%02d_", 
                mytm.tm_year + 1900, 
                mytm.tm_mon + 1, 
                mytm.tm_mday);
        if(today != mytm.tm_mday){ // 当前日志文件不是今天创建的，不该往里写文本信息。要重新开一个文件来写。
            today = mytm.tm_mday;
            count = 0;
            snprintf(newLog,
                    255, 
                    "%s%s%s",
                    dirName,
                    timeStr,
                    logName);
        }else{
            snprintf(newLog,
                    255,
                    "%s%s%s.%lld",
                    dirName,
                    timeStr,
                    logName,
                    count / maxLines);// 这一天日志很多，开了很多个日志文件，以count/maxLines作为编号
        }

        fp = fopen(newLog, "a");// 新建一个日志文件,文件名newLog
    }
    m_mutex.unlock();
    va_list valst;
    va_start(valst, format);  //可变参数的宏，获得第一个可变参数的地址
    string logStr;
    m_mutex.lock();
    int n = snprintf(buf,
                    48,
                    "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                    mytm.tm_year + 1900,
                    mytm.tm_mon + 1,
                    mytm.tm_mday,
                    mytm.tm_hour,
                    mytm.tm_min,
                    mytm.tm_sec,
                    now.tv_usec,
                    s);
    int m = vsnprintf(buf + n,
                      logBufferSize - 1,
                      format, 
                      valst);
    buf[n + m] = '\n';
    buf[n + m + 1] = '\0';
    logStr = buf;
    m_mutex.unlock();
    if(async && !logQueue->full()){
        logQueue->push(logStr);
    }else{
        m_mutex.lock();
        fputs(logStr.c_str(),fp);  //日志的核心就是干这件事，把文本信息写入文件
#ifdef YYL_DEBUG
        printf("==========finish to write log and log information is \"%s\"========\n",
                logStr.substr(0,logStr.size()-1).c_str());
#endif
        m_mutex.unlock();
    }
    va_end(valst);
}

void Log::flush(void){
    m_mutex.lock();
    fflush(fp);  // 缓冲区数据强制刷入流
    m_mutex.unlock();
}

void *Log::flushLogThread(void *args){
    return Log::getInstance()->asyncWriteLog();
}

/*异步写入日志方法*/
void *Log::asyncWriteLog(){
    string logStr;
    while(logQueue->pop(logStr)){
        m_mutex.lock();
        fputs(logStr.c_str(),fp);
        m_mutex.unlock();
    }
    return (void *)true;
}

Log::~Log(){
    if(fp != NULL){
        fclose(fp);
    }
}