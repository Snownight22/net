#ifndef _TCP_SESSION_H_
#define _TCP_SESSION_H_

#include <netinet/in.h>
#include <pthread.h>
#include "list.h"

#define TCP_SESSION_OK    (0)
#define TCP_SESSION_ERR    (-1)

#define TCP_SESSION_NOT_CONNECTED    (0)
#define TCP_SESSION_CONNECTED    (1)

#define LISTEN_BACKLOG    (50)

typedef void (*recv_callback)(const void *handler, const void *data, const int length);
typedef void (*accept_callback)(const void *clientInfo);

typedef struct client_handler
{
    struct sockaddr_in saddr;
    unsigned short sport;
    int ssockfd;
    int isconnected;
    pthread_t thread;
    int isalive;
    int reconnectTime;
    recv_callback callback;
}stClientHandler;

typedef struct client_info
{
    stListEntry entry;
    char clientip[16];
    unsigned int clientport;
    int clientfd;
}stClientInfo;

typedef struct server_handler
{
    stListHead clientList;
    pthread_mutex_t mutex;
    int clientcount;
    struct sockaddr_in laddr;
    unsigned short lport;
    int listenfd;
    int backlog;
    int isalive;
    pthread_t listenThread;
    recv_callback callback;
    accept_callback a_callback;
}stServerHandler;


int tcp_client_send(void *tcpHandler, void *buff, int length);
void* tcp_client_init(char *domain, unsigned short port, recv_callback callback);
void tcp_client_destroy(void *handler);

int tcp_server_send(void *tcpHandler, void *buff, int length);
void tcp_server_remove_client(void *serverHandler, stClientInfo *client);
void* tcp_server_init(char *saddr, unsigned short port, recv_callback callback, accept_callback aCallBack);
void tcp_server_destroy(void *handler);

#endif 
