
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

#include "work.h"
#include "request.h"
#include "response.h"
#include "chibiWebDefs.h"
#define POOL_SIZE 20


void *workerThread(void *workQueue) {
  WorkQueue * wq = (WorkQueue *) workQueue;
  Work *w = NULL;
  Request *r = NULL;
  char request[REQUEST_SIZE];
  while(1) {
    w = work_get(wq);
    printf("Thread 0x%x got work\n", (int) pthread_self());
    if (recv(w->fd, request, REQUEST_SIZE, 0) == -1) {
      free(w);
      continue;
    }
    r = http_parseRequest(request);
    request_free(r);
    http_response(w->fd, 200, "Hello world", 11);
    close(w->fd);
    free(w);
  }
  pthread_exit(0);
}

int main(int argc, char const *argv[]) {
    pthread_t workerPool[POOL_SIZE];
    WorkQueue workQueue;
    pthread_mutex_init(&(workQueue.lock), NULL);
    pthread_cond_init(&(workQueue.cond), NULL);


    int listenfd;
    struct sockaddr_in serv_addr;
    int yes = 1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '\0', sizeof serv_addr);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

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
    listen(listenfd, 10);
    Work *w;
    while(1) {
        w = (Work *) malloc(sizeof(Work));
        if (w == NULL) break;
        w->next = NULL;
        w->fd = accept(listenfd, (struct sockaddr*) NULL, NULL);
        work_put(&workQueue, w);
     }
     close(listenfd);

}
