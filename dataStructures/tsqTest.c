#include <stdio.h>
#include "tsQueue.h"


int equal(void *a, void *b) {
  int *aa = (int *) a;
  int *bb = (int *) b;
  printf("%d:%d:%d\n", *aa, *bb, *aa == *bb);
  if (*aa == *bb) return 1;
  else return 0;
}

int main(int argc, char const *argv[]) {
  TSQueue *q = tsq_create();
  int a = 1;
  int b = 2;
  int c = 3;
  tsq_put(q, &a);
  tsq_put(q, &b);
  tsq_put(q, &c);

  printf("%d\n", *(int *)tsq_get(q));
  int *z = (int *)tsq_find(q, equal, &c);
  printf("%d\n", *z);
  tsq_destroy(q, NULL);
  return 0;
}
