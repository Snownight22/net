#ifndef _TCP_SESSION_H_
#define _TCP_SESSION_H_

#include <netinet/in.h>
#include <pthread.h>
#include "list.h"

//成功
#define TCP_SESSION_OK    (0)
//失败
#define TCP_SESSION_ERR    (-1)

//未连接
#define TCP_SESSION_NOT_CONNECTED    (0)
//已连接
#define TCP_SESSION_CONNECTED    (1)

#define LISTEN_BACKLOG    (50)

typedef void (*recv_callback)(const void *handler, const void *data, const int length);
typedef void (*accept_callback)(const void *clientInfo);

/*客户端句柄，记录客户端连接信息*/
typedef struct client_handler
{
    struct sockaddr_in saddr;    //记录要连接的服务器地址信息
    unsigned short sport;
    int ssockfd;    //连接服务器用的socket套接字
    int isconnected;    //记录当前连接状态(TCP_SESSION_CONNECTED和TCP_SESSION_NOT_CONNECTED)
    pthread_t thread;
    int isalive;
    int reconnectTime;    //重连时间
    recv_callback callback;    //收到服务器信息后的回调函数
}stClientHandler;

/*记录连接上服务器的客户端信息*/
typedef struct client_info
{
    stListEntry entry;
    char clientip[16];
    unsigned int clientport;
    int clientfd;
}stClientInfo;

/*服务器句柄，记录服务器连接信息*/
typedef struct server_handler
{
    stListHead clientList;    //已连接上服务器的客户端列表
    pthread_mutex_t mutex;
    int clientcount;    //当前已连接的客户端数量
    struct sockaddr_in laddr;
    unsigned short lport;
    int listenfd;    //当前监听状态的socket套接字
    int backlog;
    int isalive;
    pthread_t listenThread;
    recv_callback callback;    //收到客户端信息后的回调函数
    accept_callback a_callback;    //接收到客户端连接后的回调函数
}stServerHandler;

/*客户端发送信息函数, tcpHandler:客户端句柄, buff:要发送的数据, length:发送的数据长度*/
int tcp_client_send(void *tcpHandler, void *buff, int length);
/*客户端初始化函数，domain:服务器域名或地址, port:服务器端口, callback:收到服务器信息后的回调函数。返回服务器句柄*/
void* tcp_client_init(char *domain, unsigned short port, recv_callback callback);
/*客户端销毁函数,与tcp_client_init成对出现, handler:客户端句柄*/
void tcp_client_destroy(void *handler);

/*服务器发送信息函数，tcpHandler:stClientInfo　要发送给的客户端信息, buff:要发送的数据, length:发送的数据长度*/
int tcp_server_send(void *tcpHandler, void *buff, int length);
/*服务器移除客户端函数，serverHandler:服务器句柄, client:客户端信息*/
void tcp_server_remove_client(void *serverHandler, stClientInfo *client);
/*服务器初始化函数, saddr:要监听的服务器地址 NULL时监听本机任意地址, port:要监听的服务器端口, callback:收到客户端信息后的回调
 *aCallBack:接收到客户端连接后的回调函数. 返回服务器句柄*/
void* tcp_server_init(char *saddr, unsigned short port, recv_callback callback, accept_callback aCallBack);
/*服务器销毁函数，与tcp_server_init成对出现，handler:服务器句柄*/
void tcp_server_destroy(void *handler);

#endif 
