//#pragma one
#ifndef __HTTP__
#define __HTTP__
#include<sys/epoll.h>
#include<fcntl.h>
/*register fd to epoll envet table and choose whether oneshot */
void addfd(int epollEveTable, int fd, bool isOneShot){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(isOneShot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollEveTable, EPOLL_CTL_ADD, fd, &event);
}
/*set non-blocking fd*/
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
/*重置fd上的事件。重置之后下次fd就绪，仍会成为就绪事件交给另一个线程处理*/
void rest_oneshot(int epollEveTable, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollEveTable, EPOLL_CTL_MOD, fd, &event);
}

#endif