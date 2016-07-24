#include "work.h"

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
