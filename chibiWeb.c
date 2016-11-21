#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "chibiWeb.h"
#include "request.h"
#include "response.h"
#include "chibiWebDefs.h"
#include "tsQueue.h"

#define POOL_SIZE 4
#define BUFSIZE 1000

typedef struct pHandle {
  char* path;
  Handler handler;
} PathHandle;

Handler h = NULL;
TSQueue *filePaths, *paths, *workQ;

void chibi_init() {
  filePaths = tsq_create();
  paths = tsq_create();
  workQ = tsq_create();
}

Response *serveFile(Request *r) {
  off_t file_size;
  struct stat stbuf;
  int fd = open(r->file, O_RDONLY);
  printf("SERVING FILE: %s\n", r->file);
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {}
  close(fd);
  /* Handle error */

  file_size = stbuf.st_size;
  Response *resp = response_new_file(200, 0, file_size);
  resp->file = 1;
  resp->fileLen = file_size;
  return resp;
}

int serve(char *path, Handler handler, TSQueue *tsq) {
  PathHandle *p = (PathHandle *) malloc(sizeof(PathHandle));
  if (p == NULL) return 0;
  p->path = strdup(path);
  p->handler = handler;
  if (tsq_put(tsq, p)) return 1;
  free(p->path);
  free(p);
  return 0;
}

int chibi_serve(char *path, Handler handler) {
  return serve(path, handler, paths);
}

int chibi_serveFiles(char *path, char *localPath) {
  return serve(path, serveFile, filePaths);
}

int find_path(void *a, void *b) {
  char *path = (char *) a;
  PathHandle *phb = (PathHandle *) b;
  return !strcmp(path, phb->path);
}

int transferFile(int clientfd, char *filename) {
  printf("Transferring file: %s\n", filename);
  int fd = open(filename, O_RDONLY);
  if (fd < 0) return 0;
  char buf[BUFSIZE];
  int bytesRead = 0;
  int bytesSent = 0;
  char* p = NULL;
  while(1) {
    bytesRead = read(fd, buf, BUFSIZE);
    if (bytesRead <= 0) break;
    printf("Read: %d\n", bytesRead);
    p = buf;
    while (bytesRead > 0) {
      bytesSent = write(clientfd, p, bytesRead);
      printf("Wrote: %d\n", bytesSent);
      if (bytesSent <= 0) break;
      bytesRead -= bytesSent;
      p += bytesSent;
    }
  }
  if (bytesRead < 0) printf("READ ERROR\n");
  if (bytesSent < 0) printf("WRITE ERROR\n");
  close(fd);
  return bytesSent;
}

void *workerThread(void *workQueue) {
  TSQueue *wq = (TSQueue *) workQueue;
  int *clientfd = NULL;
  Request *req = NULL;
  Response *resp = NULL;
  char request[REQUEST_SIZE];
  int len = 0;
  while(1) {
    clientfd = tsq_get(wq);
    printf("Thread %p got work\n", pthread_self());
    memset(request, '\0', REQUEST_SIZE);
    len = recv(*clientfd, request, REQUEST_SIZE, 0);
    if (len <= 0) {
      free(clientfd);
      continue;
    }
    printf("LEN:%d\n", len);
    req = request_parse(request);

    /* find matching path for request */
    PathHandle *ph = (PathHandle *) tsq_find(filePaths, find_path, req->root);
    if (ph != NULL) {
      resp = serveFile(req);
    } else {
      printf("No matching filepath\n");
      /* Call client response handler */
      ph = (PathHandle *) tsq_find(paths, find_path, req->path);
      if (ph != NULL) resp = ph->handler(req);
      else resp = NULL;
    }

    if (resp == NULL) {
      resp = response_new(404, "", 0);
    }

    /* Write response back to client */
    write(*clientfd, resp->msg, resp->len);
    if (resp->file) transferFile(*clientfd, req->file);
    close(*clientfd);
    request_free(req);
    response_free(resp);
    free(clientfd);
  }
  pthread_exit(0);
}

int chibi_run(int port, int poolSize) {
    pthread_t workerPool[POOL_SIZE];

    int listenfd;
    struct sockaddr_in serv_addr;
    int yes = 1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '\0', sizeof serv_addr);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    // lose the pesky "Address already in use" error message
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }
    if (bind(listenfd, (struct sockaddr*) &serv_addr, sizeof serv_addr) == -1) {
      perror("bind");
      exit(1);
    }

    for (int i = 0; i < POOL_SIZE; i++) {
      pthread_create(&workerPool[i], NULL, workerThread, (void*) workQ);
    }
    listen(listenfd, LISTEN_WAITERS);
    int *clientfd;
    while(1) {
        clientfd = (int *) malloc(sizeof(int));
        if (clientfd == NULL) break;
        *clientfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
        tsq_put(workQ, clientfd);
     }
     close(listenfd);
     return 0;
}
