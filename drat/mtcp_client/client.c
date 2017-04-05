#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
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

#include <mtcp_api.h>
#include <mtcp_epoll.h>
#include "cpu.h"
#include "rss.h"
#include "http_parsing.h"
#include "debug.h"

#define MAX_CPUS 16
#define PSIZE 32

#define IP_RANGE 1

#define RTTBUFSIZE 0xA0000  // 660K bytes
#define CHARBUFSIZE 64
#define RTTBUFDUMP RTTBUFSIZE - 64
#define SAMPLEMOD 8  // collect sample every SAMPLEMOD messages
#define WRITETO 3  // Timeout for writing to file

#define CALC_MD5SUM FALSE

#define TIMEVAL_TO_MSEC(t)		((t.tv_sec * 1000) + (t.tv_usec / 1000))
#define TIMEVAL_TO_USEC(t)		((t.tv_sec * 1000000) + (t.tv_usec))
#define TS_GT(a,b)				((int64_t)((a)-(b)) > 0)

#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

/*----------------------------------------------------------------------------*/
static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
static char *conf_file = NULL;
/*----------------------------------------------------------------------------*/
static int num_cores;
static int core_limit;
/*----------------------------------------------------------------------------*/
static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;
/*----------------------------------------------------------------------------*/
static int concurrency;
static int max_fds;
static int sleeptime;
static char *output_dir = "temp";
/*----------------------------------------------------------------------------*/
int latency;
struct thread_context
{
	int core;

	mctx_t mctx;
	int ep;

	// Measurement infrastructure
	uint32_t nmessages; 	// track how many messages have been received
	int next_write;		// where to write the next message
	struct timespec first_send; 	// timestamp of first message sent
	struct timespec last_recv;	// timestamp of last message received
	time_t last_writetime;		// last time buffer was dumped to file
	FILE *lfile;	// latency file
	FILE *tfile;	// throughput file
	char *rtt_buf;	// buffer for RTT measurements
};
typedef struct thread_context* thread_context_t;
/*----------------------------------------------------------------------------*/
struct timespec diff ( struct timespec start, struct timespec end ); 
/*----------------------------------------------------------------------------*/
thread_context_t 
CreateContext(int core)
{
	thread_context_t ctx;
	char fname[64];

	ctx = (thread_context_t) calloc (1, sizeof (struct thread_context));
	if (!ctx) {
		perror ("malloc");
		TRACE_ERROR ("Failed to allocate memory for thread context.\n");
		return NULL;
	}
	ctx->core = core;

	ctx->mctx = mtcp_create_context(core);
	if (!ctx->mctx) {
		TRACE_ERROR ("Failed to create mtcp context.\n");
		return NULL;
	}

	if (latency) {
		// Set up buffers to collect RTT measurements
		ctx->rtt_buf = (char *) calloc (1, RTTBUFSIZE); 
		if (ctx->rtt_buf == NULL) {
			TRACE_ERROR ("Error allocating buffer\n");
			exit(1);
		}

		snprintf (fname, 32, "results/rtt_%d.txt", core);
		if ((ctx->lfile = fopen (fname, "w")) == NULL) {
			perror ("file creation");
			TRACE_ERROR ("Couldn't open RTT output file.\n");
		}
	}

	return ctx;
}
/*----------------------------------------------------------------------------*/
void 
DestroyContext(thread_context_t ctx) 
{
	int ret; 
	char fname[64] = {0};
	struct timespec exp_start, exp_end;
	
	// Write any remaining data to the output file	
	if (latency && (ctx->next_write > 0)) {
		ret = fwrite (ctx->rtt_buf, 1, ctx->next_write, ctx->lfile);
		if (ret < ctx->next_write) {
			// Don't actually exit: we need to gracefully destroy context
			perror ("Write to latency file");
		}
	}

	if (throughput) {
		snprintf (fname, 32, "results/tput_%d.txt", core);
		if ((ctx->tfile = fopen (fname, "w")) == NULL) {
			perror ("Opening throughput file");
		}
	
		exp_start = ctx->first_send;
		exp_end = ctx->last_recv;
		ret = fprintf (ctx->tfile, PSIZE, "%lld,%.9ld,%lld,%.9ld,%d\n", 
			(long long) exp_start.tv_sec, exp_start.tv_nsec,
			(long long) exp_end.tv_sec, exp_end.tv_nsec,
			ctx->nmessages);
		if (ret < 0) {
			perror ("Write to tput file");
		}
	}

	mtcp_destroy_context(ctx->mctx);
	fclose (ctx->lfile);
	free (ctx->rtt_buf);
	free (ctx);
}
/*----------------------------------------------------------------------------*/
static inline int 
CreateConnection(thread_context_t ctx)
{
	mctx_t mctx = ctx->mctx;
	struct mtcp_epoll_event ev;
	struct sockaddr_in addr;
	int sockid;
	int ret;

	sockid = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		TRACE_INFO("Failed to create socket!\n");
		return -1;
	}
	ret = mtcp_setsock_nonblock(mctx, sockid);
	if (ret < 0) {
		TRACE_ERROR("Failed to set socket in nonblocking mode.\n");
		exit(-1);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = daddr;
	addr.sin_port = dport;
	
	ret = mtcp_connect(mctx, sockid, 
			(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror("mtcp_connect");
			mtcp_close(mctx, sockid);
			return -1;
		}
	}

	ev.events = MTCP_EPOLLOUT;
	ev.data.sockid = sockid;
	mtcp_epoll_ctl(mctx, ctx->ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);

	return sockid;
}
/*----------------------------------------------------------------------------*/
static inline void 
CloseConnection(thread_context_t ctx, int sockid)
{
	mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
	mtcp_close(ctx->mctx, sockid);
	if (CreateConnection(ctx) < 0) {
		done[ctx->core] = TRUE;
	}
}
/*----------------------------------------------------------------------------*/
struct timespec 
timestamp (void)
{
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec; 
}
/*----------------------------------------------------------------------------*/
void 
SendPacket ( thread_context_t ctx, int sockid ) 
{
	mctx_t mctx = ctx->mctx;
	int n; 
	struct timespec start;

	char str[PSIZE] = {0};

	start = timestamp ();
	if (ctx->nmessages == 0) {
		ctx->last_writetime = time (0);
		ctx->first_send = start;
	}	
	snprintf ( str, PSIZE, "%lld.%.9ld", 
		(long long) start.tv_sec, start.tv_nsec ); 
	n = mtcp_write ( mctx, sockid, str, PSIZE ); 
	if ( n < 0 ) {
		perror ( "Error writing to socket..." );
		return; 
	}
}
/*----------------------------------------------------------------------------*/
void
ReceivePacket ( thread_context_t ctx, int sockid ) 
{
	mctx_t mctx = ctx->mctx;
	struct mtcp_epoll_event ev;

	int ret;
	char *saveptr;

	char *secs;
	char *ns; 
	struct timespec sendtime, recvtime;

	char rtt[CHARBUFSIZE] = {0};
	char str[PSIZE] = {0};

	ret = mtcp_read (mctx, sockid, str, PSIZE);
	if (ret < 0) {
		if (errno != EAGAIN) {
			CloseConnection (ctx, sockid);
			return;
		} else {
			ev.events = MTCP_EPOLLOUT;
			ev.data.sockid = sockid;
			mtcp_epoll_ctl (ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD,
					sockid, &ev);
			return;
		}
	} else if (ret == 0) {
		CloseConnection (ctx, sockid);
		return;
	}

	// Capture receive time
	recvtime = timestamp (); 	

	ctx->nmessages++;

	if (latency && ((ctx->nmessages % SAMPLEMOD) == 0)) {
		// Parse seconds/nanoseconds from received packet
		secs = strtok_r (str, ".", &saveptr);
		ns = strtok_r (NULL, ".", &saveptr); 

		// The received packet got chopped up; RTT is invalid
		if ((secs == NULL) || (ns == NULL)) {
			ev.events = MTCP_EPOLLOUT;
			ev.data.sockid = sockid;
			mtcp_epoll_ctl (ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD,
						sockid, &ev);
			return;
		}

		sendtime.tv_sec = atoi (secs); 
		sendtime.tv_nsec = atoi (ns); 

		ret = snprintf (ctx->rtt_buf[ctx->next_write], CHARBUFSIZE, 
			"%lld,%.9ld,%lld,%.9ld\n",
			(long long) sendtime.tv_sec, sendtime.tv_nsec, 
			(long long) recvtime.tv_sec, recvtime.tv_nsec);
		if (ret < 0) {
			perror ("Dump to buf");
			TRACE_ERROR ("Dump to buf failed\n");
		} else {
			memcpy (ctx->rtt_buf[ctx->next_write], rtt, ret);
			ctx->next_write += strlen (rtt);

			// We've exceeded the threshold for buffer dump or haven't 
			// written anything for longer than WRITETO secs; write 
			// buffer to file
			if ((ctx->next_write >= RTTBUFDUMP) || 
				(time (0) - ctx->last_writetime > WRITETO)) {
				ret = fwrite (ctx->rtt_buf, 1, ctx->next_write, 
						ctx->lfile);
				if (ret < ctx->next_write) {
					perror ("write");
					TRACE_ERROR ("RTT dump to file failed\n");
				}
				memset (ctx->rtt_buf, 0, RTTBUFSIZE);
				ctx->next_write = 0;
				ctx->last_writetime = time (0);
			}
		}
	}
	
	ctx->last_recv = recvtime;

	ev.events = MTCP_EPOLLOUT;
	ev.data.sockid = sockid;
	mtcp_epoll_ctl (ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sockid, &ev);
}
/*----------------------------------------------------------------------------*/
struct timespec
diff ( struct timespec start, struct timespec end )
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000 + end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}
/*----------------------------------------------------------------------------*/
void *
RunEchoClient(void *arg)
{
	thread_context_t ctx;
	mctx_t mctx;
	int core = *(int *)arg;
	struct in_addr daddr_in;
	int i;
	int maxevents;
	int ep;
	struct mtcp_epoll_event *events;
	int nevents;

	mtcp_core_affinitize(core);

	ctx = CreateContext(core);
	if (!ctx) {
		return NULL;
	}
	mctx = ctx->mctx;
	srand(time(NULL));

	mtcp_init_rss(mctx, saddr, IP_RANGE, daddr, dport);

	daddr_in.s_addr = daddr;
	fprintf(stderr, "Thread %d connecting to %s:%u\n", 
			core, inet_ntoa(daddr_in), ntohs(dport));

	/* Initialization */
	maxevents = max_fds * 3;
	ep = mtcp_epoll_create(mctx, maxevents);
	if (ep < 0) {
		TRACE_ERROR("Failed to create epoll struct!n");
		exit(EXIT_FAILURE);
	}
	events = (struct mtcp_epoll_event *)
			calloc(maxevents, sizeof(struct mtcp_epoll_event));
	if (!events) {
		TRACE_ERROR("Failed to allocate events!\n");
		exit(EXIT_FAILURE);
	}
	ctx->ep = ep;

	while (!done[core]) {
		if (CreateConnection(ctx) < 0) {
			done[core] = TRUE;
			break;
		}

		nevents = mtcp_epoll_wait(mctx, ep, events, maxevents, -1);
	
		if (nevents < 0) {
			if (errno != EINTR) {
				TRACE_ERROR("mtcp_epoll_wait failed! ret: %d\n", nevents);
			}
			done[core] = TRUE;
			break;
		}	

		for (i = 0; i < nevents; i++) {
			if (events[i].events & MTCP_EPOLLERR) {
				int err;
				socklen_t len = sizeof(err);

				TRACE_APP("[CPU %d] Error on socket %d\n", 
						core, events[i].data.sockid);
				if (mtcp_getsockopt(mctx, events[i].data.sockid, 
					SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0) {
				}
				CloseConnection(ctx, events[i].data.sockid);
			} else if (events[i].events & MTCP_EPOLLOUT) {
				SendPacket ( ctx, events[i].data.sockid ); 
				ReceivePacket ( ctx, events[i].data.sockid ); 
				usleep (sleeptime);
			} else if ( events[i].events & MTCP_EPOLLIN ) {
				assert ( 1 );
			} else {
				TRACE_ERROR("Socket %d: event: %s\n", 
					events[i].data.sockid, EventToString(events[i].events));
				assert(0);
			}
		}
	}

	TRACE_INFO("Thread %d waiting for mtcp to be destroyed.\n", core);
	DestroyContext(ctx);

	TRACE_DBG("Thread %d finished.\n", core);
	pthread_exit(NULL);
	return NULL;
}
/*----------------------------------------------------------------------------*/
void 
SignalHandler(int signum)
{
	int i;

	for (i = 0; i < core_limit; i++) {
		done[i] = TRUE;
	}
}
/*----------------------------------------------------------------------------*/
static void
printHelp(const char *prog_name)
{
	TRACE_CONFIG("%s -s host_ip -o result_file [-c concurrency]"
		     " [-N num_cores] [-t sleeptime] [-h]\n",
		     prog_name);
	exit(EXIT_SUCCESS);
}
/*----------------------------------------------------------------------------*/
int 
main(int argc, char **argv)
{
	struct mtcp_conf mcfg;
	int cores[MAX_CPUS];
	int total_concurrency = 0;
	int ret;
	int i, o;

	char *hostname;
	int portno;
	
	// to time experiments
	time_t start;

	portno = 8000;
	hostname = NULL;
	
	// TODO update opts checking
	if ( argc < 2 ) {
		perror ( "You need arguments: \n" );
		printHelp(argv[0]);
		return 1; 
	}

	num_cores = GetNumCPUs();
	core_limit = num_cores;
	concurrency = 100;
	sleeptime = 0;
	latency = FALSE;

	// TODO argparse; make these actual args later
	while (-1 != (o = getopt(argc, argv, "N:s:f:o:c:t:lh"))) {
		switch (o) {
		case 'N':
			core_limit = atoi(optarg);
			if (core_limit > num_cores) {
				TRACE_CONFIG("CPU limit should be smaller than the "
					     "number of CPUs: %d\n", num_cores);
				return FALSE;
			}
			/** 
			 * it is important that core limit is set 
			 * before mtcp_init() is called. You can
			 * not set core_limit after mtcp_init()
			 */
			mtcp_getconf (&mcfg);
			mcfg.num_cores = core_limit;
			mtcp_setconf (&mcfg);
			break;
		case 's':
			hostname = optarg;
			break;
		case 'f': 
			conf_file = optarg;
			break;
		case 'c':
			total_concurrency = atoi (optarg);
			break;
		case 'o':
			output_dir = optarg;
			break;
		case 't':
			sleeptime = atoi (optarg);
			break;
		case 'l':
			latency = TRUE;
			break;
		case 'h':
			printHelp (argv[0]);
			break;
		}
	}

	if (hostname == NULL) {
		printf ("Usage:\n");
		printHelp (argv[0]);
	}

	daddr = inet_addr (hostname);
	dport = htons (portno);
	saddr = INADDR_ANY;

	/* per-core concurrency = total_concurrency / # cores */
	if (total_concurrency > 0) {
		concurrency = total_concurrency / core_limit;
	}

	/* set the max number of fds 3x larger than concurrency */
	max_fds = concurrency * 3;

	TRACE_CONFIG ("Application configuration:\n");
	TRACE_CONFIG ("# of cores: %d\n", core_limit);
	TRACE_CONFIG ("Concurrency: %d\n", total_concurrency);

	ret = mtcp_init ("epwget.conf");
	if (ret) {
		TRACE_ERROR ("Failed to initialize mtcp.\n");
		exit (EXIT_FAILURE);
	}
	mtcp_getconf (&mcfg);
	mcfg.max_concurrency = max_fds;
	mcfg.max_num_buffers = max_fds;
	mtcp_setconf (&mcfg);

	mtcp_register_signal (SIGINT, SignalHandler);

	for (i = 0; i < core_limit; i++) {
		cores[i] = i;
		done[i] = FALSE;

		if (pthread_create(&app_thread[i], 
					NULL, RunEchoClient, (void *)&cores[i])) {
			perror("pthread_create");
			TRACE_ERROR("Failed to create thread.\n");
			exit(-1);
		}
	}
	
	start = time (0); 
	while ((int) (time (0) - start) < 15) {
		for (i = 0; i < core_limit; i++) {
			if (done[i] == TRUE) {
				break;
			}
		}
	}
	TRACE_INFO ("Done looping--entering shutdown phase\n");
	for (i = 0; i < core_limit; i++) {
		done[i] = TRUE;
		pthread_join (app_thread[i], NULL);
		TRACE_INFO ("Thread %d joined.\n", i);
	}

	mtcp_destroy();
	return 0;
}
/*----------------------------------------------------------------------------*/
