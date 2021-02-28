#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "request.h"
#include "chibiWebDefs.h"

Request *request_new() {
  Request *r = (Request *) malloc (sizeof(Request));
  if (r == NULL) return NULL;
  r->version = NULL;
  r->type = REQUEST_NULL;
  r->path = NULL;
  r->param = NULL;
  r->body = NULL;
  r->headers = tsq_create();
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
  tsq_destroy(r->headers, NULL);
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

Request *request_parse(char *request) {
  int pathLen = 0;
  char *paramStr;
  Request *r = request_new();
  if (r == NULL) return NULL;
  
  r->buf = request;
  printf("%s\n", r->buf);
  
  /* Save pointers for strtok_r */
  char *hdrsave, *urlsave, *querysave, *http_type;
  /* Get request type (GET, POST) */
  http_type = strtok_r(r->buf, " ", &hdrsave);
  printf("-----------------------\n");
  printf("Request:%s\n", http_type);

  if (strncmp(http_type, "GET", 3) == 0) {
    r->type = REQUEST_GET;
  }
  else if (strncmp(http_type, "POST", 4) == 0) {
    r->type = REQUEST_POST;
  }

  if (r->type == REQUEST_NULL) {
    printf("We only accept GET/POST\n");
    request_free(r);
    return NULL;
  }
  
  /* Get request URL  */
  r->path = strtok_r(NULL, " ", &hdrsave);
  printf("PATH: %i: %s\n", pathLen, r->path);

  r->version = strtok_r(NULL, "\n", &hdrsave);
  printf("VERSION: %s\n", r->version);

  Param *p;
  char *hdr, *save;
  hdr = strtok_r(NULL, "\n", &hdrsave);;
  do {
      p = (Param *) malloc(sizeof(Param));
      if (p == NULL) {
        request_free(r);
        return NULL;
      }
      /* Key, value pairs are split by : */
      p->key = strtok_r(hdr, ":", &save);
      p->value = save;
      p->next = NULL;
      printf("HEADER: %s:%s\n", p->key, p->value);
      /* Add to param list */
      tsq_put(r->headers, p);
    } while((hdr = strtok_r(NULL, "\n", &hdrsave)) && *hdr != '\r'); /* Next param */

  r->body= hdrsave;
  printf("BODY: %s\n", r->body);
  /* Chop off parameters, finishing path */
  char *url = strtok_r(r->path, "?", &urlsave);
  printf("params -> %s\n", url);
  paramStr = strtok_r(NULL, " ", &urlsave);

  paramStr = paramStr ? paramStr : r->body; 
  if (paramStr) {
    r->paramStr = strdup(paramStr);
    r->path = url;
    /* Get query param half */
    printf("ParamStr: %s\n", r->paramStr);
    printf("Params:\n");
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
      printf("\t%s: %s\n", p->key, p->value);
      /* Add to param list */
      if (r->param == NULL) r->param = p;
      else lastP->next = p;
      lastP = p;  /* Used for next iteration, makes insertion O(1) */
    } while((q = strtok_r(NULL, "&", &urlsave))); /* Next param */
  }
  pathLen = strlen(r->path);
  r->root = strdup(r->path);
  strtok_r(r->root + 1, "/", &urlsave);
  r->file = urlsave;
  printf("Root: %s\nFile: %s\n", r->root, urlsave);
  /* Remove trailing forward-slash (/) */
  if (pathLen > 1 && r->path[pathLen - 1] == '/') r->path[pathLen - 1] = '\0';
  printf("PATH(%i): %s\n", pathLen, r->path);


  return r;
}
