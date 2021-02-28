#ifndef RESPONSE_H
#define RESPONSE_H

#include "chibiWebDefs.h"

// Response codes
#define STATUS_200_OK 200
#define STATUS_404_NOT_FOUND 404

typedef struct response {
  char header[RESPONSE_HDR_SIZE];
  char *msg;
  int hdrLen;
  int len;
  int isFile;
  int fileLen;
  int fd;
} Response;

Response *response_new(int status, char *text, int len);
Response *response_new_file(int status, char* filePath);
void response_free(Response *resp);

#endif /* RESPONSE_H */
