#ifndef REQUEST_H
#define REQUEST_H

#include "chibiWebDefs.h"

typedef struct param {
  char *key;
  char *value;
  struct param *next;
} Param;

typedef struct request {
    char *buf;
    char *type;
    char *path;
    char *paramStr;
    Param *param;
} Request;
/* parse a char request and return a new filled request */
Request *request_parse(char *request);
Request *request_new();
void request_free(Request *r);
void request_addParam(Request *r, Param *p);

#endif /* REQUEST_H */
