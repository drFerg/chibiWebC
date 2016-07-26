CC=gcc
CFLAGS+=-Wall -g -pthread -IdataStructures/

OBJ=chibiServer.o chibiWeb.o request.o response.o dataStructures/tsQueue.o
LDFLAGS=-IdataStructures/

all: server

server: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o chibiServer

clean:
	rm *.o chibiServer

.PHONY: clean
