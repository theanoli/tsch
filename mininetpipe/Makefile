#/######################################################################
# This is the makefile for this test harness.
# Type make with one of the following choices for environments:
#
#      tcp         : You start the receiver and transmitter manually
#      mtcp        : Same as TCP, but uses the mTCP user-mode stack
#
#      For more information, see the function PrintUsage () in harness.c
#
########################################################################

CC         = gcc
CFLAGS     = -g -O3 -Wall -Werror -pthread
SRC        = ./src

all: tcp mtcp udp 

clean:
	rm -f *.o NP* np.out

#--- All the stuff needed to compile mTCP ---#
# DPDK library and header 
DPDK_INC	= /proj/sequencer/mtcp/dpdk/include
DPDK_LIB	= /proj/sequencer/mtcp/dpdk/lib/

# mTCP library and header 
MTCP_FLD    = /proj/sequencer/mtcp/mtcp
MTCP_INC    = -I${MTCP_FLD}/include
MTCP_LIB    = -L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a

UTIL_FLD = /proj/sequencer/mtcp/util
UTIL_INC = -I${UTIL_FLD}/include

INC 	= -I./include/ ${UTIL_INC} ${MTCP_INC} -I${UTIL_FLD}/include
LIBS	= ${MTCP_LIB}

# CFLAGS for DPDK-related compilation
INC 	+= ${MTCP_INC}
DPDK_MACHINE_FLAGS = $(shell cat /proj/sequencer/mtcp/dpdk/include/cflags.txt)
INC 	+= ${DPDK_MACHINE_FLAGS} -I${DPDK_INC} -include $(DPDK_INC)/rte_config.h

DPDK_LIB_FLAGS = $(shell cat /proj/sequencer/mtcp/dpdk/lib/ldflags.txt)
LIBS 	+= -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}


#--- Compile the binaries ---#

tcp: $(SRC)/tcp.c $(SRC)/harness.c $(SRC)/harness.h 
	$(CC) $(CFLAGS) $(SRC)/harness.c $(SRC)/tcp.c -DTCP -Dwhichproto=\"TCP\" -o NPtcp -I$(SRC)

udp: $(SRC)/udp.c $(SRC)/harness.c $(SRC)/harness.h 
	$(CC) $(CFLAGS) $(SRC)/harness.c $(SRC)/udp.c -DTCP -Dwhichproto=\"UDP\" -o NPudp -I$(SRC)

mtcp: $(SRC)/mtcp.c $(SRC)/harness.c $(SRC)/harness.h 
	$(CC) $(CFLAGS) $(SRC)/harness.c $(SRC)/mtcp.c -DTCP -Dwhichproto=\"mTCP\" -o NPmtcp -I$(SRC) ${INC} ${LIBS} -lmtcp -lnuma -pthread -lrt

