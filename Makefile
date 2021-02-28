CC=gcc
CFLAGS+=-Wall -g -pthread -IdataStructures/

OBJ=chibiServer.o chibiWeb.o request.o response.o dataStructures/tsQueue.o
LDFLAGS=-IdataStructures/

all: server adts

adts:
	$(MAKE) -C dataStructures

server: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o chibiServer

check: all
	./chibiTest.sh

clean:
	rm *.o chibiServer

.PHONY: clean adts
