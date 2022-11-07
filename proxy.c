#include "csapp.h"
#include "echo.c"

int parse_uri(char *uri, char *filename, char *cgiargs);
void get_filetype(char *filename, char *filetype);
void doit(int connfd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void make_response(int fd);

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

void doit(int connfd) {
    // char server_hostname[MAXLINE], server_port[MAXLINE]; // 프록시가 요청을 보낼 서버의 IP, Port
    char server_hostname[MAXLINE] = "43.201.51.191";
    char server_port[MAXLINE] = "8000";
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    rio_t rio1;     // 클라이언트와의 rio
    rio_t rio2;     // 서버와의 rio
    size_t n;
    int clientfd = Open_clientfd(server_hostname, server_port);
    // echo(connfd);
    Rio_readinitb(&rio1, connfd);  // 클라이언트와 connection 시작
    Rio_readinitb(&rio2, clientfd); // 서버와 connection 시작

    while ((n = Rio_readlineb(&rio1, buf, MAXLINE)) != 0) {
      // while (Fgets(buf, MAXLINE, stdin) != NULL) {
      sscanf(buf, "%s %s %s", method, uri, version);       // 클라이언트에서 받은 요청 파싱(method, uri, version 뽑아냄)
      printf("strcasecmp(method,'GET')=%d\n", strcasecmp(method,"GET"));
      printf("=====>  method=%s uri=%s version=%s\n", method, uri, version);

      printf("2.[I'm proxy] proxy -> server\n");
      Rio_writen(clientfd, buf, strlen(buf)); // 내가 서버에 req 보냄
      
      printf("4.[I'm proxy] server -> proxy\n");
      Rio_readlineb(&rio2, buf, MAXLINE);  // 서버의 res 받음 (헤더 받음)
      
      printf("bbbbbbbbbbuf=%s", buf);
      Rio_writen(connfd, buf, strlen(buf)); 

    // if (strcasecmp(method,"GET") && strcasecmp(method,"HEAD")) {     
    //     printf("[PROXY]501 ERROR\n");
    //     clienterror(connfd, method, "501", "Not Implemented",
    //             "Tiny does not implement this method");
    //     return;
    //  }  

      printf("should respond\n");
      // int srcfd;
      // char *srcp, filetype[MAXLINE];

      // srcfd = open("./temp_home.html", O_WRONLY | O_CREAT | O_APPEND, MAXLINE);
      // get_filetype("./temp_home.html", filetype);
      // // Rio_writen(connfd, buf, strlen(buf));       //line:netp:servestatic:endserve

      // srcfd = Open("./temp_home.html", O_RDONLY, 0);
      // srcp = (char *)malloc(MAXLINE);
      // Rio_readn(srcfd, srcp, MAXLINE);
      // Rio_writen(connfd, srcp, n);   // connfd 로 받은 내용을 buf에 witen 하기 
      // free(srcp);



      printf("5.[I'm proxy] proxy -> client\n");
      // Rio_writen(connfd, buf, n);   // connfd 로 받은 내용을 buf에 witen 하기 
      make_response(connfd);  
      Fputs(buf, stdout);
      // }
    }
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	    strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	    strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	    strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	    strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mpg"))
	    strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".mp4"))
	    strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".mov"))
	    strcpy(filetype, "video/mp4");
    else
	    strcpy(filetype, "text/plain");
}  


/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    printf("[server]parse_uri\n");
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
	    strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
	    strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
	    strcat(filename, uri);                           //line:netp:parseuri:endconvert1
	    if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
	      strcat(filename, "home.html");               //line:netp:parseuri:appenddefault  
      
      printf("filename=%s\n", filename);
	    return 1;
    }
    else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
	    ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
	    if (ptr) {  
	      strcpy(cgiargs, ptr+1);
	      *ptr = '\0';
	    }
      else 
        strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
      strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
      strcat(filename, uri);                           //line:netp:parseuri:endconvert2
      return 0;
    }
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

void make_response(int fd) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Server Proxy</title>");
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