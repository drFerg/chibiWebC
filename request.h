#ifndef REQUEST_H
#define REQUEST_H

#define REQUEST_SIZE 1500

typedef struct param {
  char *key;
  char *value;
  struct param *next;
} Param;

typedef struct request {
    char *buf;
    char *type;
    char *url;
    char *paramStr;
    Param *param;
} Request;

Request *request_new();
Request *http_parseRequest(char *request);
void request_addParam(Request *r, Param *p);
void request_free(Request *r);

#endif /* REQUEST_H */
