#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "response.h"
#include "chibiWebDefs.h"

int generateResponse(char *buffer, int buffSize, int status_code, int length, int contentType) {
  char *textType = "text/html";
  char *content = NULL;
  time_t ticks = time(NULL);
  struct tm *loctime = localtime(&ticks);

  if (contentType == 0) content = textType;
  return snprintf(buffer, buffSize,
    "HTTP/1.1 %d\nDate: %sServer: %s\nContent-Length: %d\nContent-Type:%s \nConnection: Closed\n\n",
    status_code,
    asctime (loctime),
    SERVER_NAME,
    length,
    content);
}

Response *response_new_file(int status, char* filePath) {
  struct stat stbuf;

  int fd = open(filePath, O_RDONLY);
  printf("response_new_file(%s)\n", filePath);
  if (fd < 0) {
    perror("response_new_file(%s):");
    return 0;
  }
  
  // get file stats and check is file (ISREG)
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    printf("response_new_file(%s): not a file\n", filePath);
    close(fd);
    return NULL;
  }

  Response *resp = (Response *) malloc(sizeof (Response));
  if (resp == NULL) return NULL;

  resp->file = 1;
  resp->hdrLen = generateResponse(resp->header, RESPONSE_HDR_SIZE, status, stbuf.st_size, 1);
  resp->len = resp->hdrLen;
  resp->msg = resp->header;
  resp->fd = fd;
  printf("response_new_file(%s):\n%s\n################\n", filePath, resp->msg);
  return resp;
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
  if (!resp->file) free(resp->msg);
  free(resp);
}
