#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <stdlib.h> 
#include <unistd.h> 

// TCP-specific
#if defined(TCP)
  #include <netdb.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>

  typedef struct protocolstruct ProtocolStruct;
  struct protocolstruct
  {
      struct sockaddr_in      sin1,   /* socket structure #1              */
                              sin2;   /* socket structure #2              */
      int                     nodelay;  /* Flag for TCP nodelay           */
      struct hostent          *addr;    /* Address of host                */
      int                     sndbufsz, /* Size of TCP send buffer        */
                              rcvbufsz; /* Size of TCP receive buffer     */
  };
#else 
  // For now, we only have one option; will have more when we define our own
  // protocol TODO
  #error "TCP must be defined during compilation!"
#endif


typedef struct argstruct ArgStruct;
struct argstruct 
{
    /* This is the common information that is needed for all tests           */
    int      cache;         /* Cache flag, 0 => limit cache, 1=> use cache   */
    char     *host;         /* Name of receiving host                        */

    int      servicefd,     /* File descriptor of the network socket         */
             commfd;        /* Communication file descriptor                 */
    short    port;          /* Port used for connection                      */
    char     *r_buff;       /* Aligned receive buffer                        */
    char     *r_buff_orig;  /* Original unaligned receive buffer             */
#if defined(USE_VOLATILE_RPTR)
    volatile                /* use volatile if polling on buffer in module   */
#endif
    char     *r_ptr;        /* Pointer to current location in send buffer    */
    char     *r_ptr_saved;  /* Pointer for saving value of r_ptr             */
    char     *s_buff;       /* Aligned send buffer                           */
    char     *s_buff_orig;  /* Original unaligned send buffer                */
    char     *s_ptr;        /* Pointer to current location in send buffer    */

    int      bufflen,       /* Length of transmitted buffer                  */
             upper,         /* Upper limit to bufflen                        */
             tr,rcv,        /* Transmit and Recv flags, or maybe neither     */
             bidir,         /* Bi-directional flag                           */
             nbuff;         /* Number of buffers to transmit                 */

    int      source_node;   /* Set to -1 (MPI_ANY_SOURCE) if -z specified    */
    int      reset_conn;    /* Reset connection flag                         */
    int      soffset,roffset;
    int      syncflag; /* flag for using sync sends vs. normal sends in MPI mod*/
    /* Now we work with a union of information for protocol dependent stuff  */
    ProtocolStruct prot;
};

typedef struct data Data;
struct data
{
    double t;
    double bps;
    double variance;
    int    bits;
    int    repeat;
};

double When();

void Init(ArgStruct *p, int* argc, char*** argv);

void Setup(ArgStruct *p);

void establish(ArgStruct *p);

void Sync(ArgStruct *p);

void PrepareToReceive(ArgStruct *p);

void SendData(ArgStruct *p);

void RecvData(ArgStruct *p);

void SendTime(ArgStruct *p, double *t);

void RecvTime(ArgStruct *p, double *t);

void SendRepeat(ArgStruct *p, int rpt);

void RecvRepeat(ArgStruct *p, int *rpt);

void FreeBuff(char *buff1, char *buff2);

void CleanUp(ArgStruct *p);

void InitBufferData(ArgStruct *p, int nbytes, int soffset, int roffset);

void MyMalloc(ArgStruct *p, int bufflen, int soffset, int roffset);

void Reset(ArgStruct *p);

void mymemset(int *ptr, int c, int n);

void flushcache(int *ptr, int n);

void SetIntegrityData(ArgStruct *p);

void VerifyIntegrity(ArgStruct *p);

void* AlignBuffer(void* buff, int boundary);

void AdvanceSendPtr(ArgStruct* p, int blocksize);

void AdvanceRecvPtr(ArgStruct* p, int blocksize);

void SaveRecvPtr(ArgStruct* p);

void ResetRecvPtr(ArgStruct* p);

void PrintUsage();

int getopt( int argc, char * const argv[], const char *optstring);

void AfterAlignmentInit( ArgStruct *p );
