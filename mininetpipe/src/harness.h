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
#define NTHREADS 32
#define NSERVERCORES 32
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

// Global data structures
typedef enum program_state {
    startup,
    warmup,
    experiment,
    cooldown,
    end
} ProgramState;

// Thread-specific data structures
typedef struct threadargs ThreadArgs;
struct threadargs 
{
    /* This is the common information that is needed for all tests           */
    char *  machineid; /* Machine ID   */ 
    int     threadid;       /* The thread number                            */
    char    threadname[128];    /* MachineID.threadID for printing          */

    int     servicefd,     /* File descriptor of the network socket         */
            commfd;        /* Communication file descriptor                 */
    short   port;          /* Port used for connection                      */
    ProtocolStruct prot;   /* Protocol-depended stuff                       */

    char    *host;          /* Name of receiving host                       */
    char    *outfile;       /* Where results go to die                      */
    int     tr, rcv;
    int     latency;        /* 1 if this is a latency experiment            */

    char    *r_ptr;        /* Pointer to current location in receive buffer */
    char    *s_ptr;        /* Pointer to current location in send buffer    */
    char    rbuff[PSIZE + 1];  /* Receive buffer                                */
    char    sbuff[PSIZE + 1];  /* Send buffer                                   */

    int     bufflen;       /* Length of transmitted buffer                  */

    char    *lbuff;          /* For saving latency measurements */

    // for throughput measurements
    int     online_wait;    /* How long to wait for clients to come up */
    uint64_t counter;       /* For counting packets!                        */
    double  duration;       /* Measured time over which packets are blasted */

    // timer data 
    volatile ProgramState program_state;
    double t0;
    int tput_done;
};

typedef struct programargs ProgramArgs;
struct programargs
{
    volatile ProgramState    program_state;
    char *  machineid;      /* Machine id */
    int     pinthreads;     /* Pin threads to cores                         */
    int     latency;        /* Measure latency (1) or throughput (0)        */
    int     expduration;    /* How long to count packets                    */
    int     online_wait;    /* Tput: how long to wait for clients to come up */
    char    *host;          /* Name of receiving host                       */
    short   port;
    int     collect_stats;  /* Collect stats on resource usage              */
    int     tr,rcv;         /* Transmit and Recv flags, or maybe neither    */
    int     nthreads;       /* How many threads to launch                   */
    char    *outfile;       /* Where results go to die                      */

    char    sbuff[PSIZE + 1];   /* Tput: the string that will be sent       */

    pthread_t *tids;        /* Thread handles                               */
    ThreadArgs *thread_data;    /* Array of per-thread data structures      */

    // Possibly obsolete
    int     ncli;           /* For throughput: number of clients in exp     */
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


void UpdateProgramState (ProgramState state);

double When ();

struct timespec When2 ();

void Init (ProgramArgs *p, int* argc, char*** argv);

void Setup (ThreadArgs *p);

void ThroughputSetup (ThreadArgs *p);

void establish (ThreadArgs *p);

void throughput_establish (ThreadArgs *p);

int setsock_nonblock (int fd);

void SendData (ThreadArgs *p);

char *RecvData (ThreadArgs *p);

void SimpleWrite (ThreadArgs *p);

void LaunchThreads (ProgramArgs *p);

void *ThreadEntry (void *vargp);

void SimpleRx (ThreadArgs *p);

void *SimpleTx (void *vargp);

void TimestampWrite (ThreadArgs *p);

void Echo (ThreadArgs *p);

void CleanUp(ThreadArgs *p);

void PrintUsage();

void SignalHandler (int signum);

void CollectStats (ProgramArgs *p);

int getopt( int argc, char * const argv[], const char *optstring);
