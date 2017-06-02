########################################################################
# This is the makefile for this test harness.
# Type make with one of the following choices for environments:
#
#      tcp         : You start the receiver and transmitter manually
#      mtcp        : Same as TCP, but uses the mTCP user-mode stack
#
#      For more information, see the function printusage() in netpipe.c
#
########################################################################

CC         = cc
CFLAGS     = -O -g
SRC        = ./src

all:tcp 

clean:
	rm -f *.o NP* np.out


#--- Compile the binaries ---#

tcp: $(SRC)/tcp.c $(SRC)/netpipe.c $(SRC)/netpipe.h 
	$(CC) $(CFLAGS) $(SRC)/netpipe.c $(SRC)/tcp.c -DTCP -o NPtcp -I$(SRC)

mtcp: $(SRC)/mtcp.c $(SRC)/netpipe.c $(SRC)/netpipe.h 
	$(CC) $(CFLAGS) $(SRC)/netpipe.c $(SRC)/mtcp.c -DTCP -o NPmtcp -I$(SRC) -lmtcp -lnuma -pthread -lps -lrt

