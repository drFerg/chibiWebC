#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "response.h"
#include "chibiWebDefs.h"

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

Response *response_new(int status, char *text, int len) {
  Response *resp = (Response *) malloc(sizeof (Response));
  if (resp == NULL) return NULL;
  resp->file = 0;
  resp->hdrLen = generateResponse(resp->header, RESPONSE_HDR_SIZE, status, len, 0);
  resp->len = resp->hdrLen + len + 1;
  resp->msg = (char *) malloc(resp->len);
  if (resp->msg == NULL) {
    free(resp);
    return NULL;
  }
  memcpy(resp->msg, resp->header, resp->hdrLen);
  memcpy(resp->msg + resp->hdrLen, text, len);
  resp->msg[resp->len - 1] = '\0';
  printf("RESPONSE:\n%s\n################\n", resp->msg);
  return resp;
}

void response_free(Response *resp) {
  free(resp->msg);
  free(resp);
}
