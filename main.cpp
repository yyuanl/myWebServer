#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <iostream>

#include "./lock/locker.h"
#include "./threadPool/threadPool.h"
#include "./ioProcessUnit/http_conn.h"
#include "./sqlConn/sqlConnPool.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
#define ALARM_TIME 5

extern int addfd( int epollfd, int fd, bool one_shot );
extern int removefd( int epollfd, int fd );
extern int setnonblocking( int fd );

//定时器相关参数
static int pipefd[2];

void sig_handler(int sig){
    // 信号到来，简单的往管道写入消息
    int old_error = errno;
    int msg = sig;
    send(pipefd[1], (void *)&msg, 1, 0);
    errno = old_error;
}

void addsig( int sig, void( handler )(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART; //进程调用某个阻塞系统调用时，收到该信号，进程不会返回而是重新执行系统调用
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

void show_error( int connfd, const char* info )
{
    printf( "%s", info );
    send( connfd, info, strlen( info ), 0 );
    close( connfd );
}

void timer_handler(){
    //...
    alarm(ALARM_TIME);
}


int main( int argc, char* argv[] )
{
    
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
     
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    addsig( SIGPIPE, SIG_IGN ); // SIG_IGN 默认信号处理程序
    //创建数据库连接池
    sqlConnPool *sql_conn_pool = sqlConnPool::getInstance("localhost", "root", "rand", "yylWebDB", 3306, 8);  // 数据库IP地址  登录名 密码 仓库名 数据库端口号 数据库连接个数
    // 创建线程池
    threadpool< http_conn >* pool = NULL;
    try
    {
        pool = new threadpool< http_conn >;
    }
    catch( ... )
    {
        return 1;
    }
    http_conn* users = new http_conn[ MAX_FD ];
    assert( users );
    //初始化数据库读取表
    users->initmysql_result(sql_conn_pool);
cout<<"initmysql_result"<<endl;

    int user_count = 0;
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    struct linger tmp = { 1, 0 };
    setsockopt( listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );   //若有数据待发送，则延迟关闭

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret >= 0 );

    ret = listen( listenfd, 5 );
    assert( ret >= 0 );

    epoll_event events[ MAX_EVENT_NUMBER ];
    int epollfd = epoll_create( 5 );
    assert( epollfd != -1 );
    addfd( epollfd, listenfd, false );
    http_conn::m_epollfd = epollfd;
    
    // 处理非活动连接相关
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]); // 写端设置为非阻塞
    addfd(epollfd, pipefd[0], false);  // 同统一事件源，监听管道读端是否有信号到达

    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);

    bool timeout = false;
    bool stop = false;

    alarm(ALARM_TIME);
    while(!stop)
    {
        int number = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 );
        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            printf( "epoll failure\n" );
            break;
        }

        for ( int i = 0; i < number; i++ )
        {
            int sockfd = events[i].data.fd;
            if( sockfd == listenfd )
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                if ( connfd < 0 )
                {
                    printf( "errno is: %d\n", errno );
                    continue;
                }
                if( http_conn::m_user_count >= MAX_FD )
                {
                    show_error( connfd, "Internal server busy" );
                    continue;
                }
                
                users[connfd].init( connfd, client_address );
            }
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
            {
                //如果有异常直接关闭客户端连接   移除对应的定时器
                users[sockfd].close_conn();
                //....移除定时器操作
            }else if(sockfd == pipefd[0] && (events[i].events & EPOLLIN)){
                //处理信号
                char msgs[10];
                ret = recv(pipefd[0], (void*)msgs, sizeof(msgs), 0);
                if(ret == -1){
                    continue;
                }else if(ret == 0){
                    continue;
                }else{
                    for(int i = 0; i < ret;i++){
                        switch (msgs[i])
                        {
                        case SIGALRM:
                            timeout = true;
                            break;
                        case SIGTERM:
                            stop = true;
                        default:
                            break;
                        }
                    }
                }
            }// 处理客户端发送的数据
            else if( events[i].events & EPOLLIN )
            {
#ifndef NDEBUDE
                std::cout<<"======[m_log]====="<<std::endl;
                std::cout<<sockfd<<" connection is ready to read"<<std::endl;
                std::cout<<"======[m_log]====="<<std::endl;
                
#endif
                if( users[sockfd].read() )
                {
                    // 给连接创建一个定时器对象
                    pool->append( users + sockfd );
                    //定时器容器相关的处理
                }
                else
                {   // 定时容器相关操作 回调函数 关闭连接
                    users[sockfd].close_conn();
                }
                
            }
            else if( events[i].events & EPOLLOUT )
            {
                if( users[sockfd].write() )
                {
                    //定时器操作 继续维护活动连接
                    
                }else{
                    // 调用定时器回调函数 删除连接
                    users[sockfd].close_conn();
                }
            }
            else
            {}
        }

        if(timeout){
            //当有连接超时 重置定时器 重发alarm信号
            timer_handler();
            timeout = false;
        }
    }

    close( epollfd );
    close( listenfd );
    close(pipefd[0]);
    close(pipefd[1]);
    delete [] users;
    delete pool;
    return 0;
}
