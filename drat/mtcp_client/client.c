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

#define MAX_URL_LEN 128
#define MAX_FILE_LEN 128
#define HTTP_HEADER_LEN 1024

#define IP_RANGE 1
#define MAX_IP_STR_LEN 16

#define BUF_SIZE (8*1024)

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
static mctx_t g_mctx[MAX_CPUS];
static int done[MAX_CPUS];
/*----------------------------------------------------------------------------*/
static int num_cores;
static int core_limit;
/*----------------------------------------------------------------------------*/
static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;
FILE *fd;
/*----------------------------------------------------------------------------*/
static int concurrency;
static int max_fds;
/*----------------------------------------------------------------------------*/
struct thread_context
{
	int core;

	mctx_t mctx;
	int ep;
};
typedef struct thread_context* thread_context_t;
/*----------------------------------------------------------------------------*/
static struct thread_context *g_ctx[MAX_CPUS];
/*----------------------------------------------------------------------------*/
struct timespec diff ( struct timespec start, struct timespec end ); 
/*----------------------------------------------------------------------------*/
thread_context_t 
CreateContext(int core)
{
	thread_context_t ctx;

	ctx = (thread_context_t)calloc(1, sizeof(struct thread_context));
	if (!ctx) {
		perror("malloc");
		TRACE_ERROR("Failed to allocate memory for thread context.\n");
		return NULL;
	}
	ctx->core = core;

	ctx->mctx = mtcp_create_context(core);
	if (!ctx->mctx) {
		TRACE_ERROR("Failed to create mtcp context.\n");
		return NULL;
	}
	g_mctx[core] = ctx->mctx;

	return ctx;
}
/*----------------------------------------------------------------------------*/
void 
DestroyContext(thread_context_t ctx) 
{
	mtcp_destroy_context(ctx->mctx);
	free(ctx);
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
recv_packet ( thread_context_t ctx, int sockid ) 
{
	mctx_t mctx = ctx->mctx;
	int rd;
	char *str;
	char *str_copy;
	char *secs;
	char *ns; 
	char *rtt;
	struct timespec parsed_start, end, tdiff;

	str = malloc ( PSIZE * sizeof ( char ) ); 
	if ( str == NULL ) {
		perror ( "Malloc error" ); 
		return;
	}

	memset ( str, '\0', PSIZE ); 

	rd = 1;
	while (rd > 0) {
		rd = mtcp_read ( mctx, sockid, str, PSIZE ); 
		if (rd <= 0)
			break;

		TRACE_APP("Socket %d: mtcp_read ret: %d\n",
				sockid, rd);
	}	

	if (rd > 0) {
		TRACE_APP("Socket %d Done\n", sockid ); 

	} else if (rd == 0) {
		/* connection closed by remote host */
		TRACE_DBG("Socket %d connection closed with server.\n", sockid);

		CloseConnection(ctx, sockid);

	} else if (rd < 0) {
		if (errno != EAGAIN) {
			TRACE_DBG("Socket %d: mtcp_read() error %s\n", 
					sockid, strerror(errno));
			CloseConnection(ctx, sockid);
		}
	}

	end = timestamp (); 	

	str_copy = malloc ( PSIZE * sizeof ( char ) );
	if ( str_copy == NULL ) {
		perror ( "Malloc error" );
		free ( str );
		return;
	}
	strncpy ( str_copy, str, PSIZE );

	secs = strtok ( str, "." );
	ns = strtok ( NULL, "." ); 

	if ( ( secs == NULL ) || ( ns == NULL ) ) {
		perror ( "STRTOK returned NULL prematurely" );
		free ( str_copy ); 
		free ( str );
		return;
	}

	parsed_start.tv_sec = atoi ( secs ); 
	parsed_start.tv_nsec = atoi ( ns ); 

	tdiff = diff ( parsed_start, end ); 

	rtt = malloc ( PSIZE * 2 * sizeof ( char ) ); 
	if ( rtt == NULL ) {
		free ( str );
		free ( str_copy );
		perror ( "Malloc error" ); 
		return;
	}
	memset ( rtt, '\0', PSIZE * 2 ); 

	snprintf ( rtt, PSIZE, "%lld,%.9ld", 
		(long long) end.tv_sec, end.tv_nsec ); 
	snprintf ( rtt + strlen ( rtt ), PSIZE, ",%lld,%.9ld\n", 
		(long long) tdiff.tv_sec, tdiff.tv_nsec );

	// TODO 
	//if ( fprintf ( fd, "%s", rtt ) <= 0 ) {
	//	printf ( "Nothing written!\n" );
	//}
	
	free ( rtt );  // current time timestamp + rtt
	free ( str_copy ); 
	free ( str );
}
/*----------------------------------------------------------------------------*/
void 
send_packet ( thread_context_t ctx, int sockid ) 
{
	mctx_t mctx = ctx->mctx;
	int n; 
	char *str;
	struct mtcp_epoll_event ev;
	struct timespec start;

	str = malloc ( PSIZE * sizeof ( char ) ); 
	if ( str == NULL ) {
		perror ( "Malloc error" ); 
		return;
	}

	memset ( str, '\0', PSIZE ); 

	start = timestamp (); 
	snprintf ( str, PSIZE, "%lld.%.9ld", 
		(long long) start.tv_sec, start.tv_nsec ); 
	n = mtcp_write ( mctx, sockid, str, PSIZE ); 
	if ( n < 0 ) {
		perror ( "Error writing to socket..." );
		free ( str ); 
		return; 
	}
	TRACE_APP("Socket %d sent.\n", sockid);
	ev.events = MTCP_EPOLLIN;
	ev.data.sockid = sockid;
	mtcp_epoll_ctl ( ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sockid, &ev );

	free ( str );
}
/*----------------------------------------------------------------------------*/
struct timespec
diff ( struct timespec start, struct timespec end )
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
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
	g_ctx[core] = ctx;
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

			} else if (events[i].events & MTCP_EPOLLIN) {
				recv_packet ( ctx, events[i].data.sockid ); 

			} else if (events[i].events == MTCP_EPOLLOUT) {
				send_packet ( ctx, events[i].data.sockid ); 

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
int 
main(int argc, char **argv)
{
	struct mtcp_conf mcfg;
	int cores[MAX_CPUS];
	int total_concurrency = 0;
	int ret;
	int i;

	char *hostname;
	int portno;
	
	// to time experiments
	time_t start;

	// This stuff doesn't do anything right now...
	// char *fname;
	// int exp_duration;
	
	// TODO do getopts
	if ( argc < 3 ) {
		perror ( "You need arguments: <hostname> <portno>" ); 
		return 1; 
	}

	hostname = argv[1];
	portno = atoi ( argv[2] );
	// fname = argv[3]; 
	// exp_duration = atoi ( argv[4] ); 

	daddr = inet_addr(hostname);
	dport = htons(portno);
	saddr = INADDR_ANY;

	num_cores = GetNumCPUs();
	core_limit = num_cores;
	concurrency = 100;

	// TODO argparse; make these actual args later
	core_limit = 1;
	total_concurrency = 100;

	mtcp_getconf ( &mcfg );
	mcfg.num_cores = core_limit;
	mtcp_setconf ( &mcfg );
	
	// for (i = 3; i < argc - 1; i++) {
	// 	if (strcmp(argv[i], "-N") == 0) {
	// 		core_limit = atoi(argv[i + 1]);
	// 		if (core_limit > num_cores) {
	// 			TRACE_CONFIG("CPU limit should be smaller than the "
	// 					"number of CPUS: %d\n", num_cores);
	// 			return FALSE;
	// 		}
	// 		/** 
	// 		 * it is important that core limit is set 
	// 		 * before mtcp_init() is called. You can
	// 		 * not set core_limit after mtcp_init()
	// 		 */
	// 		mtcp_getconf(&mcfg);
	// 		mcfg.num_cores = core_limit;
	// 		mtcp_setconf(&mcfg);
	// 	} else if (strcmp(argv[i], "-c") == 0) {
	// 		total_concurrency = atoi(argv[i + 1]);


	/* per-core concurrency = total_concurrency / # cores */
	if (total_concurrency > 0)
		concurrency = total_concurrency / core_limit;

	/* set the max number of fds 3x larger than concurrency */
	max_fds = concurrency * 3;

	TRACE_CONFIG("Application configuration:\n");
	TRACE_CONFIG("# of cores: %d\n", core_limit);
	TRACE_CONFIG("Concurrency: %d\n", total_concurrency);

	ret = mtcp_init("epwget.conf");
	if (ret) {
		TRACE_ERROR("Failed to initialize mtcp.\n");
		exit(EXIT_FAILURE);
	}
	mtcp_getconf(&mcfg);
	mcfg.max_concurrency = max_fds;
	mcfg.max_num_buffers = max_fds;
	mtcp_setconf(&mcfg);

	mtcp_register_signal(SIGINT, SignalHandler);

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
	
	start = time ( 0 ); 
	while ( (time ( 0 ) - start) < 30 ) {
	}
	for ( i = 0; i < core_limit; i++ ) {
		done[i] = TRUE;
	}

	for (i = 0; i < core_limit; i++) {
		pthread_join(app_thread[i], NULL);
		TRACE_INFO("Thread %d joined.\n", i);
	}

	mtcp_destroy();
	return 0;
}
/*----------------------------------------------------------------------------*/
