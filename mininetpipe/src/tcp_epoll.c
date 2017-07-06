#include "harness.h"

#include <sys/epoll.h>

#define NEVENTS 10

int doing_reset = 0;
int ep = -1;


int
tcp_accept_helper (int sockid, struct sockaddr *addr, socklen_t *addrlen)
{
    // Accept incoming connections on the listening socket
    int nevents, i;
    struct epoll_event events[NEVENTS];
    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.fd = sockid;

    if (epoll_ctl (ep, EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl");
        exit (1);
    }

    // TODO this part will need to get put into a thread eventually/somehow; this
    // should be a continuous process
    // Also need to be putting the accepted FDs into the watch list
    while (1) {
        nevents = epoll_wait (ep, events, NEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("epoll_wait");
            }
            exit (1);
        }

        for (i = 0; i < nevents; i++) {
            if (events[i].data.fd == sockid) {
                // New connection incoming...
                epoll_ctl (ep, EPOLL_CTL_DEL, sockid, NULL);  // event is ignored
                return accept (sockid, addr, addrlen);
            } else {
                printf ("Socket error!\n");
                exit (1);
            }
        }
    }
}


int
tcp_read_helper (int sockid, char *buf, int len)
{
    int nevents, i;
    struct epoll_event events[NEVENTS];
    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.fd = sockid;
    if (epoll_ctl (ep, EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = epoll_wait (ep, events, NEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("epoll_wait");
            }
            exit (1);
        }

        for (i = 0; i < nevents; i++) {
            if (events[i].data.fd == sockid) {
                epoll_ctl (ep, EPOLL_CTL_DEL, sockid, NULL);
                return read (sockid, buf, len);
            } else {
                printf ("Socket error!\n");
                exit (1);
            }
        }
    }
}
        

int
tcp_write_helper (int sockid, char *buf, int len)
{
    int nevents, i;
    struct epoll_event events[NEVENTS];
    struct epoll_event event;

    event.events = EPOLLOUT;
    event.data.fd = sockid;
    if (epoll_ctl (ep, EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = epoll_wait (ep, events, NEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("epoll_wait");
            }
            exit (1);
        }

        for (i = 0; i < nevents; i++) {
            if (events[i].data.fd == sockid) {
                epoll_ctl (ep, EPOLL_CTL_ADD, sockid, NULL);
                return write (sockid, buf, len);
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
    // Initialize buffers to put incoming/outgoing data
    p->s_ptr = (char *) malloc (PSIZE);
    p->r_ptr = (char *) malloc (PSIZE);

    if ((p->s_ptr == NULL) || (p->r_ptr == NULL)) {
        printf ("Malloc error!\n");
        exit (1);
    }

    memset (p->s_ptr, 0, PSIZE);
    memset (p->r_ptr, 0, PSIZE);
}


void
Setup (ArgStruct *p)
{
    // Initialize connections: create socket, bind, listen (in establish)
    int sockfd;
    struct sockaddr_in *lsin1, *lsin2;
    char *host;
    struct hostent *addr;
    struct protoent *proto;
    int socket_family = AF_INET;

    host = p->host;

    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);

    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));

    if ((sockfd = socket (socket_family, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        printf ("tester: can't open stream socket!\n");
        exit (-4);
    }

    if (!(proto = getprotobyname ("tcp"))) {
        printf ("tester: protocol 'tcp' unknown!\n");
        exit (555);
    }

    if (p->tr) {
        if (atoi (host) > 0) {
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
    } else if (p->rcv) {
        memset ((char *) lsin1, 0, sizeof (*lsin1));
        lsin1->sin_family       = AF_INET;
        lsin1->sin_addr.s_addr  = htonl (INADDR_ANY);
        lsin1->sin_port         = htons (p->port);

        if (bind (sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            printf ("tester: server: bind on local address failed! errno=%d\n", errno);
            exit (-6);
        }

        p->servicefd = sockfd;
    }

    establish (p);
}


void
SendData (ArgStruct *p)
{
    int bytesWritten, bytesLeft;
    char *q;
    struct timespec sendtime;  // this wil get ping-ponged to measure latency

    // We're going to assume here that entire msg gets through in one go
    // But keep this check just in case we need it later
    bytesWritten = 0;

    if (p->tr) {
        // Transmitter (client) should send timestamp
        sendtime = When2 ();
        snprintf (p->s_ptr, PSIZE, "%lld,%.9ld%-31s", 
                (long long) sendtime.tv_sec, sendtime.tv_nsec, ",");
        q = p->s_ptr;
    } else {
        // Receiver (server) should just echo back whatever it got
        q = p->r_ptr;
    }    
    p->bufflen = strlen (q);     
    bytesLeft = p->bufflen;

    while ((bytesLeft > 0) &&
            ((bytesWritten = tcp_write_helper (p->commfd, q, bytesLeft)) 
            != 0)) {
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


char *
RecvData (ArgStruct *p)
{
    int bytesLeft;
    int bytesRead;
    char *q;
    char *buf;

    bytesRead = 0;
    q = p->r_ptr;
    bytesLeft = PSIZE - 1;

    while ((bytesLeft > 0) &&
        ((bytesRead = tcp_read_helper (p->commfd, q, bytesLeft)) != 0)) {
        if (bytesRead < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                break;
            }
        }
        
        bytesLeft -= bytesRead;
        q += bytesRead;
    }

    if ((bytesLeft > 0) && (bytesRead == 0)) {
        printf ("tester: EOF encountered on reading from socket\n");
    } else if (bytesRead == -1) {
        printf ("tester: read: error encountered, errno=%d\n", errno);
        exit (401);
    }
    
    if (p->tr) {
        struct timespec recvtime = When2 ();        
        buf = malloc (PSIZE * 2);
        if (buf == NULL) {
            printf ("Malloc error!\n");
            exit (1);
        }

        snprintf (buf, PSIZE, "%s", p->r_ptr);
        snprintf (buf + PSIZE - 1, PSIZE, "%lld,%.9ld\n", 
                (long long) recvtime.tv_sec, recvtime.tv_nsec);
        return buf;
    } else {
        return NULL;
    }
}


void
establish (ArgStruct *p)
{
    socklen_t clen;
    struct protoent *proto;

    clen = (socklen_t) sizeof (p->prot.sin2);

    if (p->tr) {
        while ((connect (p->commfd, (struct sockaddr *) &(p->prot.sin1), 
                        sizeof (p->prot.sin1)) < 0) && (errno != EINPROGRESS)) {
            if (!doing_reset || errno != ECONNREFUSED) {
                printf ("client: cannot connect to server! errno=%d\n", errno);
                exit (-10);
            }
        }
    } else if (p->rcv) {
        listen (p->servicefd, 5);
        p->commfd = tcp_accept_helper (p->servicefd,     
                                (struct sockaddr *) &(p->prot.sin2), &clen);
        while (p->commfd < 0 && errno == EAGAIN) {
            p->commfd = tcp_accept_helper (p->servicefd,     
                                    (struct sockaddr *) &(p->prot.sin2), &clen);
        }

        if (p->commfd < 0) {
            printf ("Server: accept failed! errno=%d\n", errno);
            perror ("accept");
            exit (-12);
        }

        if (!(proto = getprotobyname ("tcp"))) {
            printf ("unknown protocol!\n");
            exit (555);
        }
    }
}


void
CleanUp (ArgStruct *p)
{
   if (p->tr) {
      close (p->commfd);

   } else if ( p->rcv ) {
      close (p->commfd);
      close (p->servicefd);
      close (ep);
   }
}


void
Reset(ArgStruct *p)
{
    /* Reset sockets */
    if (p->reset_conn) {

        doing_reset = 1;
    
        /* Close the sockets */
        CleanUp (p);
    
        /* Now open and connect new sockets */
        Setup (p);
    }

}


/* Dummy functions -----------------------------------------------------------*/
// May need filling in later

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
AfterAlignmentInit (ArgStruct *p)
{

}

