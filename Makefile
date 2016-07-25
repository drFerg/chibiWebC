
CFLAGS+=-Wall -g -pthread

INC=chibiServer.c chibiWeb.c request.c response.c work.c dataStructures/tsQueue.c
LIBS=-IdataStructures/

all: server

server: $(INC)
	gcc $(CFLAGS) $(INC) $(LIBS) -o chibiServer
