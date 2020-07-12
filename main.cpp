#include<libgen.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/un.h>
#include<bits/socket.h>
int main(int argc, char *argv[]){
    if(argc < 3){
        printf("usage:%s please input ip adrress and port\n", basename(argv[0]));
    }
    // set socket address, and socket() -> bind() -> listen()
    const char *ip = argv[1];
    int port  = atoi(argv[2]);
    struct sockaddr_in addrServer;
    addrServer.sin_family = AF_INET;


    return 0;

}
