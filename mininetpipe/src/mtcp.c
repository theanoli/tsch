#include "harness.h"

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#define DEBUG 0
#define WARMUP 3
#define COOLDOWN 5

int doing_reset = 0;
int ep = -1;

mctx_t mctx = NULL;

int mtcp_write_helper (mctx_t, int, char *, int);
int mtcp_read_helper (mctx_t, int, char *, int);

int
mtcp_accept_helper (mctx_t mctx, int sockid, struct sockaddr *addr,
                    socklen_t *addrlen)
{
    int nevents, i;
    struct mtcp_epoll_event events[MAXEVENTS];
    struct mtcp_epoll_event event;

    event.events = MTCP_EPOLLIN;
    event.data.sockid = sockid;

    if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = mtcp_epoll_wait (mctx, ep, events, MAXEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("mtcp_epoll_wait");
            }
            exit (1);
        }

        // Wait for an incoming connection; return when we get one
        for (i = 0; i < nevents; i++) {
            if (events[i].data.sockid == sockid) {
                // New connection incoming...
                mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
                return mtcp_accept (mctx, sockid, addr, addrlen);
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
    p->s_ptr = (char *) malloc (PSIZE);
    p->r_ptr = (char *) malloc (PSIZE);
    p->lbuff = (char *) malloc (PSIZE * 2);
    
    if ((p->s_ptr == NULL) || (p->r_ptr == NULL) || (p->lbuff == NULL)) {
        printf ("Malloc error!\n");
        exit (1);
    }

    memset (p->s_ptr, 0, PSIZE);
    memset (p->r_ptr, 0, PSIZE);
    memset (p->lbuff, 0, PSIZE * 2);

    struct mtcp_conf mcfg;
    mtcp_getconf (&mcfg);
    mcfg.num_cores = 1;
    mtcp_setconf (&mcfg);
	mtcp_init ("mtcp.conf");
}


void
Setup (ArgStruct *p)
{
    // Initialize connections: create socket, bind, listen (in establish)
    // Also creates mtcp_epoll instance
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

    if (!mctx) {
        mtcp_core_affinitize (0);
        mctx = mtcp_create_context (0);
        ep = mtcp_epoll_create (mctx, MAXEVENTS);
    }

    if (!mctx) {
        printf ("tester: can't create mTCP socket!");
        exit (-4);
    }

    if ((sockfd = mtcp_socket (mctx, socket_family, MTCP_SOCK_STREAM, 0)) < 0) {
        printf ("tester: can't open stream socket!\n");
        exit (-4);
    }

    mtcp_setsock_nonblock (mctx, sockfd);

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
        // TODO REUSEADDR/REUSEPORT stuff needs to get done here? 
        memset ((char *) lsin1, 0, sizeof (*lsin1));
        lsin1->sin_family       = AF_INET;
        lsin1->sin_addr.s_addr  = htonl (INADDR_ANY);
        lsin1->sin_port         = htons (p->port);

        if (mtcp_bind (mctx, sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            printf ("tester: server: bind on local address failed! errno=%d\n", errno);
            exit (-6);
        }

        p->servicefd = sockfd;
    }

    establish (p);
}


void
SimpleWrite (ArgStruct *p)
{
    // Client-side; send some amount of data to the server,
    // then receive data (and throw it away on function exit).
    // We shouldn't do epoll on the client side, since we only
    // want one connection to the server per client program
    
    char buffer[PSIZE];
    int n;

    snprintf (buffer, PSIZE, "%s", "hello, world!");

    n = mtcp_write (mctx, p->commfd, buffer, PSIZE);
    if (n < 0) {
        perror ("write to server");
        exit (1);
    }

    memset (buffer, 0, PSIZE);

    n = mtcp_read (mctx, p->commfd, buffer, PSIZE);
    if (n < 0) {
        perror ("read from server");
        exit (1);
    }
}


void 
TimestampWrite (ArgStruct *p)
{
    // Send and then receive an echoed timestamp.
    // Return a pointer to the stored timestamp. 

    char pbuffer[PSIZE];  // for packets
    int n;
    struct timespec sendtime, recvtime;

    sendtime = When2 ();
    snprintf (pbuffer, PSIZE, "%lld,%.9ld%-31s",
            (long long) sendtime.tv_sec, sendtime.tv_nsec, ",");

    n = mtcp_write (mctx, p->commfd, pbuffer, PSIZE - 1);
    if (n < 0) {
        perror ("write");
        exit (1);
    }

    memset (pbuffer, 0, PSIZE);

    n = mtcp_read (mctx, p->commfd, pbuffer, PSIZE);
    if (n < 0) {
        perror ("read");
        exit (1);
    }
    recvtime = When2 ();        

    snprintf (p->lbuff, PSIZE, "%s", pbuffer);
    snprintf (p->lbuff + PSIZE - 1, PSIZE, "%lld,%.9ld\n", 
            (long long) recvtime.tv_sec, recvtime.tv_nsec);
    if (DEBUG)
        printf ("Got timestamp: %s\n", p->lbuff);
}


void
Echo (ArgStruct *p)
{
    // Server-side only!
    // Loop through for expduration seconds and count each packet you send out
    // Start counting packets after a few seconds to stabilize connection(s)
    int j, n, i, done;
    struct mtcp_epoll_event events[MAXEVENTS];

    if (p->latency) {
        // We only have one client; add it to the events list
        struct mtcp_epoll_event event;

        event.events = MTCP_EPOLLIN | MTCP_EPOLLET;
        event.data.sockid = p->commfd;

        if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, p->commfd, &event) == -1) {
            perror ("epoll_ctl");
            exit (1);
        }

        if (mtcp_setsock_nonblock (mctx, p->commfd) < 0) {
            printf ("tester: couldn't set socket to nonblocking!\n");
            exit (-6);
        }
    }

    p->counter = 0;

    // Add a two-second delay to let the clients stabilize
    p->program_state = warmup; 
    printf ("Setting the alarm...\n");
    alarm (WARMUP);
    while (p->program_state != end) {
        n = mtcp_epoll_wait (mctx, ep, events, MAXEVENTS, -1);

        if (n < 0) {
            perror ("epoll_wait");
            exit (1);
        }
        
        for (i = 0; i < n; i++) {
            // Check for errors
            if ((events[i].events & MTCP_EPOLLERR) ||
                    (events[i].events & MTCP_EPOLLHUP) ||
                    !(events[i].events & MTCP_EPOLLIN)) {
                printf ("epoll error!\n");
                close (events[i].data.sockid);
                if (p->latency) {
                    exit (1);
                } else {
                    continue;
                }
            } else if (events[i].data.sockid == p->servicefd) {
                // Someone is trying to connect; ignore. All clients should have
                // connected already.
                continue;
            } else {
                // There's data to be read
                done = 0;
                char *q;
                int to_write;
                int written;

                // This is dangerous because p->r_ptr is only PSIZE bytes long
                // TODO figure this out
                q = p->r_ptr;

                while ((j = mtcp_read (mctx, events[i].data.sockid,
                               q, PSIZE - 1)) > 0) {
                    q += j;
                }
                
                if (errno != EAGAIN) {
                    if (j < 0) {
                        perror ("server read");
                    }
                    done = 1;  // Close this socket
                } else {
                    // We've read all the data; echo it back to the client
                    to_write = q - p->r_ptr;
                    written = 0; 
                    
                    while ((j = mtcp_write (mctx, events[i].data.sockid, 
                                    p->r_ptr, q - p->r_ptr) + written) < to_write) {
                        if (j < 0) {
                            if (errno != EAGAIN) {
                                perror ("server write"); 
                                done = 1;
                                break;
                            }
                        }
                        written += j; 
                        
                        // This should happen only extremely rarely
                        printf ("Had to loop...\n");
                    }

                    if ((p->program_state == experiment) && !done) {
                        (p->counter)++;
                    } 
                }

                if (done) {
                    close (events[i].data.sockid);
                }
            }
        }
    }
}


void
ThroughputSetup (ArgStruct *p)
{
    int sockfd; 
    struct sockaddr_in *lsin1, *lsin2;
    char *host;
    struct hostent *addr;
    struct protoent *proto;
    int socket_family = AF_INET;

    if (p->rcv) {
        printf ("*** Setting up connection(s)... ***\n");
    }

    host = p->host;

    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);
    
    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));
    
    if (!mctx) {
        mtcp_core_affinitize (0);
        mctx = mtcp_create_context (0);
        ep = mtcp_epoll_create (mctx, MAXEVENTS);
    }

    if (!mctx) {
        printf ("tester: can't create mTCP socket!");
        exit (-4);
    }

    printf ("\tCreating socket...\n");
    if ((sockfd = mtcp_socket (mctx, socket_family, MTCP_SOCK_STREAM, 0)) < 0) {
        printf ("tester: can't open stream socket!\n");
        exit (-4);
    }

    if (mtcp_setsock_nonblock (mctx, sockfd) < 0) {
        printf ("tester: couldn't set socket to nonblocking!\n");
        exit (-6);
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

        if (mtcp_bind (mctx, sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            printf ("tester: server: bind on local address failed! errno=%d\n", errno);
            exit (-6);
        }

        p->servicefd = sockfd;
    }

    throughput_establish (p);    
}


void
throughput_establish (ArgStruct *p)
{
    // TODO this FD needs to get added to an epoll instance
    socklen_t clen;
    struct protoent *proto;
    int nevents, i;
    struct mtcp_epoll_event events[MAXEVENTS];
    struct mtcp_epoll_event event;
    double t0, duration;
    int connections = 0;

    clen = (socklen_t) sizeof (p->prot.sin2);
    
    if (p->tr) {
        while ((mtcp_connect (mctx, p->commfd, (struct sockaddr *) &(p->prot.sin1), 
                        sizeof (p->prot.sin1)) < 0) && (errno != EINPROGRESS)) {
            if (!doing_reset || errno != ECONNREFUSED) {
                printf ("client: cannot connect to server! errno=%d\n", errno);
                exit (-10);
            }
        }
    } else if (p->rcv) {
        event.events = MTCP_EPOLLIN;
        event.data.sockid = p->servicefd;

        if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, p->servicefd, &event) == -1) {
            perror ("epoll_ctl");
            exit (1);
        }

        mtcp_listen (mctx, p->servicefd, 1024);

        t0 = When ();
        printf ("\tStarting loop to wait for connections...\n");

        while ((duration = (t0 + 10) - When ()) > 0) {
            if (connections == p->ncli)  {
                printf ("OMGLSDJF:LDSKJF:LDSKJF:DLSFJ Got all the connections...\n");
            }

            nevents = mtcp_epoll_wait (mctx, ep, events, MAXEVENTS, duration); 
            if (nevents < 0) {
                if (errno != EINTR) {
                    perror ("epoll_wait");
                }
                exit (1);
            }

            for (i = 0; i < nevents; i++) {
                if (events[i].data.sockid == p->servicefd) {
                    while (1) {
                        char hostbuf[NI_MAXHOST], portbuf[NI_MAXSERV];
                        
                        p->commfd = mtcp_accept (mctx, p->servicefd, 
                                (struct sockaddr *) &(p->prot.sin2), &clen);

                        if (p->commfd == -1) {
                           if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                               break;
                           } else {
                               perror ("accept");
                               break;
                           }
                        }

                        getnameinfo ((struct sockaddr *) &p->prot.sin2, 
                                clen, hostbuf, sizeof (hostbuf),
                                portbuf, sizeof (portbuf),
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                            
                        if (!(proto = getprotobyname ("tcp"))) {
                            printf ("unknown protocol!\n");
                            exit (555);
                        }

                        // Set socket to nonblocking
                        if (mtcp_setsock_nonblock (mctx, p->commfd) < 0) {
                            printf ("Error setting socket to non-blocking!\n");
                            continue;
                        }

                        // Add descriptor to epoll instance
                        event.data.sockid = p->commfd;
                        event.events = MTCP_EPOLLIN | MTCP_EPOLLET;  
                        if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, p->commfd, &event) < 0) {
                            perror ("epoll_ctl");
                            exit (1);
                        }

                        connections++;
                        if (!(connections % 50)) {
                            printf ("%d connections so far...\n", connections);
                        }
                    }
                } else {
                    if (events[i].events & MTCP_EPOLLIN) {
                        int nread;
                        char buf[128];

                        nread = mtcp_read (mctx, events[i].data.sockid, buf, PSIZE);
                        nread = mtcp_write (mctx, events[i].data.sockid, buf, PSIZE);
                        if (nread) {
                        }
                    }
                } 
            }
        }
    }

    printf ("Setup complete... getting ready to start experiment\n");
}


void
establish (ArgStruct *p)
{
    socklen_t clen;
    struct protoent *proto;

    clen = (socklen_t) sizeof (p->prot.sin2);

    if (p->tr) {
        while ((mtcp_connect (mctx, p->commfd, (struct sockaddr *) &(p->prot.sin1), 
                        sizeof (p->prot.sin1)) < 0) && (errno != EINPROGRESS)) {
            if (!doing_reset || errno != ECONNREFUSED) {
                printf ("client: cannot connect to server! errno=%d\n", errno);
                exit (-10);
            }
        }
    } else if (p->rcv) {
        mtcp_listen (mctx, p->servicefd, 1024);
        p->commfd = mtcp_accept_helper (mctx, p->servicefd,     
                                (struct sockaddr *) &(p->prot.sin2), &clen);
        while (p->commfd < 0 && errno == EAGAIN) {
            p->commfd = mtcp_accept_helper (mctx, p->servicefd,     
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
      mtcp_close (mctx, p->commfd);

   } else if ( p->rcv ) {
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


int 
mtcp_read_helper (mctx_t mctx, int sockid, char *buf, int len)
{
    int nevents, i;
    struct mtcp_epoll_event events[MAXEVENTS];
    struct mtcp_epoll_event event;

    event.events = MTCP_EPOLLIN;
    event.data.sockid = sockid;
    if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &event) < 0) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = mtcp_epoll_wait (mctx, ep, events, MAXEVENTS, -1);
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


int
mtcp_write_helper (mctx_t mctx, int sockid, char *buf, int len)
{
    int nevents, i;
    struct mtcp_epoll_event events[MAXEVENTS];
    struct mtcp_epoll_event event;

    event.events = MTCP_EPOLLOUT;
    event.data.sockid = sockid;
    if (mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &event) < 0) {
        perror ("epoll_ctl");
        exit (1);
    }

    while (1) {
        nevents = mtcp_epoll_wait (mctx, ep, events, MAXEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("mtcp_epoll_wait");
            }
            exit (1);
        }

        for (i = 0; i < nevents; ++i) {
            if (events[i].data.sockid == sockid) {
                mtcp_epoll_ctl (mctx, ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
                return mtcp_write (mctx, sockid, buf, len);
            } else {
                printf ("Socket error!\n");
                exit (1);
            }
        }
    }
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
            ((bytesWritten = mtcp_write_helper (mctx, p->commfd, q, bytesLeft)) 
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
        ((bytesRead = mtcp_read_helper (mctx, p->commfd, q, bytesLeft)) != 0)) {
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

