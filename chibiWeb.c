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
#include <signal.h>

#include "chibiWeb.h"
#include "request.h"
#include "response.h"
#include "chibiWebDefs.h"
#include "tsQueue.h"

#define POOL_SIZE 4
#define BUFSIZE 1000

typedef struct pHandle {
  char* path;
  char* localPath;
  Handler handler;
} PathHandle;

Handler h = NULL;
TSQueue *filePaths, *paths, *workQ;
// use volatile to prevent compiler caching
static volatile int keepRunning = TRUE;

/* Copy s1 and s2 into a new str and return it */
char* strcatcpy(char* s1, char* s2) {
  char* str = (char*) malloc(strlen(s1) + strlen(s2) + 1);
  if (!str) return NULL;

  sprintf(str, "%s%s", s1, s2);
  return str;
}
void pathhandle_free(void* ph) {
  PathHandle* p = (PathHandle*) ph;
  free(p->path);
  if (p->localPath) free(p->localPath);
  free(p);
}

int find_path(void *a, void *b) {
  char *path = (char *) a;
  PathHandle *phb = (PathHandle *) b;
  return !strcmp(path, phb->path);
}

void interruptHandler(int dummy) {
  keepRunning = FALSE;
}

void setup_signal_handler(){
  // use sigaction to stop blocking calls from restarting after interrupts
  struct sigaction a;
  a.sa_handler = interruptHandler;
  a.sa_flags = 0;
  sigemptyset(&a.sa_mask);
  sigaction(SIGINT, &a, NULL);
}

int chibi_init() {
  setup_signal_handler();

  filePaths = tsq_create();
  if (!filePaths) {
    printf("Failed to init filePaths queue\n");
    return FALSE;
  }
  paths = tsq_create();
  if (!paths) {
    printf("Failed to init paths queue\n");
    return FALSE;
  }
  workQ = tsq_create();
  if (!workQ) {
    printf("Failed to init work queue\n");
    return FALSE;
  }
  return TRUE;
}

Response *serveFile(Request *req) {
  PathHandle* ph = (PathHandle *) tsq_find(filePaths, find_path, req->root);
  if (!ph) {
    printf("serveFile: %s\n", req->root);
    return 0;
  }
  req->file = strcatcpy(ph->localPath, req->file);
  printf("Resolved filepath: %s\n", req->file);

  if (!req->file) return 0;

  return response_new_file(STATUS_200_OK, req->file);
}

int serve(char *path, char* localPath, Handler handler, TSQueue *tsq) {
  PathHandle *p = (PathHandle *) malloc(sizeof(PathHandle));
  if (p == NULL) return 0;
  
  // copy over paths
  p->path = strdup(path);
  if (localPath) p->localPath = strdup(localPath);
  p->handler = handler;

  if (tsq_put(tsq, p)) return 1;
  
  // put failed, free and bail
  pathhandle_free(p);
  return 0;
}



int chibi_serve(char *path, Handler handler) {
  return serve(path, NULL, handler, paths);
}

int chibi_serveFiles(char *path, char *localPath) {
  return serve(path, localPath, serveFile, filePaths);
}



int transferFile(int clientfd, int fd) {
  char buf[BUFSIZE];
  int bytesRead = 0;
  int bytesSent = 0;
  char* p = NULL;
  
  printf("transferFile: starting transfer(%d -> %d)\n", fd, clientfd);
  while(1) {
    // read a chunk
    bytesRead = read(fd, buf, BUFSIZE);
    if (bytesRead <= 0) break;

    p = buf;
    while (bytesRead > 0) {
      // write a chunk
      bytesSent = write(clientfd, p, bytesRead);
      if (bytesSent <= 0) break;

      bytesRead -= bytesSent;
      p += bytesSent;
    }
  }
  if (bytesRead < 0) perror("transferFile: READ ERROR\n");
  if (bytesSent < 0) perror("transferFile: WRITE ERROR\n");
  close(fd);

  printf("transferFile: transfer complete(%d -> %d)\n", fd, clientfd);
  return bytesSent;
}

void *workerThread(void *workQueue) {
  TSQueue *wq = (TSQueue *) workQueue;
  int *clientfd = NULL;
  Request *req = NULL;
  Response *resp = NULL;
  char request[REQUEST_SIZE];
  int len = 0;
  while(keepRunning) {
    clientfd = tsq_get(wq);
    /* Check for stop sentinel */
    if (*clientfd == -1) break;

    /* Otherwise got work */
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
      resp = response_new(STATUS_404_NOT_FOUND, "", 0);
    }

    /* Write response back to client */
    write(*clientfd, resp->msg, resp->len);
    if (resp->isFile) transferFile(*clientfd, resp->fd);
    
    close(*clientfd);
    request_free(req);
    response_free(resp);
    free(clientfd);
  }
  pthread_exit(0);
}

int chibi_run(int port, int poolSize) {
    printf("chibiWeb starting...\n");
    pthread_t workerPool[POOL_SIZE];
    int listenfd;
    struct sockaddr_in serv_addr;
    int socket_yes = 1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '\0', sizeof serv_addr);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    printf("> opening socket on port: %d\n", port);
    // set SO_REUSEADDR to prevent "Address already in use"
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &socket_yes, sizeof socket_yes) == -1) {
        perror("setsockopt");
        exit(1);
    }
    if (bind(listenfd, (struct sockaddr*) &serv_addr, sizeof serv_addr) == -1) {
      perror("bind");
      exit(1);
    }
    printf("> bound socket\n");
    printf("> creating workers...");
    for (int i = 0; i < POOL_SIZE; i++) {
      pthread_create(&workerPool[i], NULL, workerThread, (void*) workQ);
    }
    printf(" created %d\n", POOL_SIZE);

    printf("> listening on: http://127.0.0.1:%d\n", port);
    listen(listenfd, LISTEN_WAITERS);
    int *clientfd;

    // main loop for receiving incoming requests
    while(keepRunning) {
      clientfd = (int *) malloc(sizeof(int));
      if (clientfd == NULL) break;

      *clientfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
      // check for signal interrupt
      if (*clientfd == -1 & errno == EINTR) break;
      // got a valid fd, add to queue to service
      tsq_put(workQ, clientfd);
    }

    printf("> exiting\n");
    void *result = NULL;
    printf("> stopping threads...");

    /* fill workQ with stop sentinels */
    int stopSentinel = -1;
    for (int i = 0; i < POOL_SIZE; i++) {
      tsq_put(workQ, &stopSentinel);
    }

    /* Wait on threads to pick up sentinel and stop gracefully */
    for (int i = 0; i < POOL_SIZE; i++) {
      pthread_join(workerPool[i], &result);
      printf(" [%d]", i);
    }
    printf("done\n");

    printf("> freeing resources...");
    // free data stores
    tsq_destroy(workQ, NULL);
    tsq_destroy(paths, &pathhandle_free);
    tsq_destroy(filePaths, &pathhandle_free);

    // free/close file descriptors
    if (clientfd) free(clientfd);
    close(listenfd);

    printf("done\n");
    printf("Shutdown complete\n");
    return 0;
}
