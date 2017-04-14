#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#define MAX_EVENTS 4096
#define MAX_CPUS 16
#define PSIZE 32
#define BUFSIZE 64
#define PORT 8000

#define TRUE (1)
#define FALSE (0)

#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))

int setsock_nonblock (int);
/*----------------------------------------------------------------------------*/
struct thread_context
{
	int ep;
};
/*----------------------------------------------------------------------------*/
int core_limit;
static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
/*----------------------------------------------------------------------------*/
void
CloseConnection (struct thread_context *ctx, int sockfd) {
	epoll_ctl (ctx->ep, EPOLL_CTL_DEL, sockfd, NULL);
	close (sockfd);
}
/*----------------------------------------------------------------------------*/
static int
HandleReadEvent (struct thread_context *ctx, int sockid)
{
	// Handles incoming packets by echoing contents back to sender
	struct epoll_event ev;
	char buf[PSIZE] = {0};
	int ret;

	ret = read (sockid, buf, PSIZE);
	if ((ret < 0) && (ret != -EAGAIN)) {
		return ret;
	}

	ret = write (sockid, buf, PSIZE);
	if ((ret < 0) && (ret != -EAGAIN)) {
		return ret;
	}

	ev.events = EPOLLIN;
	ev.data.fd = sockid;
	epoll_ctl (ctx->ep, EPOLL_CTL_MOD, sockid, &ev);
	return ret;
}
/*----------------------------------------------------------------------------*/
int
AcceptConnection (struct thread_context *ctx, int listener)
{
	struct epoll_event ev;
	struct sockaddr in_addr;
	socklen_t in_len;
	int infd;
	int ret;

	in_len = sizeof (in_addr);
	infd = accept (listener, &in_addr, &in_len);
	if (infd < 0) {
		if ((errno == EAGAIN) || 
			(errno == EWOULDBLOCK)) {
			// All incoming connections processed
			return infd;
		} else {
			perror ("accept");
			return -1;
		}
	}

	printf ("New connection %d accepted\n", infd);
	ret = setsock_nonblock (infd);
	if (ret < 0) {
		perror ("epoll_ctl");
		exit (1);
	}

	ev.data.fd = infd;
	ev.events = EPOLLIN;
	ret = epoll_ctl (ctx->ep, EPOLL_CTL_ADD, infd, &ev);
	
	return infd;
}
/*----------------------------------------------------------------------------*/
struct thread_context *
InitializeServerThread (int core) {
	struct thread_context *ctx;

	ctx = (struct thread_context *) calloc (1, sizeof (struct thread_context));

	// Create epoll descriptor
	ctx->ep = epoll_create1 (0);
	if (ctx->ep < 0) {
		perror ("epoll_create1");
		return NULL;
	}

	return ctx;
}
/*----------------------------------------------------------------------------*/
int
create_and_bind_socket (void)
{
	int listener;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int ret;

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo (NULL, "8000", &hints, &result);
	if (ret != 0) {
		perror ("getaddrinfo: failed to get sockets");
		return -1;	
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		listener = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (listener == -1) {
			continue;
		}

		ret = bind (listener, rp->ai_addr, rp->ai_addrlen);
		if (ret == 0) {
			// Bind was successful
			break;
		}

		close (listener);
	}

	if (rp == NULL) {
		perror ("couldn't bind");
		return -1;
	}
	
	freeaddrinfo (result);
	return listener;
}

int
setsock_nonblock (int sockfd)
{
	// TODO why do we need to do this? 
	int flags, s;
	
	flags = fcntl (sockfd, F_GETFL, 0);
	if (flags < 0) {
		perror ("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl (sockfd, F_SETFL, flags);
	if (s < 0) {
		perror ("fcntl");
		return -1; 
	}
	
	return 0;
}

int CreateListeningSocket (struct thread_context *ctx) 
{
	int listener; 
	struct epoll_event ev;
	int ret;

	/* Create socket, set as non-blocking */
	listener = create_and_bind_socket ();
	if (listener < 0) {
		perror ("create_and_bind");
		return -1;
	}

	ret = setsock_nonblock (listener);
	if (ret < 0) {
		perror ("setsock_nonblock");
		return -1;
	}

	ret = listen (listener, 4096);
	if (ret < 0) {
		perror ("listen");
		return -1;
	}

	ev.events = EPOLLIN;
	ev.data.fd = listener;
	epoll_ctl (ctx->ep, EPOLL_CTL_ADD, listener, &ev);

	return listener;
}
/*----------------------------------------------------------------------------*/
void *
RunServerThread (void *arg)
{
	int core = *(int *) arg;
	struct thread_context *ctx;
	int listener;
	int ep; 
	struct epoll_event *events;
	int nevents;
	int i, ret;
	int do_accept;

	ctx = InitializeServerThread (core);
	if (!ctx) {
		perror ("Failed to initialize server thread");
		exit (-1);
	}	
	ep = ctx->ep;
	events = (struct epoll_event *) calloc (MAX_EVENTS, 
						sizeof (struct epoll_event));
	if (!events) {
		perror ("calloc event structs");
		exit (-1);
	}

	listener = CreateListeningSocket (ctx);
	if (listener < 0) {
		perror ("failed to create listening socket");
		exit (-1);
	}

	while (!done[core]) {
		nevents = epoll_wait (ep, events, MAX_EVENTS, -1);
		if (nevents < 0) {
			if (errno != EINTR) {
				perror ("epoll_wait");
			}
			break;
		}

		do_accept = FALSE;
		for (i = 0; i < nevents; i++) {
			if (events[i].data.fd == listener) {
				// Someone is trying to connect
				do_accept = TRUE;
			} else if (events[i].events & EPOLLERR) {
				// There was a connection error
				int err; 
				socklen_t len = sizeof (err);
	
				// Error on this connection	
				printf ("Error on socket %d\n", core);
				// Maybe do better echecking here but don't care now
				CloseConnection (ctx, events[i].data.fd);
			} else if (events[i].events & EPOLLIN) {
				// There's data waiting to be read
				ret = HandleReadEvent (ctx, events[i].data.fd);
				
				if (ret == 0) {
					// Connection closed from other end
					CloseConnection (ctx, events[i].data.fd);
				} else if (ret < 0) {
					// If not EAGAIN, it's an actual error
					if (errno != EAGAIN) {
						CloseConnection (ctx, 
							events[i].data.fd);
					}
				}
			} else if (events[i].events & EPOLLOUT) {
				continue;
			} else {
				// Anything else is an error and we should exit
				printf ("Unknown event");
				exit (1);
			}	
		}
		if (do_accept) {
			// Accept all the waiting connections now
			while (1) {
				ret = AcceptConnection (ctx, listener);
				if (ret < 0) {
					break;
				}
			}
		}
	}

	pthread_exit (NULL);
	return NULL;
}
/*----------------------------------------------------------------------------*/
void
SignalHandler (int signum)
{
	int i;

	for (i = 0; i < core_limit; i++) {
		if (app_thread[i] == pthread_self ()) {
			done[i] = TRUE;
		} else {
			if (!done[i]) {
				pthread_kill (app_thread[i], signum);
			}
		}
	}
}
/*----------------------------------------------------------------------------*/
static void
printHelp (const char *prog_name) 
{
	printf ("%s USAGE HERE\n", prog_name);
	exit (0);
}
/*----------------------------------------------------------------------------*/
int
main (int argc, char **argv)
{
	int ret;
	int cores[MAX_CPUS];  // for now only 1 core
	int i, opt;

	while (-1 != (opt = getopt (argc, argv, "N:h"))) {
		switch (opt) {
		case 'N':
			core_limit = 1;  // TODO fix this later
			break;
		case 'h': 
			printHelp (argv[0]);
			break;
		}	
	}

	// Register the signal handler
	if (signal (SIGINT, SignalHandler) == SIG_IGN) {
		signal (SIGINT, SIG_IGN);
	}

	for (i = 0; i < core_limit; i++) {
		cores[i] = i;
		done[i] = FALSE;

		if (pthread_create (&app_thread[i], NULL, RunServerThread,
					(void *)&cores[i])) {
			perror ("pthread_create");
			exit (-1);
		}
	}

	for (i = 0; i < core_limit; i++) {
		pthread_join (app_thread[i], NULL);
	}

	return 0;
}
