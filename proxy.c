#include "csapp.h"
#include "echo.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 프록시가 요청을 받고 응답해줄 클라이언트의 IP, Port
  // char server_hostname[MAXLINE], server_port[MAXLINE]; // 프록시가 요청을 보낼 서버의 IP, Port
  char server_hostname[MAXLINE] = "43.201.51.191";
  char server_port[MAXLINE] = "8000";

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  listenfd = Open_listenfd(argv[1]);  // 대기 회선
  while (1) {
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 프록시가 서버로서 클라이언트와 맺는 파일 디스크립터(소켓 디스크립터) : 고유 식별되는 회선이자 메모리 그 자체
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    char buf[MAXLINE];

    rio_t rio;
    size_t n;
    int clientfd = Open_clientfd(server_hostname, server_port);
    // echo(connfd);
    Rio_readinitb(&rio, connfd);  // 클라이언트와 connection 시작

    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
      printf("server received %d bytes\n", (int)n);
      Rio_writen(connfd, buf, n);   // connfd 로 받은 내용을 buf에 witen 하기 
      Rio_readinitb(&rio, clientfd);
      while (Fgets(buf, MAXLINE, stdin) != NULL) {
          printf("proxy is now working\n");
          Rio_writen(clientfd, buf, strlen(buf));
          Rio_readlineb(&rio, buf, MAXLINE);
          Fputs(buf, stdout);
      }
      Close(clientfd);
    }
    Close(connfd);
    exit(0);
  }
  exit(0);
}
