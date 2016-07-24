#ifndef WORK_H
#define WORK_H
#include <pthread.h>
typedef struct work {
  int fd;
  struct work *next;
} Work;

typedef struct workQueue {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  Work *work;
} WorkQueue;



void work_put(WorkQueue *wq, Work *w);
Work *work_get(WorkQueue *wq);

#endif /* WORK_H */
