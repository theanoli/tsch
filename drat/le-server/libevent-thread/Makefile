CC=gcc
CFLAGS=-levent -lpthread

all: echoserver_threaded
	$(CC) echoserver_threaded.c workqueue.c $(CFLAGS)
