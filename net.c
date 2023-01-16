#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include"net.h"
#include"common.h"

extern struct ctx ctx;

extern struct msg_que recv_que;
extern struct msg_que send_que;

int client_connect(char* host, int port){
    struct sockaddr_in addr;

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    inet_pton(AF_INET, host, &addr.sin_addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("socket connect");
        return -1;
    }
    ctx.is_connected = 1;
    return sock_fd;
}
void send_message(int sock_fd){
    int wcount = 0;
    if ((wcount = write(sock_fd, send_que.buf[send_que.m_num % 0xff], sizeof(unsigned char)*strlen(send_que.buf[send_que.m_num % 0xff]))) != -1);
    if (wcount == -1)
        perror("socket write");
    send_que.m_num++;
}

void *recv_message(void *arg){
    extern int sock_fd;
    int rcount;
    while ((rcount = read(sock_fd, recv_que.buf[recv_que.m_num % 0xff], sizeof(unsigned char) * BUFLEN)) != -1){
    if (rcount == -1)
        perror("socket read");
    rcount = 0;
    recv_que.m_num++;
    }
}