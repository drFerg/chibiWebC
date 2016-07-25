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

#include "chibiWeb.h"
#include "work.h"
#include "request.h"
#include "response.h"
#include "chibiWebDefs.h"
#include "tsQueue.h"

#define POOL_SIZE 4

typedef struct pHandle {
  char* path;
  Handler handler;
}PathHandle;

Handler h = NULL;
TSQueue *paths;

void chibi_init() {
  paths = tsq_create();
}

int chibi_serve(char *path, Handler handler) {
  PathHandle *p = (PathHandle *) malloc(sizeof(PathHandle));
  if (p == NULL) return 0;
  p->path = path;
  p->handler = handler;
  if (tsq_put(paths, p)) return 1;
  free(p);
  return 0;
}

int find_path(void *a, void *b) {
  char *path = (char *) a;
  PathHandle *phb = (PathHandle *) b;
  return !strcmp(path, phb->path);
}

void *workerThread(void *workQueue) {
  WorkQueue * wq = (WorkQueue *) workQueue;
  Work *w = NULL;
  Request *r = NULL;
  Response *resp = NULL;
  char request[REQUEST_SIZE];
  int len = 0;
  while(1) {
    w = work_get(wq);
    printf("Thread 0x%x got work\n", (int) pthread_self());
    memset(request, '\0', REQUEST_SIZE);
    len = recv(w->fd, request, REQUEST_SIZE, 0);
    if (len <= 0) {
      free(w);
      continue;
    }
    printf("LEN:%d\n", len);
    r = http_parseRequest(request);

    /* find matching path for request */
    PathHandle *ph = (PathHandle *) tsq_find(paths, find_path, r->path);
    if (ph != NULL) resp = ph->handler(r);
    else resp = NULL;
    if (resp == NULL) {
      resp = response_new(404, "", 0);
    }

    /* Write response back to client */
    write(w->fd, resp->msg, resp->len);
    close(w->fd);
    request_free(r);
    response_free(resp);
    free(w);
  }
  pthread_exit(0);
}

int chibi_run(int port, int poolSize) {
    pthread_t workerPool[POOL_SIZE];
    WorkQueue workQueue;
    workQueue.work = NULL;
    pthread_mutex_init(&(workQueue.lock), NULL);
    pthread_cond_init(&(workQueue.cond), NULL);

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
      pthread_create(&workerPool[i], NULL, workerThread, (void*) &workQueue);
    }
    listen(listenfd, LISTEN_WAITERS);
    Work *w;
    while(1) {
        w = (Work *) malloc(sizeof(Work));
        if (w == NULL) break;
        w->next = NULL;
        w->fd = accept(listenfd, (struct sockaddr*) NULL, NULL);
        work_put(&workQueue, w);
     }
     close(listenfd);
     return 0;
}
