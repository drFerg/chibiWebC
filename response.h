#ifndef RESPONSE_H
#define RESPONSE_H

#include "chibiWebDefs.h"

typedef struct response {
  char header[RESPONSE_HDR_SIZE];
  char *msg;
  int hdrLen;
  int len;
} Response;

Response *response_new(int status, char *text, int len);
void response_free(Response *resp);

#endif /* RESPONSE_H */
