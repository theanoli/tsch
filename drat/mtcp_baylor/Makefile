TARGETS = server
CC=gcc -g -O3 -Wall -Werror -fgnu89-inline
DPDK=1

# DPDK LIBRARY and HEADER
DPDK_INC=/proj/sequencer/mtcp/dpdk/include
DPDK_LIB=/proj/sequencer/mtcp/dpdk/lib/

# mtcp library and header 
MTCP_FLD    =/proj/sequencer/mtcp/mtcp/
MTCP_INC    =-I${MTCP_FLD}/include
MTCP_LIB    =-L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a

INC = -I./include/ ${MTCP_INC} 
LIBS = ${MTCP_LIB}

# CFLAGS for DPDK-related compilation
INC += ${MTCP_INC}
ifeq ($(DPDK),1)
DPDK_MACHINE_FLAGS = $(shell cat /proj/sequencer/mtcp/dpdk/include/cflags.txt)
INC += ${DPDK_MACHINE_FLAGS} -I${DPDK_INC} -include $(DPDK_INC)/rte_config.h
endif

ifeq ($(DPDK),1)
DPDK_LIB_FLAGS = $(shell cat /proj/sequencer/mtcp/dpdk/lib/ldflags.txt)
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}
else
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}
endif

all: TCPEchoServer-Thread

#	gcc -Wall -o server TCPEchoServer-Thread.c DieWithError.c HandleTCPClient.c CreateTCPServerSocket.c AcceptTCPConnection.c $(LIB_DIR) $(LIBS) $(CFLAGS)
TCPEchoServer-Thread.o: TCPEchoServer-Thread.c DieWithError.c HandleTCPClient.c CreateTCPServerSocket.c AcceptTCPConnection.c
	${CC} -c $^ ${CFLAGS} ${INC}

TCPEchoServer-Thread: TCPEchoServer-Thread.o DieWithError.o HandleTCPClient.o CreateTCPServerSocket.o AcceptTCPConnection.o
	${CC} $^ ${LIBS} -o $@

clean:
	rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile

