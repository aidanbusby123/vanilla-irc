#ifndef NET_H
#define NET_H

int client_connect(char* host, int port);
void* recv_message(void *arg);
void send_message(int sock_fd);

#endif