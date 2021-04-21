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
#include "./httpConnection/http_conn.h"
#include "./sqlConn/sqlConnPool.h"
#include "./utils/utils.h"
#include "./log/log.h"
#include "./log/blockQueue.h"


extern int pipefd[2];

void timer_handler(){
    //...
    alarm(ALARM_TIME);
}

#define TEST


void writeTest(int fd, int m_epollfd,int m_sockfd){
    
    
    const char* htmlPath = "/home/yyl/myWebServer/src/picture.html";
    struct iovec m_iv[2];
    char messages[ 2048 ];
    struct stat m_file_stat;
    memset(messages, '\0', sizeof(messages));
    stat(htmlPath, &m_file_stat);
    int file = open(htmlPath, O_RDONLY);
    char *m_file_address = (char*)mmap(0,m_file_stat.st_size, PROT_READ, MAP_PRIVATE, file, 0);
    
    sprintf(messages,"HTTP/1.1 200 OK\r\n");
    sprintf(messages,"%sContent-Length: %d\r\n",messages,(int)m_file_stat.st_size);
    sprintf(messages,"%sConnection: keep-alive\r\n",messages);
    sprintf(messages,"%s\r\n",messages);
    //printf("%s",messages);

    m_iv[0].iov_base = messages;
    m_iv[0].iov_len = strlen(messages);
    m_iv[1].iov_base = m_file_address;
    m_iv[1].iov_len = m_file_stat.st_size;

    int toSend = m_iv[0].iov_len + m_iv[1].iov_len;
    int sended = 0;
    while(1){
        int retByte;
        retByte = writev(fd,m_iv,2);
        if (retByte < 0){
            if (errno == EAGAIN){
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                break;
            }
            break;
        }
        toSend -= retByte;
        sended += retByte;
        if(sended >= m_iv[0].iov_len){
            m_iv[1].iov_base = m_file_address + (sended - m_iv[0].iov_len);
            m_iv[0].iov_len = 0;
        }else{
            m_iv[0].iov_base = messages + sended;
            m_iv[0].iov_len = m_iv[0].iov_len - sended;
        }
        if(toSend <= 0){
            break;
        }
    }
    
    close(file);
    munmap( m_file_address, m_file_stat.st_size );

}

int main1(){
    
    const char* htmlPath = "/home/yyl/myWebServer/src/picture.html";

    struct stat m_file_stat;
    stat(htmlPath,&m_file_stat);
    printf("%d\n", m_file_stat.st_size);

    return 0;
}
int main( int argc, char* argv[] ){

#ifdef SYNCLOG
    Log::getInstance()->init("ServerLog", 2000,800000,0);
#endif

#ifdef ASYNCLOG
    Log::getInstance()->init("ServerLog", 2000,800000,8);
#endif
    
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
     
    const char* ip = argv[1];  //
    int port = atoi( argv[2] );

    //addsig( SIGPIPE, SIG_IGN );  
    Signal(SIGPIPE,SIG_IGN);// SIG_IGN 默认信号处理程序 SIGPIPE:向一个没有读用户的管道做写操作
    //创建数据库连接池
    sqlConnPool *sql_conn_pool = sqlConnPool::getInstance("localhost", "yyl", "123456", "yylWebDB", 3306, 8);  // 数据库IP地址  登录名 密码 仓库名 数据库端口号 数据库连接个数
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
    //cout<<"initmysql_result"<<endl;

    int user_count = 0;
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    struct linger tmp = { 1, 0 };
    setsockopt( listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );   //若有数据待发送，则延迟关闭

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );   //点分十进制转换为二进制
    address.sin_port = htons( port );  //htons:以网络字节序表示的16位整数

    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );//讲套接字与本机地址关联
    assert( ret >= 0 );

    ret = listen( listenfd, 5 );  // 进行监听，宣告该sockekt愿意接受链接请求；第二个参数含义在linux2.2之后修改，表示完成三次握手等待accept调用的队列长度
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

    // addsig(SIGALRM, sig_handler, false);    //SIGALRM：来自alarm函数的定时器信号
    // addsig(SIGTERM, sig_handler, false);   //SIGTERM：软件终止信号

    /*设置信号处理函数*/
    Signal(SIGALRM, sig_handler);//SIGALRM：来自alarm函数的定时器信号
    Signal(SIGTERM, sig_handler);//SIGTERM：软件终止信号

    bool timeout = false;
    bool stop = false;

    alarm(ALARM_TIME);
    LOG_INFO("listen file description is %d", listenfd);
    while(!stop){
        int eventNumber;
        if((eventNumber = Epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1 )) == -2){
            LOG_INFO("epoll_wait monitor %d ready event and while loop break", eventNumber);
            break;
        }
        LOG_INFO("epoll_wait monitor %d ready event", eventNumber);
        for ( int i = 0; i < eventNumber; i++ ){
            int sockfd = events[i].data.fd;
            LOG_INFO("judge fd=%d event", sockfd);
            if( sockfd == listenfd ){//新连接请求
                LOG_INFO("fd=%d event is a new connection request", sockfd);
                if(clientConnRequest(listenfd, users,sql_conn_pool) == -1){
                    continue;
                }
            }else if(sockfd == pipefd[0] && (events[i].events & EPOLLIN)){  //管道可读端就绪事件
                LOG_INFO("fd=%d event is pipe read event", sockfd);
                if(!timerEvent(&timeout, &stop)){
                    continue;
                }
            }else if( events[i].events & EPOLLIN ){  //可读事件
                LOG_INFO("fd=%d event is read event", sockfd);
                readEvent(sockfd, users, pool);
            }else if( events[i].events & EPOLLOUT ){// 发送数据
                LOG_INFO("fd=%d event is write event", sockfd);
                writeEvent(sockfd, users);
                // writeTest(sockfd,epollfd,sockfd);
                // break;
            }else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) ){
                LOG_INFO("fd=%d event is exception event", sockfd);
                exceptionEvent(users, sockfd);
            }else{

            }
            
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


// int main(){
// #ifdef SYNCLOG
//     Log::getInstance()->init("ServerLog", 2000,800000,0);
// #endif
//     LOG_INFO("this a log test %d",123424);
//     return 0;
// }
