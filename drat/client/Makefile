CC=gcc
CFLAGS=-std=gnu99 -Wall -lrt 

SOURCES=client.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLES=$(basename $(SOURCES))

all: $(EXECUTABLES)

client: client.o
	$(CC) $(CFLAGS) client.o -o client  

clean:
	rm *o $(EXECUTABLES)
