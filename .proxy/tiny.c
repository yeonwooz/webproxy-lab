/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"
#include "echo.c"
#define VERBOSE 0

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char* method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char* method);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 2) {
      fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
      clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
	    doit(connfd);  
      Close(connfd);                                   
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
  
    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);              
    sscanf(buf, "%s %s %s", method, uri, version);

    if (VERBOSE) {
      printf("Request headers:\n");
      printf("%s\n", buf);
    }

    if (strcasecmp(method,"GET") && strcasecmp(method,"HEAD")) {     
        if (VERBOSE) {
          printf("501 ERROR\n");
        } 
        clienterror(fd, method, "501", "잘못된 요청",
                "501 에러. 올바른 요청이 아닙니다.");
        return;
    }                                                 

    read_requesthdrs(&rio);                            
    if (VERBOSE) {
      printf("[server]starts parse\n");
    } 
    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);       //line:netp:doit:staticcheck
    if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
      if (VERBOSE) {
        printf("404 ERROR\n");
      } 
      clienterror(fd, filename, "404", "Not found", "요청하신 파일을 찾을 수 없습니다.");
	    return;
    }                                                    

    if (is_static) { /* Serve static content */          

    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 
        clienterror(fd, filename, "403", "Forbidden",
        "파일을 실행할 수 없습니다.");
        return;
    }
	  serve_static(fd, filename, sbuf.st_size, method);        //line:netp:doit:servestatic
    }
    else { /* Serve dynamic content */
      if (VERBOSE) {
        printf("[server]is_dynamic\n");
      } 

      if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
	      clienterror(fd, filename, "403", "Forbidden", "요청하신 파일을 실행할 수 없습니다.");
	    return;
	  }
	  serve_dynamic(fd, filename, cgiargs, method);          
  }
}
/* $end doit */

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	    Rio_readlineb(rp, buf, MAXLINE);
      if (VERBOSE) {
	      printf("%s", buf);
      } 
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ 
	    strcpy(cgiargs, "");                          
	    strcpy(filename, ".");                        
	    strcat(filename, uri);                           
	    if (uri[strlen(uri)-1] == '/')                  
      {
        strcat(filename, "home.html"); 
        printf("%s\n", filename);
      }
      else {
        uri = uri + strlen(uri)-5;
        printf("strcasecmp(uri, 'adder')=%d\n", strcasecmp(uri, "adder"));
        if (!strcasecmp(uri, "adder")) {
          strcat(filename, ".html");
          printf("%s\n", filename);
 
        }
      }         
      if (VERBOSE) {
        printf("filename=%s\n", filename);
      } 
	    return 1;
    }
    else {  /* Dynamic content */                      
	    ptr = index(uri, '?');                        
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
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize, char* method)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    
    /* Send response headers to client */
    get_filetype(filename, filetype);     
    sprintf(buf, "HTTP/1.0 200 OK\r\n");   
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    if (VERBOSE) {
      printf("Response headers:\n");
      printf("serve_static buffffff=>%s", buf);
    }      

    if (!strcasecmp(method, "HEAD")){
      // HEAD  메서드로 들어왔다면 스트링이 일치하여 0
      return;
    }

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);   

    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);//line:netp:servestatic:mmap
    srcp = (char *)malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);

    if (VERBOSE) {
      printf("3. [I'm server] server -> proxy\n");
      printf("srcp=%s, filesize=%d\n", srcp, filesize);
    }
    Rio_writen(fd, srcp, filesize);         
    // Close(srcfd);                           
    // Munmap(srcp, filesize);                
    free(srcp);
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
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs, char* method) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (!strcasecmp(method,"HEAD")){
      return;
    }

    if (Fork() == 0) { /* child */ 
      /* Real server would set all CGI vars here */
      setenv("QUERY_STRING", cgiargs, 1); 
      Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
      Execve(filename, emptylist, environ); /* Run CGI program */ 
    }
    Wait(NULL); /* Parent waits for and reaps child */ 
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    if (VERBOSE) {
      printf("client error\n");
    }
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