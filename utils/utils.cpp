#include "utils.h"
#include "../httpConnection/http_conn.h"

/************************** 
 * Error-handling functions
 **************************/
/* $begin unixerror */
void unix_error(const char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

/************************************
 * Wrappers for Unix signal functions 
 ***********************************/

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    // struct sigaction action, old_action;

    // action.sa_handler = handler;  
    // sigemptyset(&action.                  ); /* Block sigs of type being handled */
    // action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    // if (sigaction(signum, &action, &old_action) < 0){
    //     string s = "Signal error";
    //     unix_error(s.c_str());
    // }
	    
    // return (old_action.sa_handler);
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( 1 )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( signum, &sa, NULL ) != -1 );
    return NULL;
}
/* $end sigaction */

/* $begin sig_handler */
int pipefd[2];//定时器相关参数
void sig_handler(int sig){
    // 信号到来，简单的往管道写入消息
    int old_error = errno;
    int msg = sig;
    send(pipefd[1], (void *)&msg, 1, 0);
    errno = old_error;
}
/* $end sig_handler */


/* $begin setnonblocking */
int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}
/* $end setnonblocking */

/* $begin addfd */
void addfd( int epollfd, int fd, bool one_shot )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if( one_shot )
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );
}
/* $end addfd */

/* $begin removefd */
void removefd( int epollfd, int fd )
{
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close( fd );
}
/* $end removefd */

/* $begin modfd */
void modfd( int epollfd, int fd, int ev )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}
/*
EPOLLET:et模式，epoll_wait检测到某个文件描述符上有事件发生则通知应用程序（加入就绪事件集），
此时必须立即处理该事件，如果不处理下次epoll_wait并不会把该事件通知给应用程序。lt模式，反之，
如果没有及时处理，则下次epoll_wait仍然会通知给应用程序。
EPOLLONESHOT：保证一个描述符某个时刻只有一个线程来处理
EPOLLRDHUP：对端断开
*/
/* $end modfd */

/* $begin show_error */
void show_error( int connfd, const char* info )
{
    printf( "%s", info );
    send( connfd, info, strlen( info ), 0 );
    close( connfd );
}
/* $end show_error */

/* $begin Epoll_wait */
int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout){
    int number = epoll_wait( epfd, events, maxevents, timeout);
    std::cout<<"number = "<<number<<std::endl;
    if(errno == EINTR){
        std::cout<<"errno is EINTR"<<std::endl;
    }
    if ( ( number < 0 ) && ( errno != EINTR ) )
    {
        printf( "epoll failure\n" );
        return -2;
    }
    return number;
}
/* $end Epoll_wait */

/* $begin clientConnRequest */
int clientConnRequest(int listenfd,http_conn* users,sqlConnPool *connPool){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof( client_address );
    int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
    //std::cout<<"connected socket is "<<connfd<<std::endl;
    LOG_DEBUG("a new http connection established, fd = %d",connfd);
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
        return -1;
    }
    if( http_conn::m_user_count >= MAX_FD )
    {
        show_error( connfd, "Internal server busy" );
        return -1;
    }
    
    users[connfd].init( connfd, client_address ,connPool);
    return connfd;
}
/* $end clientConnRequest */

/* $begin readEvent */
void readEvent(int sockfd, http_conn *users, threadpool< http_conn >* pool){
#ifndef NDEBUDE
    std::cout<<"======[m_log]====="<<std::endl;
    std::cout<<sockfd<<" connection is ready to read"<<std::endl;
    std::cout<<"======[m_log]====="<<std::endl;             
#endif
    if( users[sockfd].read() ){
        // 给连接创建一个定时器对象
        pool->append( users + sockfd );
        //定时器容器相关的处理
    }else{   
        // 定时容器相关操作 回调函数 关闭连接
        users[sockfd].close_conn();
    }
}
/* $end readEvent */

/* $begin writeEvent */
void writeEvent(int sockfd, http_conn *users){
    if( users[sockfd].write() ){
        //定时器操作 继续维护活动连接
    }else{
        // 调用定时器回调函数 删除连接
        users[sockfd].close_conn();
    }
}
/* $end writeEvent */

/* $begin timerEvent */
bool timerEvent(bool *timeout, bool *stop){
    //处理信号
    char msgs[10];
    int ret = recv(pipefd[0], (void*)msgs, sizeof(msgs), 0);
    if(ret == -1 || ret == 0){  // 读取0个字节表示对端关闭
        return false;
    }else{
        for(int i = 0; i < ret;i++){
            switch (msgs[i]){
            case SIGALRM:
                *timeout = true;
                break;
            case SIGTERM:
                *stop = true;
            default:
                break;
            }
        }
    }
    return true;
}
/* $end timerEvent */

void exceptionEvent(http_conn *users, int sockfd){
    //如果有异常直接关闭客户端连接   移除对应的定时器
    users[sockfd].close_conn();
    //....移除定时器操作
}