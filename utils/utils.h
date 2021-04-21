#ifndef __UTILS_H__
#define __UTILS_H__
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <assert.h>
#include "../threadPool/threadPool.h"
#include "../httpConnection/http_conn.h"
#include "../log/log.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
#define ALARM_TIME 5

#define SYNCLOG  // 同步日志
//#define ASYNCLOG //异步日志
#define YYL_DEBUG  //代码调试开关
/* Our own error-handling functions */
void unix_error(char *msg);

/* Signal wrappers */
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*针对定时器的信号处理函数*/
void sig_handler(int sig);

/*epoll_ctl包装函数*/
void addfd( int epollfd, int fd, bool one_shot ); //向内核epoll事件表添加新的需要监听描述符
void removefd( int epollfd, int fd );  // 删除文件描述符
void modfd( int epollfd, int fd, int ev ); //修改文件描述符

/*设置文件描述符为非阻塞*/
int setnonblocking( int fd );

void show_error( int connfd, const char* info );

/*epoll_wait包装函数*/
int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

/*新连接请求处理逻辑*/
int clientConnRequest(int listenfd,http_conn* users,sqlConnPool *connPool);

/*http连接可读事件处理逻辑*/
void readEvent(int sockfd, http_conn *users, threadpool< http_conn >* pool);

/*http连接可写事件处理逻辑*/
void writeEvent(int sockfd, http_conn *users);

/*定时器信号处理逻辑*/
bool timerEvent(bool *timeout, bool *stop);

/*异常事件处理逻辑*/
void exceptionEvent(http_conn *users, int sockfd);

#endif