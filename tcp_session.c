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
#include "log.h"
#include "tcp_session.h"

int tcp_client_connect(stClientHandler *handler)
{
    if (-1 == (connect(handler->ssockfd, (struct sockaddr*)(&handler->saddr), sizeof(handler->saddr))))
    {
        LOG_ERROR("Connect Server Fail!\n");
        handler->isconnected = TCP_SESSION_NOT_CONNECTED;
        return TCP_SESSION_ERR;
    }

    LOG_INFO("Connected to server %s\n", inet_ntoa(handler->saddr.sin_addr));
    handler->isconnected = TCP_SESSION_CONNECTED;

    return TCP_SESSION_OK;
}

int tcp_client_send(void *tcpHandler, void *buff, int length)
{
    stClientHandler *handler = (stClientHandler *)tcpHandler;

    if ((TCP_SESSION_NOT_CONNECTED == handler->isconnected) || (-1 == handler->ssockfd))
        return TCP_SESSION_ERR;

    send(handler->ssockfd, buff, length, 0);
    return TCP_SESSION_OK;
}

void* tcp_client_process(void*arg)
{
    stClientHandler *handler = (stClientHandler *)arg;
    int ret;
    char buff[1024];
    fd_set rsets, wsets;

    FD_ZERO(&rsets);
    FD_ZERO(&wsets);
    while (handler->isalive)
    {
        if (TCP_SESSION_NOT_CONNECTED == handler->isconnected)
            tcp_client_connect(handler);
        if (TCP_SESSION_NOT_CONNECTED == handler->isconnected)
        {
            sleep(handler->reconnectTime);
            continue;
        }

        FD_SET(handler->ssockfd, &rsets);
        //FD_SET(handler->ssockfd, &wsets);
        select(handler->ssockfd+1, &rsets, NULL/*&wsets*/, NULL, NULL);
        if (FD_ISSET(handler->ssockfd, &rsets))
        {
            ret = recv(handler->ssockfd, buff, 1024, 0);
            if (0 == ret)
            {
                handler->isconnected == 0;
                continue;
            }
            else if (0 < ret)
            {
                LOG_INFO("receive buff len:%d\n", ret);
                if (NULL != handler->callback)
                    handler->callback(handler, buff, ret);
            }
        }

        /*
        if (FD_ISSET(handler->ssockfd, &wsets))
        {
            char *sendbuff = "hello,world";
            send(handler->ssockfd, sendbuff, strlen(sendbuff), 0);
            LOG_INFO("send buff\n");
            sleep(20);
        }
        */
    }

}

void* tcp_client_init(char *domain, unsigned short port, recv_callback callback)
{
    struct hostent *host;
    int fd;
    stClientHandler *handler;

    log_init();

    host = gethostbyname(domain);
    if (NULL == host)
    {
        LOG_ERROR("Get Host Ip error\n");
        return NULL;
    }

    if (0 > (fd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        LOG_ERROR("Create Socket error\n");
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
    handler->ssockfd = fd;
    handler->isconnected = TCP_SESSION_NOT_CONNECTED;
    handler->callback = callback;
    handler->reconnectTime = 30;

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

    log_destory();
}

int tcp_server_send(void *tcpHandler, void *buff, int length)
{
    stClientInfo *client = (stClientInfo *)tcpHandler;

    send(client->clientfd, buff, length, 0);

    return TCP_SESSION_OK;
}

void tcp_server_remove_client(void *serverHandler, stClientInfo *client)
{
    stServerHandler *handler = (stServerHandler *)serverHandler;

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

void* tcp_server_init(char *saddr, unsigned short port, recv_callback callback, accept_callback aCallBack)
{
    stServerHandler *handler;
    int fd;
    struct sockaddr_in servaddr;

    log_init();

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
        return NULL;
    }

    if ( 0 > listen(fd, LISTEN_BACKLOG))
    {
        LOG_ERROR("Listen Error, errno:%d\n", errno);
        return NULL;
    }

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

    pthread_create(&handler->listenThread, NULL, tcp_server_listen_thread, handler);
    LOG_INFO("Server Start!\n");

    return handler;
}

void tcp_server_destroy(void *handler)
{
    stServerHandler *sHandler = (stServerHandler *)handler;

    if (NULL != sHandler)
    {
        if (-1 != sHandler->listenfd)
            close(sHandler->listenfd);

        free(sHandler);
    }

    log_destory();
}
