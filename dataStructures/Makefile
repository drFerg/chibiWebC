CC=gcc
CFLAGS+=-Wall

all: tsqTest

tsqTest: tsqTest.o tsQueue.o
	$(CC) $(CFLAGS) tsqTest.o tsQueue.o -o tsqTest

tsqTest.o: tsqTest.c tsQueue.h

clean:
	rm *.o tsqTest

.PHONY: clean
