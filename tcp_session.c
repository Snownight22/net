#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "log.h"
#include "tcp_session.h"

int tcp_client_connect(stClientHandler *handler)
{
    if (handler->ssockfd >= 0)
    {
        close(handler->ssockfd);
        handler->ssockfd = -1;
    }

    if (0 > (handler->ssockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        LOG_ERROR("Create Socket error\n");
        return TCP_SESSION_ERR;
    }

    if (-1 == (connect(handler->ssockfd, (struct sockaddr*)(&handler->saddr), sizeof(handler->saddr))))
    {
        LOG_ERROR("Connect Server Fail!\n");
        handler->isconnected = TCP_SESSION_NOT_CONNECTED;
        return TCP_SESSION_ERR;
    }

    LOG_INFO("Connected to server %s\n", inet_ntoa(handler->saddr.sin_addr));
    handler->isconnected = TCP_SESSION_CONNECTED;

    int addrlen = sizeof(struct sockaddr);
    int ret;
    ret = getsockname(handler->ssockfd, (struct sockaddr*)(&handler->laddr), (socklen_t *)(&addrlen));
    if (0 != ret)
        LOG_ERROR("Get Sock Name error, ret:%d, errno:%d\n", ret, errno);

    return TCP_SESSION_OK;
}

int tcp_client_addr_get(void *clientHandler, long *lip, unsigned short *lport)
{
    stClientHandler *handler = (stClientHandler *)clientHandler;
    if (TCP_SESSION_NOT_CONNECTED == handler->isconnected)
    {
        LOG_ERROR("tcp client is not connected\n");
        return TCP_SESSION_ERR;
    }

    *lip = ntohl(handler->laddr.sin_addr.s_addr);
    *lport = ntohs(handler->laddr.sin_port);

    return TCP_SESSION_OK;
}

static void tcp_client_send_signal(int signum)
{
    LOG_INFO("get sigpipe signal\n");
}

int tcp_client_send(void *tcpHandler, void *buff, int length)
{
    stClientHandler *handler = (stClientHandler *)tcpHandler;

    if (NULL == handler)
    {
        LOG_ERROR("handler is null\n");
        return TCP_SESSION_ERR;
    }

    if (NULL == buff || 0 == length)
    {
        LOG_ERROR("send Buff is NULL or length is zero\n");
        return TCP_SESSION_ERR;
    }

    //signal(SIGPIPE, tcp_client_send_signal);
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    if ((TCP_SESSION_NOT_CONNECTED == handler->isconnected) || (-1 == handler->ssockfd))
        return TCP_SESSION_ERR;

    int ret = send(handler->ssockfd, buff, length, 0);
    //LOG_INFO("client send over ret:%d\n", ret);
    if (0 > ret)
    {
        //handler->isconnected = TCP_SESSION_NOT_CONNECTED;
        return TCP_SESSION_ERR;
    }

    return TCP_SESSION_OK;
}

void tcp_client_reconnectTime_set(void *handler, int time)
{
    stClientHandler *tcpHandler = (stClientHandler *)handler;
    if (NULL == tcpHandler)
    {
        LOG_ERROR("handler is null\n");
        return ;
    }
    tcpHandler->reconnectTime = time;
}

void tcp_client_timeout_set(void *tcpHandler, int time)
{
    stClientHandler *handler = (stClientHandler *)tcpHandler;
    if (NULL == handler)
    {
        LOG_ERROR("handler is null\n");
        return ;
    }

    handler->timeout = time;
}

void tcp_client_close_session(void *tcpHandler)
{
    stClientHandler *handler = (stClientHandler *)tcpHandler;
    if (NULL == handler)
    {
        LOG_ERROR("handler is null\n");
        return ;
    }

    handler->shouldClose = 1;
}

void* tcp_client_process(void*arg)
{
    stClientHandler *handler = (stClientHandler *)arg;
    int ret;
    char buff[1024];
    fd_set rsets, wsets;
    int reconnectTime;
    struct timeval waittime;

    FD_ZERO(&rsets);
    FD_ZERO(&wsets);
    while (handler->isalive)
    {
        if (1 == handler->shouldClose)
            break;
        reconnectTime = handler->reconnectTime;
        if (TCP_SESSION_NOT_CONNECTED == handler->isconnected)
        {
            if (reconnectTime == 0)
                break;
            tcp_client_connect(handler);
        }
        if (TCP_SESSION_NOT_CONNECTED == handler->isconnected)
        {
            sleep(reconnectTime);
            continue;
        }

        FD_SET(handler->ssockfd, &rsets);
        //FD_SET(handler->ssockfd, &wsets);
        waittime.tv_sec = handler->timeout;
        select(handler->ssockfd+1, &rsets, NULL/*&wsets*/, NULL, &waittime);
        if (FD_ISSET(handler->ssockfd, &rsets))
        {
            //LOG_INFO("client begin receive\n");
            ret = recv(handler->ssockfd, buff, 1024, 0);
            if (0 == ret)
            {
                LOG_INFO("receive 0, server is gone\n");
                handler->isconnected = TCP_SESSION_NOT_CONNECTED;
                if (handler->callback2)
                    handler->callback2(handler);
                continue;
            }
            else if (0 < ret)
            {
                LOG_INFO("receive buff len:%d\n", ret);
                if (NULL != handler->callback)
                    handler->callback(handler, buff, ret);
            }
        }
    }

    LOG_INFO("close client session\n");
    if (-1 != handler->ssockfd)
    {
        close(handler->ssockfd);
        handler->ssockfd = -1;
        handler->isconnected = TCP_SESSION_NOT_CONNECTED;
    }

}

void* tcp_client_init(char *domain, unsigned short port, recv_callback callback, close_callback callback2)
{
    struct hostent *host;
    int fd;
    stClientHandler *handler;

    //log_init(NULL);

    host = gethostbyname(domain);
    if (NULL == host)
    {
        LOG_ERROR("Get Host Ip error\n");
        return NULL;
    }


    handler = (stClientHandler *) malloc(sizeof(stClientHandler));
    if (NULL == handler)
    {
        LOG_ERROR("Malloc Handler error\n");
        return NULL;
    }

    handler->saddr.sin_addr.s_addr = *(unsigned long *)host->h_addr;
    handler->saddr.sin_port = htons(port);
    handler->saddr.sin_family = AF_INET;
    handler->ssockfd = -1;
    handler->isconnected = TCP_SESSION_NOT_CONNECTED;
    handler->callback = callback;
    handler->callback2 = callback2;
    handler->reconnectTime = 30;
    handler->shouldClose = 0;
    handler->timeout = 1;

    tcp_client_connect(handler);

    handler->isalive = 1;
    pthread_create(&handler->thread, NULL, tcp_client_process, handler);

    return handler;
}

void tcp_client_destroy(void *handler)
{
    stClientHandler *tcphandler = (stClientHandler *)handler;
    
    if (NULL != tcphandler)
    {
        if (tcphandler->isalive)
        {
            tcphandler->isalive = 0;
            pthread_join(tcphandler->thread, NULL);
        }

        if (-1 != tcphandler->ssockfd)
            close(tcphandler->ssockfd);

        free(tcphandler);
    }

    //log_destory();
}

int tcp_server_send(void *tcpHandler, void *buff, int length)
{
    stClientInfo *client = (stClientInfo *)tcpHandler;
    if (NULL == client)
    {
        LOG_ERROR("client is NULL\n");
        return TCP_SESSION_ERR;
    }

    if (NULL == buff || 0 == length)
    {
        LOG_ERROR("send buff is null or length is 0\n");
        return TCP_SESSION_ERR;
    }

    send(client->clientfd, buff, length, 0);

    return TCP_SESSION_OK;
}

void tcp_server_remove_client(void *serverHandler, stClientInfo *client)
{
    stServerHandler *handler = (stServerHandler *)serverHandler;
    if (NULL == handler || NULL == client)
    {
        LOG_ERROR("serverHandler is NULL or client is NULL");
        return ;
    }

    pthread_mutex_lock(&handler->mutex);
    LIST_DELETE(&handler->clientList, &(client->entry));
    handler->clientcount--;
    pthread_mutex_unlock(&handler->mutex);

    close(client->clientfd);
    free(client);
}

void* tcp_server_listen_thread(void *serverHandler)
{
    stServerHandler *handler = (stServerHandler *)serverHandler;
    int maxfd = handler->listenfd;
    fd_set rset, allset;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    int connfd;
    int ready;
    char buff[1024];

    FD_ZERO(&allset);
    FD_SET(handler->listenfd, &allset);

    while (handler->isalive)
    {
        rset = allset;

        ready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(handler->listenfd, &rset))
        {
            clilen = sizeof(cliaddr);
            connfd = accept(handler->listenfd, (struct sockaddr *)&cliaddr, &clilen);
            if (0 <= connfd)
            {
                stClientInfo *client = (stClientInfo *) malloc(sizeof(stClientInfo));
                if (NULL == client)
                {
                    LOG_ERROR("accept client malloc error\n");
                    continue;
                }
                strcpy(client->clientip, inet_ntoa((struct in_addr)cliaddr.sin_addr));    //inet_ntop(AF_INET, &cliaddr.sin_addr, 4, NULL));
                client->clientport = ntohs(cliaddr.sin_port);
                client->clientfd = connfd;
                LOG_INFO("accept client:%s:%d, fd:%d\n", client->clientip, client->clientport, connfd);
                if (handler->a_callback)
                    handler->a_callback(client);

                pthread_mutex_lock(&handler->mutex);
                LIST_INSERT_TAIL(&(handler->clientList), &(client->entry));
                handler->clientcount++;
                pthread_mutex_unlock(&handler->mutex);
            
                FD_SET(connfd, &allset);
                if (connfd > maxfd)
                    maxfd = connfd;
            }

            if (--ready <= 0)
                continue;
        }

        stListEntry *entry, *next;
        LIST_FOREACH_NEXT(&handler->clientList, entry, next)
        {
            stClientInfo *client = container_of(entry, stClientInfo, entry);
            if (FD_ISSET(client->clientfd, &rset))
            {
                int ret = recv(client->clientfd, buff, 1024, 0);
                if (0 == ret)
                {
                    LOG_INFO("client offline %s:%d\n", client->clientip, client->clientport);
                    FD_CLR(client->clientfd, &allset);
                    tcp_server_remove_client(handler, client);
                }
                else if (0 < ret)
                {
                    LOG_INFO("receive buff len:%d\n", ret);
                    if (NULL != handler->callback)
                        handler->callback(handler, buff, ret);
                }

                if (--ready <= 0)
                    break;
            }
        }
    }
}

int tcp_server_addr_get(void *serverHandler, long *lip, unsigned short *lport)
{
    stServerHandler *handler = (stServerHandler *)serverHandler;

    *lip = ntohl(handler->laddr.sin_addr.s_addr);
    *lport = ntohs(handler->laddr.sin_port);

    return TCP_SESSION_OK;
}

void* tcp_server_init(char *saddr, unsigned short port, recv_callback callback, accept_callback aCallBack)
{
    stServerHandler *handler;
    int fd;
    struct sockaddr_in servaddr;
    int addrlen;

    //log_init(NULL);

    if (0 > (fd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        LOG_ERROR("Create Server Socket error\n");
        return NULL;
    }

    if (NULL == saddr)
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        servaddr.sin_addr.s_addr = inet_addr(saddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (0 > (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr))))
    {
        LOG_ERROR("Bind Error, errno:%d\n", errno);
        close(fd);
        return NULL;
    }

    if ( 0 > listen(fd, LISTEN_BACKLOG))
    {
        LOG_ERROR("Listen Error, errno:%d\n", errno);
        close(fd);
        return NULL;
    }

    addrlen = sizeof(struct sockaddr);
    int ret = getsockname(fd, (struct sockaddr *)(&servaddr), (socklen_t *)(&addrlen));
    if (0 != ret)
        LOG_ERROR("server Get Sock Name error, ret:%d, errno:%d\n", ret, errno);

    handler = (stServerHandler *)malloc(sizeof(stServerHandler));
    if (NULL == handler)
    {
        LOG_ERROR("Malloc Server Handler error\n");
        return NULL;
    }

    LIST_INIT(&(handler->clientList));
    pthread_mutex_init(&handler->mutex, NULL);
    handler->clientcount = 0;

    handler->listenfd = fd;
    handler->backlog = LISTEN_BACKLOG;
    handler->isalive = 1;
    handler->callback = callback;
    handler->a_callback = aCallBack;
    memcpy(&handler->laddr, &servaddr, sizeof(struct sockaddr_in));

    pthread_create(&handler->listenThread, NULL, tcp_server_listen_thread, handler);

    LOG_INFO("Server Start!\n");

    return handler;
}

void tcp_server_destroy(void *handler)
{
    stServerHandler *sHandler = (stServerHandler *)handler;

    if (NULL != sHandler)
    {
        stListEntry *entry, *next;
        LIST_FOREACH_NEXT(&(sHandler->clientList), entry, next)
        {
            stClientInfo *client = container_of(entry, stClientInfo, entry);
            tcp_server_remove_client(handler, client);
        }

        if (-1 != sHandler->listenfd)
            close(sHandler->listenfd);

        free(sHandler);
    }

    //log_destory();
}
