#ifndef TSQUEUE_H
#define TSQUEUE_H
#include <pthread.h>

typedef struct elem {
  void *payload;
  struct elem *next;
} Elem;

typedef struct tsqueue {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  Elem *elem;
} TSQueue;


TSQueue *tsq_create();
void tsq_destroy(TSQueue *tsq, void (*payloadFree)(void *payload));
int tsq_put(TSQueue *tsq, void *payload);
void *tsq_get(TSQueue *tsq);
void *tsq_find(TSQueue *tsq, int (*equal)(void *a, void *b), void *a);

#endif /* TSQUEUE_H */
