#include<libgen.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/un.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<assert.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<pthread.h>
#include"./ioProcessUnit/http.h"
#include"./logicalProcessUnit/work.h"
//#define NDEBUG
#define MAX_EVENT_NUMBER 1024

int main(int argc, char *argv[]){
    if(argc < 3){
        printf("usage:%s please input ip adrress and port\n", basename(argv[0]));
    }
    struct epoll_event okevents[MAX_EVENT_NUMBER]; // for prepared event;
    // set socket address, and socket() -> bind() -> listen()
    const char *ip = argv[1];
    int port  = atoi(argv[2]);
    struct sockaddr_in addrServer;
    addrServer.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addrServer.sin_addr);
    addrServer.sin_port = htons(port);
    
    int listfd = socket(PF_INET, SOCK_STREAM,0);
    assert(listfd != -1);
 
    int ret = bind(listfd, (struct sockaddr * )&addrServer, sizeof(addrServer));
    assert(ret != -1);
    ret = listen(listfd, 5);

    int eventTable = epoll_create(5);
    assert(eventTable != -1);
    addfd(eventTable, listfd, false); // can't register listen fd with EPOLLONESHOT
#ifndef NDEBUG
    printf("%s function :successfuly register listen fd %d at %s.\n", __func__,listfd, __DATE__);
#endif
    while(1){
        int ret = epoll_wait(eventTable, okevents, MAX_EVENT_NUMBER,-1);
        if(ret < 0){
            printf("epoll failure\n");
            break;
        }
        for(int i = 0; i < ret; i++){
            int currfd = okevents[i].data.fd;
            if(currfd == listfd){
                struct sockaddr_in reqClientAddr;
                socklen_t reqClientAddLen = sizeof(reqClientAddr);
                int connfd = accept(listfd, (struct sockaddr *)&reqClientAddr, &reqClientAddLen);
                assert(connfd != -1);
                addfd(eventTable, connfd, true); // register connected socket fd with EPOLLONESHOT

#ifndef NDEBUG
                printf("%s function :successfuly register connect fd %d at %s.\n", __func__,connfd, __DATE__);
#endif               
            }else if(okevents[i].events & EPOLLIN){
                pthread_t thread;
                fds newWorkFds;
                newWorkFds.epollEveTab = eventTable;
                newWorkFds.sockfd = currfd;
                pthread_create(&thread, NULL, worker, (void *) &newWorkFds);
            }else{
                printf("something else happened \n");
            }
        }
    }

    


    close(listfd);
#ifndef NDEBUG
    printf("service stop..\n");
#endif

    return 0;

}
