#ifndef CHIBI_WEB_H
#define CHIBI_WEB_H

#include "response.h"
#include "request.h"

typedef Response * (*Handler)(Request *);

void chibi_serve(char *url, Handler handler);
int chibi_run(int port, int poolSize);

#endif /* CHIBI_WEB_H */
