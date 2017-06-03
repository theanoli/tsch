TARGETS = client
CC=gcc -g -O3 -Wall -Werror -fgnu89-inline
DPDK=1

# DPDK LIBRARY and HEADER
DPDK_INC=/proj/sequencer/mtcp/dpdk/include
DPDK_LIB=/proj/sequencer/mtcp/dpdk/lib/

# mtcp library and header 
MTCP_FLD    =/proj/sequencer/mtcp/mtcp
MTCP_INC    =-I${MTCP_FLD}/include
MTCP_LIB    =-L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a

UTIL_FLD = /proj/sequencer/mtcp/util
UTIL_INC = -I${UTIL_FLD}/include

INC = -I./include/ ${UTIL_INC} ${MTCP_INC} -I${UTIL_FLD}/include
LIBS = ${MTCP_LIB}

# CFLAGS for DPDK-related compilation
INC += ${MTCP_INC}
DPDK_MACHINE_FLAGS = $(shell cat /proj/sequencer/mtcp/dpdk/include/cflags.txt)
INC += ${DPDK_MACHINE_FLAGS} -I${DPDK_INC} -include $(DPDK_INC)/rte_config.h

DPDK_LIB_FLAGS = $(shell cat /proj/sequencer/mtcp/dpdk/lib/ldflags.txt)
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}

all: client 

client.o: client.c
	${CC} -c $^ ${CFLAGS} ${INC}

client: client.o
	${CC} $^ ${LIBS} -o $@

clean:
	rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile

