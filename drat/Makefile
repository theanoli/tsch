CC=g++
CFLAGS=-c -Wall

SOURCES=client.c server.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=$(basename $(SOURCES))

all: $(EXECUTABLES)

client: client.o
	$(CC) client.o -o client

server: server.o
	$(CC) server.o -o server

clean:
	rm *o $(EXECUTABLES)
