#include "csapp.h"
#include "echo.c"

void doit(int connfd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void parse_uri(char *uri,char *hostname,char *path,int *port);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *eol_hdr = "\r\n";

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 프록시가 요청을 받고 응답해줄 클라이언트의 IP, Port

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
    doit(connfd); // 프록시가 중개를 시작
    Close(connfd);
  }
  return 0;
}

void doit(int client_fd) {
    char hostname[MAXLINE], path[MAXLINE];  // 프록시가 요청을 보낼 서버의 hostname, 파일경로
    int port = 80;    // 서버 포트 80 고정 
    
    char buf[MAXLINE], hdr[MAXLINE], new_request[MAXBUF], response[1<<15];;
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    int server_fd;

    rio_t client_rio;     // 클라이언트와의 rio
    rio_t server_rio;     // 서버와의 rio

    Rio_readinitb(&client_rio, client_fd);  // 클라이언트와 connection 시작
    Rio_readlineb(&client_rio, buf, MAXLINE);

    sscanf(buf, "%s %s %s", method, uri, version);       // 클라이언트에서 받은 요청 파싱(method, uri, version 뽑아냄)

    if (strcasecmp(method,"GET") && strcasecmp(method,"HEAD")) {     
        printf("[PROXY]501 ERROR\n");
        clienterror(client_fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
        return;
    } 

    parse_uri(uri, hostname, path, port);

    server_fd = Open_clientfd(hostname, port);

    Rio_readinitb(&server_rio, server_fd);

    printf("2.[I'm proxy] proxy -> server\n");
    Rio_writen(server_fd, new_request, strlen(new_request)); // 서버에 req 보냄

    size_t n;
    while ((n=Rio_readlineb(&server_rio, buf, MAXLINE)) !=0) {
      printf("4.[I'm proxy] server -> proxy\n");  // 서버에서 응답 받음

      printf("5.[I'm proxy] proxy -> client\n");
      Rio_writen(client_fd, buf, n);   // 클라이언트에게 응답 전달
      Close(client_fd);
    }
    Close(server_fd);
}


/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

void parse_uri(char *uri,char *hostname, char *path, int *port) {
  /*
   uri가  
   / , /cgi-bin/adder 이렇게 들어올 수도 있고,
   http://11.22.33.44:5001/home.html 이렇게 들어올 수도 있다.

   알맞게 파싱해서 hostname, path, port로 나누어주어야 한다!
  */

  *port = 80;

  printf("uri=%s\n", uri);
  
  char *parsed;
  parsed = strstr(uri, "//");

  if (parsed == NULL) {
    parsed = uri;
  }
  else {
    parsed = parsed + 2;  // 포인터 두칸 이동 
  }
  char *parsed2 = strstr(parsed, ":");

  if (parsed2 == NULL) {
    // ':' 이후가 없다면, port가 없음
    parsed2 = strstr(parsed, "/");
    if (parsed2 == NULL) {
      sscanf(parsed,"%s",hostname);
    } 
    else {
        *parsed2 = '\0';
        sscanf(parsed,"%s",hostname);
        *parsed2 = '/';
        sscanf(parsed2,"%s",path);
    }

  } else {
      // ':' 이후가 있으므로 port가 있음
      *parsed2 = '\0';
      sscanf(parsed, "%s", hostname);
      sscanf(parsed2+1, "%d%s", port, path);
  }
  printf("hostname=%s port=%d path=%s\n", hostname, port, path);
}