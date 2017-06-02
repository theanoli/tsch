#include "harness.h"

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#define NEVENTS 10

int doing_reset = 0;
mctx_t mctx = NULL;
int ep = -1;


int
mtcp_accept_helper (mctx_t mctx, int sockid, struct sockaddr *addr,
                    socklen_t *addrlen)
{
    int nevents, i;
    struct mtcp_epoll_event events[NEVENTS];
    struct mtcp_epoll_event evctl;

    evctl.events = MTCP_EPOLLIN;
    evctl.data.sockid = sockid;

    if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &evctl) < 0) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = mtcp_epoll_wait (mctx, ep, events, NEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("mtcp_epoll_wait");
            }
            exit (1);
        }

        // Wait for an incoming connection; return when we get one
        for (i = 0; i < nevents; ++i) {
            if (events[i].data.sockid == sockid) {
                mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
                return mtcp_accept (mctx, sockid, addr, addrlen);
            } else {
                printf ("Socket error!\n");
                exit (1);
            }
        }
    }
}


int 
mtcp_read_helper (mctx_t mctx, int sockid, char *buf, int len)
{
    int nevents, i;
    struct mtcp_epoll_event events[NEVENTS];
    struct mtcp_epoll_event evctl;

    evctl.events = MTCP_EPOLLIN;
    evctl.data.sockid = sockid;
    if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &evctl) < 0) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = mtcp_epoll_wait (mctx, ep, events, NEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("mtcp_epoll_wait");
            }
            exit (1);
        }

        for (i = 0; i < nevents; ++i) {
            if (events[i].data.sockid == sockid) {
                mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
                return mtcp_read (mctx, sockid, buf, len);
            } else {
                printf ("Socket error!\n");
                exit (1);
            }
        }
    }
}


void
Init (ArgStruct *p, int *pargc, char ***pargv)
{
	// Set protocol-specific variables
	p->tr = 0;
	p->rcv = 1;
	mtcp_int ("mtcp.conf");
}


void
Setup (ArgStruct *p)
{
	// Initialize connections
	int sockfd;
    struct sockaddr_in *lsin1, *lsin2;
    char *host;
    struct hostent *addr;
    struct protoent *proto;
    // int send_size, recv_size;
    int socket_family = AF_INET;

    host = p->host;

    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);

    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));

    if (!mctx) {  // why?
        mtcp_core_affinitize (mtcp_get_hyperthread_sibling (0));
        mctx = mtcp_create_context (0);
        ep = mtcp_epoll_create (mctx, NEVENTS);
    }

    if (!mctx) {  // Uh oh something is broken
        printf ("tester: can't create mTCP socket!");
        exit (-4);
    }

    if ((sockfd = mtcp_socket (mctx, socket_family, MTCP_SOCK_STREAM, 0)) < 0) {
        printf ("tester: can't open stream socket!");
        exit (-4);
    }

    mtcp_setsock_nonblock (mctx, sockfd); 

    if (!(proto = getprotobyname ("tcp"))) {
        printf ("tester: protocol 'tcp' unknown!\n");
        exit (555);
    }

    if (p->tr) {    // This is the transmitter
        if (atoi (host) > 0) {
            // Got an IP address
            lsin1->sin_family = AF_INET;
            lsin1->sin_addr.s_addr = inet_addr (host);
        } else {
            if ((addr = gethostbyname (host)) == NULL) {
                printf ("tester: invalid hostname '%s'\n", host);
                exit (-5);
            }
        
            lsin1->sin_family = addr->h_addrtype;
            memcpy (addr->h_addr, (char *) &(lsin1->sin_addr.s_addr), addr->h_length);
        }
        
        lsin1->sin_port = htons (p->port);
        p->commfd = sockfd;

    } else if (p->rcv) {    // This is the receiver
        memset ((char *) lsin1, 0, sizeof (*lsin1));

        lsin1->sin_family       = AF_INET;
        lsin1->sin_addr.s_addr  = htonl (INADDR_ANY);
        lsin1->sin_port         = htons (p->port);

        if (mtcp_bind (mctx, sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            printf ("tester: server: bind on local address failed! errno=%d", errno);
            exit (-6);
        }

        p->servicefd = sockfd;
    }

    // TODO do we need these?
    // p->upper = send_size + recv_size;

    establish (p);
}


static int
readFully (int fd, void *obuf, int len)
{
    // TODO fill this in if needed
    return 0;
}


void
Sync (ArgStruct *p)
{
    // TODO fill in if needed
}


void 
PrepareToReceive (ArgStruct *p)
{
    // Not needed
}


void
SendData (ArgStruct *p)
{
    int bytesWritten, bytesLeft;
    char *q;

    // We're going to assume here that entire msg gets through in one go
    // But keep this check just in case we need it later
    bytesLeft = p->bufflen;
    bytesWritten = 0;
    q = p->s_ptr;
   
    while ((bytesLeft > 0) &&
            (bytesWritten = mtcp_write_helper (mctx, p->commfd, q, bytesLeft)) != 0) {
        if (bytesWritten < 0) {
            if (errno == EAGAIN) {
                continue;      // not an actual error
            } else {
                break;
            }
        }

        bytesLeft -= bytesWritten;
        q += bytesWritten;
    }

    if (bytesWritten < 0) {
        printf ("tester: write: error encountered, errno=%d\n", errno);
        exit (401);
    }
}


void
RecvData (ArgStruct *p)
{
    int bytesLeft;
    int bytesRead;
    char *q;

    bytesLeft = p->bufflen;
    bytesRead = 0;
    q = p->r_ptr;

    while ((bytesLeft > 0) &&
        (bytesRead = mtcp_read_helper (mctx, p->commfd, q, bytesLeft)) != 0) {
        if (bytesRead < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                break;
            }

            bytesLeft -= bytesRead;
            q += bytesRead;
        }
    }

    if ((bytesLeft > 0) && (bytesRead == 0)) {
        printf ("tester: EOF encountered on reading from socket\n");
    } else if (bytesRead == -1) {
        printf ("tester: read: error encountered, errno=%d\n", errno);
        exit (401);
    }
}


void
SendTime (ArgStruct *p, double *t)
{
    // TODO fill in if needed
}


void
RecvTime (ArgStruct *p, double *t)
{
    // TODO fill in if needed
}


void
SendRepeat (ArgStruct *p, int rpt)
{
    // TODO fill in if needed
}


void 
RecvRepeat (ArgStruct *p, int *rpt)
{
    // TODO fill in if needed
}

void
establish (ArgStruct *p)
{
    int one = 1;
    socklen_t clen;
    struct protoent *proto;
    
    clen = (socklen_t) sizeof (p->prot.sin2);
    
    if (p->tr ){
    
        while ((mtcp_connect (mctx, p->commfd, (struct sockaddr *) &(p->prot.sin1),
                     sizeof (p->prot.sin1)) < 0) && (errno != EINPROGRESS)) {
    
            /* If we are doing a reset and we get a connection refused from
            * the connect() call, assume that the other node has not yet
            * gotten to its corresponding accept() call and keep trying until
            * we have success.
            */
            if (!doing_reset || errno != ECONNREFUSED) {
                printf("Client: Cannot Connect! errno=%d\n",errno);
                exit(-10);
            } 
          
        }
    
    } else if (p->rcv ) {
    
        /* SERVER */
        mtcp_listen (mctx, p->servicefd, 5);
        p->commfd = mtcp_accept_helper (mctx, p->servicefd, 
                                        (struct sockaddr *) &(p->prot.sin2), &clen);
        while (p->commfd < 0 && errno == EAGAIN) {
            p->commfd = mtcp_accept_helper (mctx, p->servicefd, 
                                        (struct sockaddr *) &(p->prot.sin2), &clen);
        }
        
        if (p->commfd < 0){
            printf ("Server: Accept Failed! errno=%d\n", errno);
            perror ("accept");
            exit (-12);
        }
        
        if (!(proto = getprotobyname ("tcp"))){
            printf ("unknown protocol!\n");
            exit (555);
        }
    }
}


void
CleanUp (ArgStruct *p)
{
   char *quit = "QUIT";

   if (p->tr) {
      mtcp_write_helper (mctx, p->commfd, quit, 5);
      mtcp_read_helper (mctx, p->commfd, quit, 5);
      mtcp_close (mctx, p->commfd);

   } else if ( p->rcv ) {
      mtcp_read_helper (mctx, p->commfd, quit, 5);
      mtcp_write_helper (mctx, p->commfd, quit,5);
      mtcp_close (mctx, p->commfd);
      mtcp_close (mctx, p->servicefd);

   }
}


void
Reset(ArgStruct *p)
{
  
  /* Reset sockets */
  if (p->reset_conn) {

    doing_reset = 1;

    /* Close the sockets */
    CleanUp2(p);

    /* Now open and connect new sockets */
    Setup(p);
  }

}

void
AfterAlignmentInit(ArgStruct *p)
{

}

