#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <assert.h>
#include <errno.h>

#include "utilities.h"

#define MAX_EVENTS 4096
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
/*----------------------------------------------------------------------------*/
thread_context_t
CreateContext (int core)
{
	thread_context_t ctx;

	ctx = (thread_context_t) calloc (1, sizeof (struct thread_context));
	if (!ctx) {
		perror ("malloc");
		printf ("Failed to allocate memory for thread context.\n");
		return NULL;
	}
	ctx->core = core;

	if (latency) {
		// Set up buffers to collect RTT measurements
		ctx->rtt_buf = (char *) calloc (1, RTTBUFSIZE); 
		if (ctx->rtt_buf == NULL) {
			printf ("Error allocating buffer\n");
			exit(1);
		}

		snprintf (ctx->lfname, CHARBUFSIZE, "results/%s/rtt_%d.dat", 
			output_dir, core);
		if ((ctx->lfile = fopen (ctx->lfname, "wb")) == NULL) {
			perror ("file creation");
			printf ("Couldn't open RTT output file.\n");
		}
	}

	if (throughput) {
		// This will get opened later
		snprintf (ctx->tfname, CHARBUFSIZE, "results/%s/tput_%d.dat",
			output_dir, core);
	}

	return ctx;
}
/*----------------------------------------------------------------------------*/
void 
DestroyContext (thread_context_t ctx) 
{
	int ret; 
	struct timespec exp_start, exp_end;
	
	// Write any remaining data to the output file	
	if (latency && (ctx->next_write > 0)) {
		printf ("Writing to latency file...\n");
		ret = fwrite (ctx->rtt_buf, 1, ctx->next_write, ctx->lfile);
		if (ret < ctx->next_write) {
			// Don't actually exit: we need to gracefully destroy context
			perror ("Write to latency file");
		}
		fclose (ctx->lfile);
	}

	if (throughput) {
		printf ("Writing to throughput file...\n");
		if ((ctx->tfile = fopen (ctx->tfname, "wb")) == NULL) {
			perror ("Opening throughput file");
		}
	
		exp_start = ctx->first_send;
		exp_end = ctx->last_recv;
		ret = fprintf (ctx->tfile, "%lld,%.9ld,%lld,%.9ld,%d\n", 
			(long long) exp_start.tv_sec, exp_start.tv_nsec,
			(long long) exp_end.tv_sec, exp_end.tv_nsec,
			ctx->nmessages);
		if (ret < 0) {
			perror ("Write to tput file");
		}
		fclose (ctx->tfile);
	}
	
	free (ctx->rtt_buf);
	printf ("Wrote out data (if any); destroying context %d\n", ctx->core);
	free (ctx);
}
/*----------------------------------------------------------------------------*/
static inline int 
CreateConnection (thread_context_t ctx)
{
	struct epoll_event ev;
	struct sockaddr_in addr;
	int sockid;
	int ret;

	sockid = socket (AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		printf ("Failed to create socket!\n");
		return -1;
	}

	//ret = setsock_nonblock (sockid);
	//if (ret < 0) {
	//	printf("Failed to set socket in nonblocking mode.\n");
	//	exit(-1);
	//}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = daddr;
	addr.sin_port = dport;
	
	ret = connect (sockid, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror ("connect");
			close (sockid);
			return -1;
		}
	}

	printf ("Connected successfully!\n");
	ev.events = EPOLLOUT;
	ev.data.fd = sockid;
	epoll_ctl (ctx->ep, EPOLL_CTL_ADD, sockid, &ev);

	return sockid;
}
/*----------------------------------------------------------------------------*/
static inline void 
CloseConnection(thread_context_t ctx, int sockid)
{
	epoll_ctl (ctx->ep, EPOLL_CTL_DEL, sockid, NULL);
	close (sockid);
	if (CreateConnection (ctx) < 0) {
		done[ctx->core] = TRUE;
	}
}
/*----------------------------------------------------------------------------*/
void 
SendPacket (thread_context_t ctx, int sockid) 
{
	int n; 
	struct timespec start;

	char str[PSIZE] = {0};

	start = timestamp ();
	if (ctx->nmessages == 0) {
		ctx->last_writetime = time (0);
		ctx->first_send = start;
	}	
	snprintf (str, PSIZE, "%lld.%.9ld", 
		(long long) start.tv_sec, start.tv_nsec); 
	n = write (sockid, str, PSIZE); 
	if ( n < 0 ) {
		perror ( "Error writing to socket..." );
		return; 
	}
}
/*----------------------------------------------------------------------------*/
void
ReceivePacket (thread_context_t ctx, int sockid) 
{
	struct epoll_event ev;

	int ret;
	char *saveptr;

	char *secs;
	char *ns; 
	struct timespec sendtime, recvtime;

	char str[PSIZE] = {0};

	ret = read (sockid, str, PSIZE);
	if (ret < 0) {
		if (errno != EAGAIN) {
			CloseConnection (ctx, sockid);
			return;
		} else {
			ev.events = EPOLLOUT;
			ev.data.fd = sockid;
			epoll_ctl (ctx->ep, EPOLL_CTL_MOD, sockid, &ev);
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
			ev.events = EPOLLOUT;
			ev.data.fd = sockid;
			epoll_ctl (ctx->ep, EPOLL_CTL_MOD, sockid, &ev);
			return;
		}

		sendtime.tv_sec = atoi (secs); 
		sendtime.tv_nsec = atoi (ns); 
		
		ret = snprintf (&ctx->rtt_buf[ctx->next_write], CHARBUFSIZE, 
			"%lld,%.9ld,%lld,%.9ld\n",
			(long long) sendtime.tv_sec, sendtime.tv_nsec, 
			(long long) recvtime.tv_sec, recvtime.tv_nsec);
		if (ret < 0) {
			perror ("Dump to buf");
			printf ("Dump to buf failed\n");
		} else {
			ctx->next_write += ret;

			// We've exceeded the threshold for buffer dump or haven't 
			// written anything for longer than WRITETO secs; write 
			// buffer to file
			if ((ctx->next_write >= RTTBUFDUMP) || 
				(time (0) - ctx->last_writetime > WRITETO)) {
				printf ("Writing %d bytes to latency file...\n", ctx->next_write);
				ret = fwrite (ctx->rtt_buf, 1, ctx->next_write, 
						ctx->lfile);
				if (ret < ctx->next_write) {
					perror ("write");
					printf ("RTT dump to file failed\n");
				}
				memset (ctx->rtt_buf, 0, RTTBUFSIZE);
				ctx->next_write = 0;
				ctx->last_writetime = time (0);
			}
		}
	}
	
	ctx->last_recv = recvtime;

	ev.events = EPOLLOUT;
	ev.data.fd = sockid;
	epoll_ctl (ctx->ep, EPOLL_CTL_MOD, sockid, &ev);
	
}
/*----------------------------------------------------------------------------*/
void *
RunEchoClient(void *arg)
{
	thread_context_t ctx;
	int core = *(int *)arg;
	struct in_addr daddr_in;
	int i;
	int ep;
	struct epoll_event *events;
	int nevents;

	ctx = CreateContext (core);
	if (!ctx) {
		return NULL;
	}
	srand(time(NULL));

	daddr_in.s_addr = daddr;
	fprintf (stderr, "Thread %d connecting to %s:%u\n", 
			core, inet_ntoa (daddr_in), ntohs (dport));

	/* Initialization */
	ep = epoll_create1 (0);
	if (ep < 0) {
		printf ("Failed to create epoll struct!\n");
		exit (-1);
	}
	events = (struct epoll_event *) 
			calloc (MAX_EVENTS, sizeof(struct epoll_event));
	if (!events) {
		printf ("Failed to allocate events!\n");
		exit(-1);
	}
	ctx->ep = ep;

	if (CreateConnection(ctx) < 0) {
		done[core] = TRUE;
	}

	while (!done[core]) {
		nevents = epoll_wait (ep, events, MAX_EVENTS, -1);
	
		if (nevents < 0) {
			if (errno != EINTR) {
				printf ("epoll_wait failed! ret: %d\n", nevents);
			}
			done[core] = TRUE;
			break;
		}	

		for (i = 0; i < nevents; i++) {
			if (events[i].events & EPOLLERR) {
				int err;
				socklen_t len = sizeof(err);

				printf ("[CPU %d] Error on socket %d\n", 
						core, events[i].data.fd);
				if (getsockopt (events[i].data.fd, 
					SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0) {
				}
				CloseConnection (ctx, events[i].data.fd);
			} else if (events[i].events & EPOLLOUT) {
				SendPacket (ctx, events[i].data.fd); 
				ReceivePacket (ctx, events[i].data.fd); 
			} else if (events[i].events & EPOLLIN) {
				printf ("Got an EPOLLIN\n");
				assert (1);
			} else {
				assert (0);
			}
			usleep (sleeptime);
		}
	}

	printf ("Destroying context on thread %d\n", core);
	DestroyContext(ctx);

	printf ("Thread %d destoyed.\n", core);
	pthread_exit(NULL);
	return NULL;
}
/*----------------------------------------------------------------------------*/
void 
SignalHandler(int signum)
{
	int i;

	for (i = 0; i < num_cores; i++) {
		done[i] = TRUE;
	}
}
/*----------------------------------------------------------------------------*/
static void
printHelp(const char *prog_name)
{
	printf("%s -s host_ip -o result_file"
		     " [-N num_cores] [-u sleeptime (usec)] [-h (help)]" 
			" [-t (throughput)] [-l (latency)]\n",
		     prog_name);
	exit (0);
}
/*----------------------------------------------------------------------------*/
int 
main(int argc, char **argv)
{
	int cores[MAX_CPUS];
	int ret;
	int i, o;
	char outdirname[CHARBUFSIZE];

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

	num_cores = 1;
	sleeptime = 0;
	duration = 15;
	latency = FALSE;
	throughput = FALSE;
	output_dir = "temp";

	while (-1 != (o = getopt(argc, argv, "N:s:o:u:d:lth"))) {
		switch (o) {
		case 'N':
			num_cores = atoi(optarg);
			break;
		case 's':
			hostname = optarg;
			break;
		case 'o':
			output_dir = optarg;
			break;
		case 'u':
			sleeptime = atoi (optarg);
			break;
		case 'd':
			duration = atoi (optarg);
			break;
		case 'l':
			latency = TRUE;
			break;
		case 't':
			throughput = TRUE;
			break;
		case 'h':
			printHelp (argv[0]);
			break;
		}
	}

	snprintf (outdirname, CHARBUFSIZE, "results/%s", output_dir);
	if (throughput || latency) {
		printf ("Creating output directory for results: %s\n", outdirname);
		if (((mkdir (outdirname, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) &&
			(errno != EEXIST)) {
			perror ("output dir creation");
			printf ("Output directory creation failed!\n");
		}
	}

	signal (SIGINT, SignalHandler);

	if (hostname == NULL) {
		printf ("Usage:\n");
		printHelp (argv[0]);
	}

	daddr = inet_addr (hostname);
	dport = htons (portno);
	saddr = INADDR_ANY;

	printf ("------------------------------------------------------------\n");
	printf ("Application configuration:\n");
	printf ("# of cores: %d\n", num_cores);
	printf ("Experiment duration: %d\n", duration);
	printf ("Sleeptime between send and receive (us): %d\n", sleeptime);
	printf ("------------------------------------------------------------\n");


	for (i = 0; i < num_cores; i++) {
		cores[i] = i;
		done[i] = FALSE;

		if (pthread_create (&app_thread[i], 
					NULL, RunEchoClient, (void *)&cores[i])) {
			perror ("pthread_create");
			printf ("Failed to create thread.\n");
			exit (-1);
		}
	}
	
	start = time (0); 
	while ((int) (time (0) - start) < duration) {
		for (i = 0; i < num_cores; i++) {
			if (done[i] == TRUE) {
				break;
			}
		}
	}
	printf ("Done looping--entering shutdown phase\n");
	for (i = 0; i < num_cores; i++) {
		printf ("Setting thread %d to true\n", i);
		done[i] = TRUE;
	}

	for (i = 0; i < num_cores; i++) {
		pthread_join (app_thread[i], NULL);
		printf ("Thread %d joined.\n", i);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
	
