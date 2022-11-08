#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        printf("1. [I'm client] client -> proxy\n");
        Rio_writen(clientfd, buf, strlen(buf));         // 서버에 req 보냄

        printf("6.[I'm client] proxy -> client\n");
        Rio_readlineb(&rio, buf, MAXLINE);              // 서버의 res 받음
        printf("srcp=%s\n", buf);

        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}