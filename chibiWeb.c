
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

#define POOL_SIZE 20
#define REQUEST_SIZE 1500

typedef struct work {
  int fd;
  struct work *next;
} Work;

typedef struct workQueue {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  Work *work;
} WorkQueue;

typedef struct param {
  char *key;
  char *value;
  struct param *next;
} Param;

typedef struct request {
    char buf[REQUEST_SIZE];
    char *type;
    char *url;
    char *paramStr;
    Param *param;
} Request;

#define SERVER_NAME "ChibiWebC"

int generateResponse(char *buffer, int buffSize, int response, int length, int contentType) {
  time_t ticks;
  struct tm *loctime;
  ticks = time(NULL);
  loctime = localtime (&ticks);

  return snprintf(buffer, buffSize,
    "HTTP/1.1 %d\nDate: %sServer: %s\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\n",
    response,
    asctime (loctime),
    SERVER_NAME,
    length);
}

int http_response(int fd, int status, char *text, int len) {
  char header[1000];

  int hdrLen = generateResponse(header, 1000, status, len, 0);
  int responseLen = hdrLen + len + 1;
  char *response = (char *) malloc(responseLen);
  if (response == NULL) return -1;
  memcpy(response, header, hdrLen);
  memcpy(response + hdrLen, text, len);
  response[responseLen - 1] = '\0';
  printf("RESPONSE:\n%s\n################\n", response);
  return write(fd, response, responseLen);
}


void request_addParam(Request *r, Param *p) {
  if (r->param == NULL) r->param = p;
  else {
    Param *np = r->param;
    while (np->next != NULL) np = np->next;
    np->next = p;
  }
}

Request *request_new() {
  Request *r = (Request *) malloc (sizeof(Request));
  if (r == NULL) return NULL;
  memset(r->buf, '\0', REQUEST_SIZE);
  r->type = NULL;
  r->url = NULL;
  r->param = NULL;
  return r;
}

void request_free(Request *r) {
  if (r == NULL) return;
  /* Free parameter list */
  Param *p, *q;
  p = r->param;
  while (p != NULL) {
    q = p;
    p = q->next;
    free(q);
  }
  /* Free rest of struct */
  free(r);
}

Request *http_parseRequest(int fd) {
  Request *r = request_new();
  if (r == NULL) return NULL;
  if (recv(fd, r->buf, REQUEST_SIZE, 0) == -1) {
    request_free(r);
    return NULL;
  }
  printf("%s\n", r->buf);
  /* Save pointers for strtok_r */
  char *hdrsave, *urlsave, *querysave;
  /* Get request type (GET, POST) */
  r->type = strtok_r(r->buf, " ", &hdrsave);
  printf("Request:%s\n", r->type);
  if (strncmp(r->type, "GET", 3) != 0) {
    printf("We only accept gets\n");
    request_free(r);
    return NULL;
  }
  /* Get request URL + params */
  r->url = strtok_r(NULL, " ", &hdrsave);
  /* Check if we have a list of query parameters and seperate */
  char *url = strtok_r(r->url, "?", &urlsave);
  printf("URL: %s\n", url);
  if (url) r->url = url;
  else return r;

  /* Get query param half */
  r->paramStr = strtok_r(NULL, "?", &urlsave);
  if (r->paramStr == NULL) return r;
  printf("paramstr:%s\n", r->paramStr);
  /* Do we have more than one? (Split by &) */
  char *q = strtok_r(r->paramStr, "&", &urlsave);
  if (q == NULL) q = r->paramStr;

  /* Retrieve params */
  Param *p, *lastP;
  do {
    p = (Param *) malloc(sizeof(Param));
    if (p == NULL) {
      request_free(r);
      return NULL;
    }
    /* Key, value pairs are split by = */
    p->key = strtok_r(q, "=", &querysave);
    p->value = strtok_r(NULL, "=", &querysave);
    p->next = NULL;
    printf("%s:%s\n", p->key, p->value);
    /* Add to param list */
    if (r->param == NULL) r->param = p;
    else lastP->next = p;
    lastP = p;  /* Used for next iteration, makes insertion O(1) */
  } while((q = strtok_r(NULL, "&", &urlsave))); /* Next param */
  return r;
}


Work *work_get(WorkQueue *wq) {
  Work *q = NULL;
  pthread_mutex_lock(&(wq->lock));
  while (wq->work == NULL) pthread_cond_wait(&(wq->cond), &(wq->lock));
  q = wq->work;
  wq->work = wq->work->next;
  pthread_mutex_unlock(&(wq->lock));
  return q;
}

void work_put(WorkQueue *wq, Work *w) {
  Work *v;
  pthread_mutex_lock(&(wq->lock));
  if (wq->work == NULL) wq->work = w;
  else {
    for(v = wq->work; v->next != NULL; v = v->next);
    v->next = w;
  }
  pthread_cond_signal(&(wq->cond));
  pthread_mutex_unlock(&(wq->lock));
}

void *workerThread(void *workQueue) {
  WorkQueue * wq = (WorkQueue *) workQueue;
  Work *w = NULL;
  Request *r = NULL;
  while(1) {
    w = work_get(wq);
    printf("Thread 0x%x got work\n", (int) pthread_self());
    r = http_parseRequest(w->fd);
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
        w->next = NULL;
        w->fd = accept(listenfd, (struct sockaddr*) NULL, NULL);
        work_put(&workQueue, w);
     }
}
