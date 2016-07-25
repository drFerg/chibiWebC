#include "tsQueue.h"
#include <stdlib.h>


TSQueue *tsq_create() {
  TSQueue *tsq = (TSQueue *) malloc(sizeof(TSQueue));
  if (tsq == NULL) return NULL;
  pthread_mutex_init(&(tsq->lock), NULL);
  pthread_cond_init(&(tsq->cond), NULL);
  tsq->elem = NULL;
  return tsq;
}

void tsq_destroy(TSQueue *tsq, void (*payloadFree)(void *payload)) {
  Elem *e, *f;
  e = tsq->elem;
  if (payloadFree) {
    while (e != NULL) {
      f = e;
      e = f->next;
      payloadFree(f);
    }
  }
  free(tsq);
}

void *tsq_get(TSQueue *tsq) {
  void *q = NULL;
  Elem *e = NULL;
  pthread_mutex_lock(&(tsq->lock));
  while (tsq->elem == NULL) pthread_cond_wait(&(tsq->cond), &(tsq->lock));
  e = tsq->elem;
  q = tsq->elem->payload;
  tsq->elem = tsq->elem->next;
  pthread_mutex_unlock(&(tsq->lock));
  free(e);
  return q;
}

int tsq_put(TSQueue *tsq, void *payload) {
  Elem *e;
  Elem *elem = (Elem *) malloc(sizeof(Elem));
  if (elem == NULL) return 0;
  elem->payload = payload;
  elem->next = NULL;

  if (tsq->elem == NULL) tsq->elem = elem;
  else {
    for(e = tsq->elem; e->next != NULL; e = e->next)
    ;
    e->next = elem;
  }
  pthread_cond_signal(&(tsq->cond));
  pthread_mutex_unlock(&(tsq->lock));
  return 1;
}

void *tsq_find(TSQueue *tsq, int (*equal)(void *a, void *b), void *a) {
  void *b = NULL;
  pthread_mutex_lock(&(tsq->lock));
  for(Elem *e = tsq->elem; e != NULL; e = e->next) {
    if (equal(a, e->payload)) b = e->payload;
  }
  pthread_mutex_unlock(&(tsq->lock));
  return b;
}
