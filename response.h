#ifndef RESPONSE_H
#define RESPONSE_H

int http_response(int fd, int status, char *text, int len);

#endif /* RESPONSE_H */
