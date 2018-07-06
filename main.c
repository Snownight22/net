#include <stdio.h>
#include <string.h>
#include "tcp_session.h"

int g_loop = 1;

void receive_data(const void *handler, const void *data, const int length)
{
    printf("hello, received!\n");
    g_loop = 0;
}

void server_receive_data(const void *handler, const void *data, const int length)
{
    printf("client say:%s\n", data);
}

int main(int argc, char *argv[])
{
#ifdef CLIENT
    char *domain = "192.168.1.186";
    unsigned short port = 6666;
    stClientHandler *handler;
    int ret;
    char buff[128] = {0};

    handler = (stClientHandler *)tcp_client_init(domain, port, receive_data);

    while (g_loop)
    {
        fputs("client say: ", stdout);
        fgets(buff, 127, stdin);
        tcp_client_send(handler, buff, strlen(buff));
        if (0 == ret)
            printf("send %s\n", buff);
        //sleep(10);
    }

    tcp_client_destroy(handler);
#endif

#ifdef SERVER
    unsigned short lport = 6666;
    stServerHandler *serverHandler;

    serverHandler = (stServerHandler *)tcp_server_init(NULL/*"192.168.1.186"*/, lport, server_receive_data, NULL);

    while (g_loop)
    {
        sleep(1);
    }

    tcp_server_destroy(serverHandler);
#endif

    return 0;
}

