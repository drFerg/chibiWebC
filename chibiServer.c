#include "chibiWeb.h"
#include "response.h"
#include "request.h"


Response *testHandler(Request *r) {
  return response_new(200, "HELLO", 5);
}

int main(int argc, char const *argv[]) {

  chibi_serve("/test", testHandler);
  chibi_run(5000, 4);
  return 0;
}
