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

#define PSIZE 32
#define PORTNO 8000
#define MAX_CPUS 16

static pthread_t app_thread[MAX_CPUS];
static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;

struct thread_context 
{
	int idx;
	int ep;
} 
typedef struct thread_context *thread_context_t;

struct timespec diff (struct timespec start, struct timespec end);

thread_context_t
CreateContext (int idx) 
{
	thread_context_t ctx;

	ctx = (thread_context_t) calloc (1, sizeof (struct thread_context));
	if (!ctx) {
		perror ("malloc");
		return NULL;
	}

	ctx->idx = i;
	return ctx;
}

void
DestroyContext (thread_context_t ctx) 
{
	free (ctx);
}

static int
make_socket_non_blocking (int sockid) 
{
	int flags, s;
	
	flags = fcntl (sockid, F_GETFL, 0);
	if (flags < 0) {
		perror ("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl (sockid, F_SETFL, flags);
	if (flags < 0) {
		perror ("fcntl");
		return -1;
	}

	return 0;
}

static inline int
CreateConnection (thread_context_t ctx)
{
	struct epoll_event ev;
	struct sockaddr_in addr;
	int sockid; 
	int ret;

	sockid = socket (AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		return -1;
	}

	ret = make_socket_non_blocking (sockid);
	if (ret < 0) {
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = daddr;
	addr.sin_port = dport;

	ret = connect (sockid, (struct sockaddr *)&addr, sizeof (struct sockaddr_in));
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror ("mtcp_connect");
			return -1;
		}
	}

	// We only want to know about send opportunities
	ev.events = EPOLLOUT;
	ev.data.sockid = sockid;
	ret = epoll_ctl (ctx->ep, EPOLL_CTL_ADD, sockid, &ev);

	return sockid;	
}

static inline void
CloseConnection (thread_context_t ctx, int sockid)
{
	epoll_ctl (ctx->ep, EPOLL_CTL_DEL, sockid, NULL);
	close (sockid);
}

struct timespec
timestamp (void)
{
	struct timespec spec;
	
	clock_gettime (CLOCK_MONOTONIC, &spec);
	return spec;
}

void ReceivePacket (thread_context_t ctx, int sockid)
{
	int rd;
	char *str;
	char *secs;
	char *ns;
	char *rtt;
	struct timespec parsed_start, end, tdiff;
	struct epoll_event ev;

	str = malloc (PSIZE * sizeof (char));
	rtt = malloc (PSIZE * 2 * sizeof (char));
	if ((str == NULL) ||
		(rtt == NULL)) {
		perror ("Malloc error");
		return;
	}

	memset (str, '\0', PSIZE);
	memset (rtt, '\0', PSIZE);
	
	rd = read (sockid, str, PSIZE);
	if (rd < 0) {
		if (errno != EAGAIN) {
			CloseConnection (ctx, sockid);
		} else {
			// We've read everything on here, return
			return;
		}
}



