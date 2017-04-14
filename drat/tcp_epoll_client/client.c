#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <assert.h>
#include <error.h>

#define MAX_CPUS 16
#define PSIZE 32

#define RTTBUFSIZE 0xA0000
#define CHARBUFSIZE 64
#define RTTBUFDUMP RTTBUFSIZE - 64
#define SAMPLEMOD 8  // collect sample every SAMPLEMOD messages
#define WRITETO 3  // Timeout for writing to file

#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
/*----------------------------------------------------------------------------*/
static int num_cores;
static int core_limit;
/*----------------------------------------------------------------------------*/
static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;
/*----------------------------------------------------------------------------*/
static int sleeptime;
static int duration; 
static char *output_dir;
/*----------------------------------------------------------------------------*/
int latency;
int throughput;
struct thread_context
{
	int core;
	int ep;

	// Measurement infrastructure
	uint32_t nmessages; 	// track how many messages have been received
	int next_write;		// where to write the next message
	struct timespec first_send; 	// timestamp of first message sent
	struct timespec last_recv;	// timestamp of last message received
	time_t last_writetime;		// last time buffer was dumped to file
	char lfname[CHARBUFSIZE];
	char tfname[CHARBUFSIZE];
	FILE *lfile;	// latency file
	FILE *tfile;	// throughput file
	char *rtt_buf;	// buffer for RTT measurements
};
typedef struct thread_context* thread_context_t;
static struct thread_context *g_ctx[MAX_CPUS];
/*----------------------------------------------------------------------------*/
struct timespec diff ( struct timespec start, struct timespec end ); 

