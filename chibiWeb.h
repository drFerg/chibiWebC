#ifndef CHIBI_WEB_H
#define CHIBI_WEB_H

#include "response.h"
#include "request.h"

typedef Response * (*Handler)(Request *);


void chibi_init();

/* Adds a path to serve which is handled by the provided handler
 *
 * handler needs to expect a Request * and return a Response * to send back to
 * the client.
 */
int chibi_serve(char *path, Handler handler);

/* Starts the server, does not return control */
int chibi_run(int port, int poolSize);

#endif /* CHIBI_WEB_H */
