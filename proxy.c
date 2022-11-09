#include "csapp.h"
#include "hash.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define VERBOSE 1
#define CONCURRENCY 2 // 0: 시퀀셜, 1: 멀티스레드, 2: 멀티프로세스

// struct cache_store {
//   // char hostname[MAXLINE];
//   // char port[MAXLINE];
//   // char path[MAXLINE];
//   char url[MAXLINE];  // 같은 url에 대한 요청인지 strcmp로 비교 예정
//   char storage[MAX_CACHE_SIZE];
// };


void doit(int connfd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void parse_uri(char *uri,char *hostname,char *path,int *port);
int make_request(rio_t* client_rio, char *hostname, char *path, int port, char *hdr, char *method);
#if CONCURRENCY == 1 
void *thread(void *vargp);  // Pthread_create 에 루틴 반환형이 정의되어있음
#endif

/* You won't lose style points for including this long line in your code */
// https://developer.mozilla.org/ko/docs/Glossary/Request_header
static const char *request_hdr_format = "%s %s HTTP/1.0\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *Accept_hdr = "    Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *EOL = "\r\n";

int main(int argc, char **argv) {
  int listenfd, *clientfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 프록시가 요청을 받고 응답해줄 클라이언트의 IP, Port
  pthread_t tid;  // 스레드에 부여할 tid 번호 (unsigned long)

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  listenfd = Open_listenfd(argv[1]);  // 대기 회선
  while (1) {
    clientlen = sizeof(struct sockaddr_storage);
    clientfd = (int *)Malloc(sizeof(int));   // 여러개의 디스크립터를 만들 것이므로 덮어쓰지 못하도록 고유메모리에 할당
    *clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 프록시가 서버로서 클라이언트와 맺는 파일 디스크립터(소켓 디스크립터) : 고유 식별되는 회선이자 메모리 그 자체

    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    
    if (VERBOSE) {
      printf("Connected to (%s, %s)\n", client_hostname, client_port);
    }

  #if CONCURRENCY == 0
      doit(*clientfd);
      Close(*clientfd);
  
  #elif CONCURRENCY == 1
      Pthread_create(&tid, NULL, thread, clientfd);
  
  #elif CONCURRENCY == 2 
    if (Fork() == 0) {
      Close(listenfd);
      doit(*clientfd);
      Close(*clientfd);
      exit(0);
    }
    Close(*clientfd);
  #endif
  }
  return 0;
}

#if CONCURRENCY == 1 
  void *thread(void *argptr) {
    int clientfd = *((int *)argptr);
    Pthread_detach((pthread_self()));
    Free(argptr);
    doit(clientfd);
    Close(clientfd);
    return NULL;
  }
#endif

void doit(int client_fd) {
    char hostname[MAXLINE], path[MAXLINE];  // 프록시가 요청을 보낼 서버의 hostname, 파일경로
    int port;
    
    char buf[MAXLINE], hdr[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    int server_fd;

    rio_t client_rio;     // 클라이언트와의 rio
    rio_t server_rio;     // 서버와의 rio

    Rio_readinitb(&client_rio, client_fd);  // 클라이언트와 connection 시작
    Rio_readlineb(&client_rio, buf, MAXLINE);  // 클라이언트의 요청(한줄) 받음 
    sscanf(buf, "%s %s %s", method, uri, version);       // 클라이언트에서 받은 요청 파싱(method, uri, version 뽑아냄)

    if (strcasecmp(method,"GET") && strcasecmp(method,"HEAD")) {     
      // 501 요청은 프록시 선에서 처리 
        if (VERBOSE) {
          printf("[PROXY]501 ERROR\n");
        }
        clienterror(client_fd, method, "501", "잘못된 요청",
                "501 에러. 올바른 요청이 아닙니다.");
        return;
    } 

    parse_uri(uri, hostname, path, &port); // req uri 파싱하여 hostname, path, port(포인터) 변수에 할당
    
    if (VERBOSE) {
      printf("[out]hostname=%s port=%d path=%s\n", hostname, port, path);
    }
    if (!strlen(hostname)) {
      if (VERBOSE) {
        printf("[PROXY]501 ERROR\n");
      }
      clienterror(client_fd, method, "501", "잘못된 요청",
                "501 에러. 올바른 요청이 아닙니다.");
    }
    if (!make_request(&client_rio, hostname, path, port, hdr, method)) {
      if (VERBOSE) {
        printf("[PROXY]501 ERROR\n");
      }
      clienterror(client_fd, method, "501", "잘못된 요청",
                "501 에러. 올바른 요청이 아닙니다.");
    }


    printf("caching!!!\n");
      //  char url[100] = "abc";
    // char value[100] ="1111";
    
    // install(url, value);
    struct nlist *cached = malloc(sizeof (struct nlist));
    // cached->name = malloc(sizeof(char *))
    // cached->defn = malloc(sizeof(char *))

    cached = lookup(uri);
    if (cached) {
      printf("\n\nname=%s\n", cached->name);
      printf("\n\ndefn=%s\n", cached->defn);
    }

    printf("\ndone\n");

    char port_value[100];
    sprintf(port_value,"%d",port);
    server_fd = Open_clientfd(hostname, port_value); // 서버와의 소켓 디스크립터 생성

    Rio_readinitb(&server_rio, server_fd);  // 서버 소켓과 연결
    Rio_writen(server_fd, hdr, strlen(hdr)); // 서버에 req 보냄

    size_t n;
    while ((n=Rio_readnb(&server_rio, buf, MAXLINE)) > 0) {
      printf("sending buf = %s\n", buf);
      install(uri, buf);
      Rio_writen(client_fd, buf, n);   // 클라이언트에게 응답 전달
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
   알맞게 파싱해서 hostname, port로, path 나누어주어야 한다!
  */

  *port = 80;
  if (VERBOSE) {
    printf("uri=%s\n", uri);
  }
  
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
  if (VERBOSE) {
    printf("hostname=%s port=%d path=%s\n", hostname, *port, path);
  }
}

int make_request(rio_t* client_rio, char *hostname, char *path, int port, char *hdr, char *method) {
  // 프록시서버로 들어온 요청을 서버에 전달하기 위해 HTTP 헤더 생성
  char req_hdr[MAXLINE], additional_hdr[MAXLINE], host_hdr[MAXLINE];
  char buf[MAXLINE];
  char *HOST = "Host";
  char *CONN = "Connection";
  char *UA = "User-Agent";
  char *P_CONN = "Proxy-Connection";
  sprintf(req_hdr, request_hdr_format, method, path); // method url version

  while (1) {
    if (Rio_readlineb(client_rio, buf, MAXLINE) == 0) break;
    if (!strcmp(buf,EOL)) break;  // buf == EOL => EOF

    if (!strncasecmp(buf, HOST, strlen(HOST))) {
      // 호스트 헤더 지정
      strcpy(host_hdr, buf);
      continue;
    }

    if (strncasecmp(buf, CONN, strlen(CONN)) && strncasecmp(buf, UA, strlen(UA)) && strncasecmp(buf, P_CONN, strlen(P_CONN))) {
       // 미리 준비된 헤더가 아니면 추가 헤더에 추가 
      strcat(additional_hdr, buf);  
      strcat(additional_hdr, "\r\n");  
    }
  }

  if (!strlen(host_hdr)) {
    sprintf(host_hdr, host_hdr_format, hostname);
  }

  sprintf(hdr, "%s%s%s%s%s%s", 
    req_hdr,   // METHOD URL VERSION
    host_hdr,   // Host header
    user_agent_hdr,
    connection_hdr,
    proxy_connection_hdr,
    EOL
  );
  if (strlen(hdr))
    return 1;
  return 0;
}