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

int http_response(int fd, int status, char *text, int len) {
  char header[1000];

  int hdrLen = generateResponse(header, 1000, status, len, 0);
  int responseLen = hdrLen + len + 1;
  char *response = (char *) malloc(responseLen);
  if (response == NULL) return -1;
  memcpy(response, header, hdrLen);
  memcpy(response + hdrLen, text, len);
  response[responseLen - 1] = '\0';
  printf("RESPONSE:\n%s\n################\n", response);
  return write(fd, response, responseLen);
}
