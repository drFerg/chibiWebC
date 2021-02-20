#include <stdio.h>
#include "tsQueue.h"
#include <stdlib.h>

#define INT_COUNT 1000

int equal(void *a, void *b) {
  return *(int *) a == *(int *)b;
}

int main(int argc, char const *argv[]) {
  TSQueue *q = tsq_create();
  int a = 1;
  int b = 2;
  int c = 3;

  printf("Test tsq_put:\n");
  printf("\tPut %d %s\n", a, tsq_put(q, &a) ? "successfully" : "failed");
  printf("\tPut %d %s\n", b, tsq_put(q, &b) ? "successfully" : "failed");
  printf("\tPut %d %s\n", c, tsq_put(q, &c) ? "successfully" : "failed");
  printf("Test tsq_get:\n\tExpected: %d - got: %d\n", a, *(int *) tsq_get(q));
  int *z = (int *) tsq_find(q, equal, &c);
  printf("Test tsq_find:\n\tExpected: %d - got: %d\n", c, *z);
  tsq_destroy(q, NULL);
  
  int *x = NULL;
  int successful = 0;
  q = tsq_create();
  printf("Test putting/finding %d ints:\n", INT_COUNT);
  for (int i = 0; i < INT_COUNT; i++) {
    x = (int *) malloc(sizeof(int));
    *x = i;
    if (!tsq_put(q, x)) printf("Failed to put %d",  i);
    int *y = (int *)tsq_find(q, equal, &i);
    if (x == y) successful += 1;
  }
  printf("\tFound %d/%d\n", successful, INT_COUNT);
  tsq_destroy(q, free);
  return 0;
}
