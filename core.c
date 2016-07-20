
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define SERVER_NAME "CWeb"
char const *msg = "HTTP/1.1 200 OK\nDate: Mon, 27 Jul 2009 12:28:53 GMT\nServer: CWeb (MacOS)\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\nContent-Length: 0\nContent-Type: text/html\nConnection: Closed";


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
  memcpy(response, header, hdrLen);
  memcpy(response + hdrLen, text, len);
  response[responseLen - 1] = '\0';
  printf("%s\n", response);
  return write(fd, response, responseLen);
}

int http_parseRequest(int fd) {
  char buf[1500];
  char *saveptr, *req, *url;
  recv(fd, buf, 1500, 0);
  printf("%s\n", buf);

  req = strtok_r(buf, " ", &saveptr);
  printf("Request:%s\n", req);
  if (strncmp(req, "GET", 3) != 0) {
    printf("We only accept gets\n");
    return 0;
  }
  url = strtok_r(NULL, " ", &saveptr);
  printf("URL: %s\n", url);
  return 1;

}

int main(int argc, char const *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char sendBuff[1025];


    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
        printf("Got new connection\n");
        http_parseRequest(connfd);
        http_response(connfd, 200, "Hello world", 11);
        // ticks = time(NULL);
        // snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));


        close(connfd);
     }
}
