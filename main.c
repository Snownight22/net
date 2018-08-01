#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "log.h"
#include "tcp_session.h"

typedef struct clients
{
    stListEntry entry;
    void *handler;
}stClients;

int g_loop = 1;
stListHead g_clients_head;

void receive_data(const void *handler, const void *data, const int length)
{
    printf("hello, received!\n");
    printf("\nserver:%s\n", (char *)data);
    g_loop = 0;
}

void server_receive_data(const void *handler, const void *data, const int length)
{
    ((char*)data)[length] = 0;
    printf("\nclient say:%s\n>>", data);
}

void server_accept_client(const void *clientInfo)
{
    stClients *client = (stClients *) malloc(sizeof(stClients));
    if (NULL != client)
    {
        client->handler = (void *)clientInfo;
        LIST_INSERT_TAIL(&g_clients_head, &client->entry);
        LOG_INFO("\nserver accept a client\n>>");
    }
}

int main(int argc, char *argv[])
{
#ifdef CLIENT
    char *domain = "192.168.1.186";
    unsigned short port = 6666;
    stClientHandler *handler;
    int ret;
    char buff[128] = {0};

    log_init("client.ini");
    handler = (stClientHandler *)tcp_client_init(domain, port, receive_data);

    while (g_loop)
    {
        fputs("client say: ", stdout);
        fgets(buff, 127, stdin);
        tcp_client_send(handler, buff, strlen(buff));
        if (0 == ret)
            printf("send %s\n", buff);
        /*
        tcp_client_timeout_set(handler, 10);
        sleep(10);
        LOG_INFO("now close session\n");
        tcp_client_close_session(handler);
        break;
        */
    }

    tcp_client_destroy(handler);
    log_destory();
#endif

#ifdef SERVER
    unsigned short lport = 0;//6666;
    stServerHandler *serverHandler;
    char buff[128] = {0};

    log_init("server.ini");
    LIST_INIT(&g_clients_head);
    serverHandler = (stServerHandler *)tcp_server_init(NULL/*"192.168.1.186"*/, lport, server_receive_data, server_accept_client);

    while (g_loop)
    {
        printf(">>");
        fgets(buff, 127, stdin);
        stListEntry *entry;
        LIST_FOREACH(&g_clients_head, entry)
        {
            stClients *client = container_of(entry, stClients, entry);
            tcp_server_send(client->handler, buff, strlen(buff));
        }

        /*
        sleep(10);
        break;
        */
    }

    tcp_server_destroy(serverHandler);
    log_destory();
#endif

    return 0;
}

