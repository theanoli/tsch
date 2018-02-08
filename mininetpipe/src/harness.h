#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <time.h>
#include <unistd.h> 

#define PSIZE   32
#define DEFPORT 8000
#define EXPDURATION 10
#define MAXEVENTS 8192

// For throughput experiments: time to wait before
// starting measurements
#define WARMUP 3
#define COOLDOWN 5

// TCP-specific
#if defined(TCP)
  #include <netdb.h>
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

typedef enum program_state {
    warmup,
    experiment,
    cooldown,
    end
} ProgramState;

typedef struct argstruct ArgStruct;
struct argstruct 
{
    /* This is the common information that is needed for all tests           */
    char    *host;         /* Name of receiving host                        */

    int     servicefd,     /* File descriptor of the network socket         */
            commfd;        /* Communication file descriptor                 */
    short   port;          /* Port used for connection                      */
    int     ncli;           /* For throughput: number of clients in exp     */
    int     online_wait;    /* Tput: how long to wait for clients to come up */
    int     collect_stats;  /* Collect stats on resource usage              */
    char    *outfile;       /* Where results go to die                      */

    char    *r_ptr;        /* Pointer to current location in receive buffer */
    char    *s_ptr;        /* Pointer to current location in send buffer    */
    char    rbuff[PSIZE + 1];  /* Receive buffer                                */
    char    sbuff[PSIZE + 1];  /* Send buffer                                   */

    int     bufflen,       /* Length of transmitted buffer                  */
            tr,rcv;        /* Transmit and Recv flags, or maybe neither     */

    int     source_node;   /* Set to -1 (MPI_ANY_SOURCE) if -z specified    */
    int     reset_conn;    /* Reset connection flag                         */

    int     latency;        /* Measure latency (1) or throughput (0)        */
    char    *lbuff;          /* For saving latency measurements */

    // for throughput measurements
    int     expduration;    /* How long to count packets                    */
    uint64_t counter;       /* For counting packets!                        */
    double  duration;       /* Measured time over which packets are blasted */
    // timer stuff
    ProgramState program_state;
    double t0;
    int tput_done;

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

double When ();

struct timespec When2 ();

void Init(ArgStruct *p, int* argc, char*** argv);

void Setup(ArgStruct *p);

void ThroughputSetup (ArgStruct *p);

void establish(ArgStruct *p);

void throughput_establish (ArgStruct *p);

int setsock_nonblock (int fd);

void Sync(ArgStruct *p);

void PrepareToReceive(ArgStruct *p);

void SendData(ArgStruct *p);

char *RecvData(ArgStruct *p);

void SimpleWrite (ArgStruct *p);

void TimestampWrite (ArgStruct *p);

void Echo (ArgStruct *p);

void SendTime(ArgStruct *p, double *t);

void RecvTime(ArgStruct *p, double *t);

void SendRepeat(ArgStruct *p, int rpt);

void RecvRepeat(ArgStruct *p, int *rpt);

void FreeBuff(char *buff1, char *buff2);

void CleanUp(ArgStruct *p);

void Reset(ArgStruct *p);

void PrintUsage();

void SignalHandler (int signum);

void ExitStrategy ();

void CollectStats (ArgStruct *p);

int getopt( int argc, char * const argv[], const char *optstring);
