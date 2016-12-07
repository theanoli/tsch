CC=g++

SOURCES=client.c server.c
EXECUTABLES=client server

all: $(SOURCES) $(EXECUTABLES)

server.o: server.c
	$(CC) server.c

client.o: client.c
	$(CC) client.c

clean:
	rm *o $(EXECUTABLES)
