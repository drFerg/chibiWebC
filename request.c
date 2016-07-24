#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "request.h"
#include "chibiWebDefs.h"

Request *request_new() {
  Request *r = (Request *) malloc (sizeof(Request));
  if (r == NULL) return NULL;
  //memset(r->buf, '\0', REQUEST_SIZE);
  r->type = NULL;
  r->url = NULL;
  r->param = NULL;
  return r;
}

void request_free(Request *r) {
  if (r == NULL) return;
  /* Free parameter list */
  Param *p, *q;
  p = r->param;
  while (p != NULL) {
    q = p;
    p = q->next;
    free(q);
  }
  /* Free rest of struct */
  free(r);
}

void request_addParam(Request *r, Param *p) {
  if (r->param == NULL) r->param = p;
  else {
    Param *np = r->param;
    while (np->next != NULL) np = np->next;
    np->next = p;
  }
}

Request *http_parseRequest(char *request) {
  Request *r = request_new();
  if (r == NULL) return NULL;
  r->buf = request;
  printf("%s\n", r->buf);
  /* Save pointers for strtok_r */
  char *hdrsave, *urlsave, *querysave;
  /* Get request type (GET, POST) */
  r->type = strtok_r(r->buf, " ", &hdrsave);
  printf("Request:%s\n", r->type);
  if (strncmp(r->type, "GET", 3) != 0) {
    printf("We only accept gets\n");
    request_free(r);
    return NULL;
  }
  /* Get request URL + params */
  r->url = strtok_r(NULL, " ", &hdrsave);
  /* Check if we have a list of query parameters and seperate */
  char *url = strtok_r(r->url, "?", &urlsave);
  printf("URL: %s\n", url);
  if (url) r->url = url;
  else return r;

  /* Get query param half */
  r->paramStr = strtok_r(NULL, "?", &urlsave);
  if (r->paramStr == NULL) return r;
  printf("paramstr:%s\n", r->paramStr);
  /* Do we have more than one? (Split by &) */
  char *q = strtok_r(r->paramStr, "&", &urlsave);
  if (q == NULL) q = r->paramStr;

  /* Retrieve params */
  Param *p, *lastP;
  do {
    p = (Param *) malloc(sizeof(Param));
    if (p == NULL) {
      request_free(r);
      return NULL;
    }
    /* Key, value pairs are split by = */
    p->key = strtok_r(q, "=", &querysave);
    p->value = strtok_r(NULL, "=", &querysave);
    p->next = NULL;
    printf("%s:%s\n", p->key, p->value);
    /* Add to param list */
    if (r->param == NULL) r->param = p;
    else lastP->next = p;
    lastP = p;  /* Used for next iteration, makes insertion O(1) */
  } while((q = strtok_r(NULL, "&", &urlsave))); /* Next param */
  return r;
}
