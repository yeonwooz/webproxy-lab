#include "csapp.h"
#include "echo.c"

struct req_content {
	char host[MAXLINE];
	char path[MAXLINE];
	int port;
};

void doit(int connfd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void make_response(int fd, char *buf);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

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
  }
  return 0;
}

void doit(int connfd) {
    // char server_hostname[MAXLINE], server_port[MAXLINE]; // 프록시가 요청을 보낼 서버의 IP, Port
    char server_hostname[MAXLINE] = "43.201.51.191";
    char server_port[MAXLINE] = "8000";
    char buf[MAXLINE], hdr[MAXLINE], new_request[MAXBUF], response[1<<15];;
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    struct req_content content;

    rio_t rio1;     // 클라이언트와의 rio
    rio_t rio2;     // 서버와의 rio
    size_t n;

    Rio_readinitb(&rio1, connfd);  // 클라이언트와 connection 시작
    Rio_readlineb(&rio1, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);       // 클라이언트에서 받은 요청 파싱(method, uri, version 뽑아냄)

    if (strcasecmp(method,"GET") && strcasecmp(method,"HEAD")) {     
        printf("[PROXY]501 ERROR\n");
        clienterror(connfd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
        return;
    } 
    parse_uri(uri, &content);
		read_requesthdrs(&rio1, hdr);

		sprintf(new_request, "GET %s HTTP/1.0\r\n", content.path);
    strcat(new_request, hdr);
		strcat(new_request, user_agent_hdr);
		strcat(new_request, accept_hdr);
		strcat(new_request, accept_encoding_hdr);
		strcat(new_request, connection_hdr);
		strcat(new_request, proxy_conn_hdr);
		strcat(new_request, "\r\n");

    int clientfd = Open_clientfd(server_hostname, server_port);

    printf("2.[I'm proxy] proxy -> server\n");
    Rio_writen(clientfd, new_request, strlen(new_request)); // 서버에 req 보냄
    
    printf("4.[I'm proxy] server -> proxy\n");
		Rio_readlineb(clientfd, response, sizeof(response));
    Close(clientfd);

    printf("5.[I'm proxy] proxy -> client\n");

    // make_response(connfd, new_request);                 
	  Rio_writen(connfd, response, sizeof(response));

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


void make_response(int fd, char *contents) 
{
    char buf[MAXLINE], body[MAXBUF];

    printf("contents===%s", contents);

    /* Build the HTTP response body */
    sprintf(body, "<html><title>[PROXY]Tiny Error-make_response</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

