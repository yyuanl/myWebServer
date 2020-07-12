#ifndef __WORK__
#define __WORK__
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<stdio.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include"../ioProcessUnit/http.h"
#define BUFFER_SIZE 1024
#define MAX_LINE 8192
#define MAX_BUF 8192
struct fds{
    int epollEveTab;
    int sockfd;
};

/*work thread*/
void *worker(void *args){
    int sockfd = ((fds*)args)->sockfd;
    int epoEveTab = ((fds*)args)->epollEveTab;
    printf("start new thread to receive data on fd: %d\n", sockfd);
    char buf[BUFFER_SIZE];
    memset(buf, '\0', BUFFER_SIZE);
    while(1){
        int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
#ifndef NDEBUG
        printf("ret is %d\n", ret);
#endif
        if(ret == 0){
            close(sockfd);
            printf("the connection closeed.\n");
            break;
        }else if(ret < 0){
            if(errno == EAGAIN){
                // read all data
                rest_oneshot(epoEveTab, sockfd);
                break;
            }
        }else{
            //printf("get request content: %s\n", buf);
            char *pbuf = index(buf,'\n');
            *pbuf = '\0';
            printf("get request content: %s\n", buf);
            char method[BUFFER_SIZE], url[BUFFER_SIZE], version[BUFFER_SIZE];
            sscanf(buf, "%s %s %s", method, url, version);
#ifndef NDEBUG
            printf("[log]function%s:request method is %s\n", __func__,method);
#endif
            if(!strcasecmp(method, "GET")){
                char path[MAX_LINE];
                strcpy(path, ".");
                strcat(path, url);
#ifndef NDEBUG
                printf("request resource path is %s\n", path);
#endif  
                struct stat sbuf;
                if(stat("./home.html", &sbuf) < 0){
                    printf("error,not find file\n");
                }
                char sendbuf[MAX_BUF];
                sprintf(sendbuf,"HTTP/1.1 200 OK\r\n");
                sprintf(sendbuf, "%sServer: Tiny Web Server\r\n", sendbuf);
                sprintf(sendbuf, "%sConnection: close\r\n", sendbuf);
                sprintf(sendbuf, "%sContent-length: %d\r\n", sendbuf, (int)sbuf.st_size);
                sprintf(sendbuf, "%sContent-type: %s\r\n\r\n", sendbuf, "text/html");
                send(sockfd, sendbuf, strlen(sendbuf),0);    //line:netp:servestatic:endserve
                printf("Response headers: \n");
                printf("%s", sendbuf);
                int srcfd = open("./home.html", O_RDONLY, 0);
                assert(srcfd >= 0);
                
                char *ptr = NULL;
                ptr = (char *)mmap(NULL,sbuf.st_size,PROT_READ, MAP_PRIVATE,srcfd,0);
               
                //assert(MAP_FAILED != (void *)-1);
#ifndef NDEBUG
                printf("html content is:\n%s\n", ptr);
                if(MAP_FAILED == (void *)-1){
                    printf("mmap error\n");
                    //break;
                }else{
                    printf("html content is:\n%s\n", ptr);
                }           
#endif
                send(sockfd, ptr, (int)sbuf.st_size,0);
                close(srcfd);
                munmap(ptr, sbuf.st_size);

#ifndef NDEBUG
                printf("successfully send html to web.\n");
#endif  
                fflush(stdout);
            }
            
        }


    }
    printf("end thread receiving data on fd:%d\n", sockfd);
}


#endif