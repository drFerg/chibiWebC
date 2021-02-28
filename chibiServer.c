#include "chibiWeb.h"
#include "response.h"
#include "request.h"
#include "string.h"
#include "stdio.h"


Response *testHandler(Request *r) {
	if (r->type == REQUEST_POST) {
		char buff[100];
		sprintf(buff, "body - %s", r->body);
		printf("%s\n", buff);
		return response_new(STATUS_200_OK, buff, strlen(buff));
	}
  return response_new(STATUS_200_OK, "HELLO", 5);
}

Response *dogHandler(Request *r) {
  return response_new(STATUS_200_OK, "WOOF!", 5);
}

Response *indexHandler(Request *r) {
	return response_new_file(STATUS_200_OK, "./static/index.html");
}


int main(int argc, char const *argv[]) {
  chibi_init();
  chibi_serve("/", indexHandler);
  chibi_serveFiles("/static", "static/");
  chibi_serve("/hello", testHandler);
  chibi_serve("/dogs", dogHandler);
  chibi_run(5000, 4);
  return 0;
}
