CC=gcc
CFLAGS=-Wall -levent

SOURCES=client.c le_server.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=$(basename $(SOURCES))

all: $(EXECUTABLES)

client: client.o
	$(CC) client.o -o client $(CFLAGS) 

le_server: le_server.o
	$(CC) le_server.o -o le_server $(CFLAGS)

clean:
	rm *o $(EXECUTABLES)
