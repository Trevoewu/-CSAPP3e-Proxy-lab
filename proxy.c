#include "csapp.h"
#include "linklist.c"
#include "linklist.h"
#include "sbuf.c"
#include "sbuf.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define BUFSIZE 16
#define NTHREADS 4
#define MAXENTRY 16
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
void *thread_server(void *argp);
void cache_init();
void *thread_replace(void *argp);
void read_header(rio_t *rio);
void doit(int fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void request_forward(rio_t *rio, int clientfd, char *serverHost,
                     char *serverPort, char *path);
void parse_url(char *url, char *host, char *port, char *uri);
void *thread_evict(void *argp);
void showCache();
void init_block(int blockFd);
int isBlocked(char *url);
sbuf_t sbuf;
linkList cache;
struct rk_sema mutex; // protect cache
char blockList[MAXENTRY][MAXLINE];
int main(int argc, char **argv) {
  // 1. open client by argv[1]
  if (argc != 2) {
    printf("USAGE: ./proxy <port>\n");
    exit(0);
  }
  // initial semaphore
  rk_sema_init(&mutex, 1);
  // open listening port
  int listenfd = open_listenfd(argv[1]);
  int clientfd;
  struct sockaddr_storage clientaddr;
  socklen_t clientlen = sizeof(clientaddr);
  char hostname[MAXLINE];
  char port[MAXLINE];
  char logBuf[MAXLINE];
  if (listenfd <= 0) {
    printf("open listening error:");
    exit(0);
  }
  // log sys
  // flag: O_RDWR(read and write permissions) O_CREAT(creat file if file name
  // not exist) O_APPEND(set file position to end of file)
  int logFd = Open("log.list", O_RDWR | O_CREAT | O_APPEND, 0);
  // block list file, request url that contained in it will be blocked
  int blockFd = Open("block.list", O_RDONLY | O_CREAT, 0);
  // init sbuf structure
  init_sbuf(&sbuf, BUFSIZE);
  // initial cache
  cache_init();
  // initial block list file
  init_block(blockFd);
  //  signal pipe handle
  if (Signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    unix_error("mask signal pipe error");
  }

  //  creat thread of evicting
  pthread_t tid;
  Pthread_create(&tid, NULL, thread_evict, NULL);
  // - create N thread to handle request
  for (int i = 0; i < NTHREADS; i++) {
    pthread_t tid;
    Pthread_create(&tid, NULL, thread_server, NULL);
  }
  while (1) {
    // 2.listening in listen port
    clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    // record log info into log file
    sprintf(logBuf, "Main [INFO] :receive form (%s: %s) client %d\n", hostname,
            port, clientfd);
    Write(logFd, logBuf, strlen(logBuf));
    // 3. add to buff
    if (clientfd > 0) {
      insert_sbuf(&sbuf, clientfd);
    }
  }
}
void read_header(rio_t *rio) {
  int n;
  char buf[MAXLINE];
  if ((n = Rio_readlineb(rio, buf, MAXLINE)) > 0) {
    printf("%s", buf);
  }
  // check read a entire request line
  while (strcmp(buf, "\r\n")) { // line:netp:readhdrs:checkterm
    Rio_readlineb(rio, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
/*
    handle request thread
*/
void *thread_server(void *argp) {
  // detach itself
  Pthread_detach(pthread_self());
  while (1) {
    // custom form buf
    int clientfd = remove_sbuf(&sbuf);
    doit(clientfd);
    showCache();
    close(clientfd);
  }
  return NULL;
}

void doit(int fd) {
  rio_t rio;
  char method[MAXLINE];
  char path[MAXLINE];
  char version[MAXLINE];
  char buf[MAXLINE];
  char host[MAXLINE];
  char port[MAXLINE];
  Rio_readinitb(&rio, fd);
  // -judge if a HTTP request
  if (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
    sscanf(buf, "%s %s %s", method, path, version);
  }
  printf("url->%s\n", path);
  if (strcasecmp(version, "HTTP/1.1") && strcasecmp("HTTP/1.0", version)) {
    clienterror(fd, version, "501", "Not is HTTP request",
                "Not A HTTP request, proxy only handle HTTP protocol");
    return;
  }
  // judge method
  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not Implemented",
                "proxy does not implement this method");
    return;
  }
  // judge if url is blocked
  if (isBlocked(path)) {
    clienterror(fd, path, "403", "403 Forbidden", "This URL is Blocked!");
    return;
  }
  // handle path string
  parse_url(path, host, port, path);
  // judge if cached
  char key[MAXLINE] = "";
  strcat(key, host);
  strcat(key, port);
  strcat(key, path);
  printf("key->%s\n", key);
  Node *node = getNode(cache, key);
  if (node) {
    printf("doit: obj cached!\n");
    // cache hit!
    Rio_writen(fd, node->value, node->size);
    // move to top
    // when cache hit, move hit entry to top, by this way, implementing
    // LRU(Least Recent Use) Strategy.
    moveTop(cache, node);
    return;
  }
  //  forward to server
  printf("doit: no cache!\n");
  request_forward(&rio, fd, host, port, path);
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE];

  /* Print the HTTP response headers */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* Print the HTTP response body */
  sprintf(buf, "<html><title>Tiny Error</title>");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<body bgcolor="
               "ffffff"
               ">\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
  Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */
/*
  forward request to server
  rio: read request header from client
  clientfd: client descriptor,  forward server response to it
  serverHost,
  serverPort: connect to server
*/
void request_forward(rio_t *rio, int clientfd, char *serverHost,
                     char *serverPort, char *path) {
  char buf[MAXLINE];

  int serverfd = Open_clientfd(serverHost, serverPort);
  // prepare header

  sprintf(buf, "GET %s HTTP/1.0\r\n", path);
  Rio_writen(serverfd, buf, strlen(buf));
  sprintf(buf, "Host: %s\r\n", serverHost);
  Rio_writen(serverfd, buf, strlen(buf));
  strcpy(buf, user_agent_hdr);
  Rio_writen(serverfd, buf, strlen(buf));
  sprintf(buf, "Connection: close\r\n");
  Rio_writen(serverfd, buf, strlen(buf));
  sprintf(buf, "Proxy-Connection: close\r\n");
  Rio_writen(serverfd, buf, strlen(buf));
  // rest of client past header
  do {
    Rio_readlineb(rio, buf, MAXLINE);
    // ignore this Header
    if (strstr(buf, "Host") || strstr(buf, "Connection") ||
        strstr(buf, "Proxy-Connection") || strstr(buf, "User-Agent")) {
      continue;
    }
    // send to server
    Rio_writen(serverfd, buf, strlen(buf));
  } while (strcmp(buf, "\r\n"));
  // writen a \r\n as end of header
  Rio_writen(serverfd, buf, strlen(buf));
  // receive form server and forward to client
  rio_t rio_server;
  size_t n_byte;
  size_t sum_byte = 0;
  char p[MAX_OBJECT_SIZE];
  Rio_readinitb(&rio_server, serverfd);

  while ((n_byte = Rio_readlineb(&rio_server, buf, MAXLINE))) {
    // store into p
    sum_byte += n_byte;
    if (sum_byte < MAX_OBJECT_SIZE)
      strcat(p, buf);
    // sent to client
    Rio_writen(clientfd, buf, strlen(buf));
  }
  write(1, p, n_byte);
  if (sum_byte < MAX_OBJECT_SIZE) {
    char *value = (char *)Malloc(sum_byte);
    strncpy(value, p, sum_byte);
    // generate key
    char key[MAXLINE] = "";
    strcat(key, serverHost);
    strcat(key, serverPort);
    strcat(key, path);
    Node *node = creatNode(key, value);
    node->size = sum_byte;
    // lock
    rk_sema_wait(&mutex);
    put(cache, node);
    // unlock
    rk_sema_post(&mutex);
  }
  return;
}
/*
  convert URL into host,port ,and path
*/
void parse_url(char *url, char *host, char *port, char *path) {
  char *ptr;
  ptr = strstr(path, "//");
  if (ptr) {
    // get host
    ptr += 2;
    strcpy(host, ptr);

    ptr = strstr(host, ":");
    if (ptr) {
      // add terminated character NULL('\0')
      *ptr = '\0';
      strcpy(port, ptr + 1);
      // get port
      ptr = strstr(port, "/");
      if (ptr) {
        strcpy(path, ptr);
        *ptr = '\0';
      }
    }
    // implicit port in URL, Default 80
    else {
      strncpy(port, "80", MAXLINE);

      ptr = strstr(host, "/");
      if (ptr) {
        // get path(uri)
        strcpy(path, ptr);
        *ptr = '\0';
      }
    }
  }
  printf("parse url: path->%s host->%s port->%s\n", path, host, port);
}
void cache_init() { listInit(&cache); }
void *thread_evict(void *argp) {
  Pthread_detach(pthread_self());
  Node *p;
  while (1) {
    u_long totalSize = getTotalSize(cache, &p);
    if (totalSize > MAX_CACHE_SIZE) {
      free(p);
    }
  }
  return NULL;
}
void showCache() {
  Node *pNode = cache->next;
  while (pNode) {
    printf("(%s,%p)", pNode->key, pNode->value);
    pNode = pNode->next;
  }
  printf("\n");
}
void init_block(int blockFd) {
  memset(blockList, MAXENTRY * MAXLINE, '\0');
  rio_t rio;
  char buf[MAXLINE];
  Rio_readinitb(&rio, blockFd);
  int num = 0;
  int n_bytes = 0;
  while ((n_bytes = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
    if (!strncasecmp(buf, "http://", strlen("http://"))) {
      strncpy(blockList[num], buf, n_bytes);
      printf("block url->%s\n", blockList[num]);
      num++;
    }
    if (num == MAXENTRY) {
      return;
    }
  }
  close(blockFd);
}
int isBlocked(char *url) {
  char buf[MAXLINE];
  for (int i = 0; i < MAXENTRY && blockList[i][0] != '\0'; i++) {
    if (!strncasecmp(blockList[i], url, strlen(url))) {
      return 1;
    }
  }
  return 0;
}