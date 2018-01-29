#include "harness.h"

#include <sys/epoll.h>

#define DEBUG 0
#define WARMUP 3
#define COOLDOWN 5

int doing_reset = 0;
int ep = -1;


int
tcp_accept_helper (int sockid, struct sockaddr *addr, socklen_t *addrlen)
{
    // Accept incoming connections on the listening socket
    int nevents, i;
    struct epoll_event events[MAXEVENTS];
    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.fd = sockid;

    if (epoll_ctl (ep, EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl 1");
        exit (1);
    }

    while (1) {
        nevents = epoll_wait (ep, events, MAXEVENTS, -1);
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


void
Init (ArgStruct *p, int *pargc, char ***pargv)
{
    // Initialize buffers to put incoming/outgoing data
    p->s_ptr = (char *) malloc (PSIZE);
    p->r_ptr = (char *) malloc (PSIZE);
    p->lbuff = (char *) malloc (PSIZE * 2);

    if ((p->s_ptr == NULL) || (p->r_ptr == NULL) 
            || (p->lbuff == NULL)) {
        printf ("Malloc error!\n");
        exit (1);
    }

    memset (p->s_ptr, 0, PSIZE);
    memset (p->r_ptr, 0, PSIZE);
    memset (p->lbuff, 0, PSIZE * 2);
}


void
Setup (ArgStruct *p)
{
    // Initialize connections: create socket, bind, listen (in establish)
    // Also creates epoll instance
    int sockfd;
    struct sockaddr_in *lsin1, *lsin2;
    char *host;
    struct hostent *addr;
    struct protoent *proto;
    int socket_family = AF_INET;
    int flags;

    host = p->host;

    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);

    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));

    ep = epoll_create (MAXEVENTS);

    flags = SOCK_STREAM;
    if (p->rcv) {
        flags |= SOCK_NONBLOCK;
    }
    if ((sockfd = socket (socket_family, flags, 0)) < 0) {
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
        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            printf ("tester: server: SO_REUSEADDR failed! errno=%d\n", errno);
            exit (-7);
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
            printf ("tester: server: SO_REUSEPORT failed! errno=%d\n", errno);
            exit (-7);
        }

        memset ((char *) lsin1, 0, sizeof (*lsin1));
        lsin1->sin_family       = AF_INET;
        lsin1->sin_addr.s_addr  = htonl (INADDR_ANY);
        lsin1->sin_port         = htons (p->port);

        if (bind (sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            printf("%s bind to %s:%d failed: %s", __func__, inet_ntoa(lsin1->sin_addr), ntohs(lsin1->sin_port), strerror(errno));
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

    n = write (p->commfd, buffer, PSIZE);
    if (n < 0) {
        perror ("write to server");
        exit (1);
    }

    memset (buffer, 0, PSIZE);

    n = read (p->commfd, buffer, PSIZE);
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

    n = write (p->commfd, pbuffer, PSIZE - 1);
    if (n < 0) {
        perror ("write");
        exit (1);
    }

    if (DEBUG)
        printf ("pbuffer: %s, %d bytes written\n", pbuffer, n);

    memset (pbuffer, 0, PSIZE);
    
    n = read (p->commfd, pbuffer, PSIZE);
    if (n < 0) {
        perror ("read");
        exit (1);
    }
    recvtime = When2 ();        

    snprintf (p->lbuff, PSIZE, "%s", pbuffer);
    snprintf (p->lbuff + PSIZE - 1, PSIZE, "%lld,%.9ld\n", 
            (long long) recvtime.tv_sec, recvtime.tv_nsec);
    if (DEBUG)
        printf ("Got timestamp: %s, %d bytes read\n", pbuffer, n);
}


void
Echo (ArgStruct *p)
{
    // Server-side only!
    // Loop through for expduration seconds and count each packet you send out
    // Start counting packets after a few seconds to stabilize connection(s)
    int j, n, i, done;
    struct epoll_event events[MAXEVENTS];

    if (p->latency) {
        // We only have one client; add it to the events list
        struct epoll_event event;

        if (setsock_nonblock (p->commfd) < 0) {
            printf ("Error setting socket to non-blocking!\n");
            exit (1);
        }
        
        event.events = EPOLLIN;
        event.data.fd = p->commfd;

        if (epoll_ctl (ep, EPOLL_CTL_ADD, p->commfd, &event) == -1) {
            perror ("epoll_ctl");
            exit (1);
        }
    }

    p->counter = 0;

    // Add a few-second delay to let the clients stabilize
    printf ("Setting the alarm...\n");
    alarm (WARMUP);
    while (p->tput_done == 0) {
        n = epoll_wait (ep, events, MAXEVENTS, -1);

        if (n < 0) {
            if (n == EINTR) {
                perror ("epoll_wait: echo intr:");
            }
            // exit (1);
        }
        
        if (DEBUG)
            printf ("Got %d events\n", n);

        for (i = 0; i < n; i++) {
            // Check for errors
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    !(events[i].events & EPOLLIN)) {
                printf ("epoll error!\n");
                close (events[i].data.fd);
                if (p->latency) {
                    exit (1);
                } else {
                    continue;
                }
            } else if (events[i].data.fd == p->servicefd) {
                // Someone is trying to connect; ignore. All clients should have
                // connected already.
                continue;
            } else {
                // There's data to be read
                done = 0;
                int to_write;
                int written;
                char *q;

                // This is dangerous because p->r_ptr is only PSIZE bytes long
                // TODO figure this out
                q = p->r_ptr;

                while ((j = read (events[i].data.fd, q, PSIZE - 1)) > 0) {
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
                    
                    while ((j = write (events[i].data.fd, 
                                    p->r_ptr + written, to_write) + written) < to_write) {
                        if (j < 0) {
                            if (errno != EAGAIN) {
                                perror ("server write");
                                done = 1;
                                break;
                            }
                        }
                        written += j;
                        printf ("Had to loop...\n");
                    }

                    if (p->docount && !done) {
                        (p->counter)++;
                    } 
                }

                if (done) {
                    printf ("Got an error!\n");
                    close (events[i].data.fd);
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
    int flags;

    if (p->rcv) {
        printf ("*** Setting up connection(s)... ***\n");
    }

    host = p->host;

    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);
    
    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));
    
    ep = epoll_create (MAXEVENTS);

    flags = SOCK_STREAM;
    if (p->rcv) {
        flags |= SOCK_NONBLOCK;
    } 

    if ((sockfd = socket (socket_family, flags, 0)) < 0) {
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
        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            printf ("tester: server: SO_REUSEADDR failed! errno=%d\n", errno);
            exit (-7);
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
            printf ("tester: server: SO_REUSEPORT failed! errno=%d\n", errno);
            exit (-7);
        }

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

    throughput_establish (p);    
}


void
throughput_establish (ArgStruct *p)
{
    // TODO this FD needs to get added to an epoll instance
    socklen_t clen;
    struct protoent *proto;
    int nevents, i;
    struct epoll_event events[MAXEVENTS];
    struct epoll_event event;
    double t0, duration;
    int connections = 0;

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
        event.events = EPOLLIN;
        event.data.fd = p->servicefd;

        if (epoll_ctl (ep, EPOLL_CTL_ADD, p->servicefd, &event) == -1) {
            perror ("epoll_ctl");
            exit (1);
        }

        listen (p->servicefd, 1024);

        t0 = When ();
        printf ("\tStarting loop to wait for connections...\n");

        while ((duration = (t0 + (p->online_wait + 10)) - When ()) > 0) {
            if (connections == p->ncli)  {
                printf ("OMGLSDJF:LDSKJF:LDSKJF:DLSFJ Got all the connections...\n");
            }

            nevents = epoll_wait (ep, events, MAXEVENTS, duration); 
            if (nevents < 0) {
                if (errno != EINTR) {
                    perror ("epoll_wait");
                }
                exit (1);
            }

            for (i = 0; i < nevents; i++) {
                if (events[i].data.fd == p->servicefd) {
                    while (1) {
                        char hostbuf[NI_MAXHOST], portbuf[NI_MAXSERV];
                        
                        p->commfd = accept (p->servicefd, 
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
                        if (setsock_nonblock (p->commfd) < 0) {
                            printf ("Error setting socket to non-blocking!\n");
                            continue;
                        }

                        // Add descriptor to epoll instance
                        event.data.fd = p->commfd;
                        event.events = EPOLLIN;  // TODO check this

                        if (epoll_ctl (ep, EPOLL_CTL_ADD, p->commfd, &event) < 0) {
                            perror ("epoll_ctl");
                            exit (1);
                        }

                        connections++;
                        if (!(connections % 50)) {
                            printf ("%d connections so far...\n", connections);
                        }
                    }
                } 
            }
        }

        // Record the actual number of successful connections
        p->ncli = connections;
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
        while ((connect (p->commfd, (struct sockaddr *) &(p->prot.sin1), 
                        sizeof (p->prot.sin1)) < 0) && (errno != EINPROGRESS)) {
            if (!doing_reset || errno != ECONNREFUSED) {
                printf ("client: cannot connect to server! errno=%d\n", errno);
                exit (-10);
            }
        }
    } else if (p->rcv) {
        listen (p->servicefd, 1024);
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

   } else if (p->rcv) {
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


int
setsock_nonblock (int fd)
{
    int flags;

    flags = fcntl (fd, F_GETFL, 0);
    if (flags == -1) {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl (fd, F_SETFL, flags) == -1) {
        perror ("fcntl");
        return -1;
    }

    return 0;
}


/* Obsolete functions? */
int
tcp_read_helper (int sockid, char *buf, int len)
{
    int nevents, i;
    struct epoll_event events[MAXEVENTS];
    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.fd = sockid;
    if (epoll_ctl (ep, EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl 2");
        exit (1);
    }

    while (1) {
        nevents = epoll_wait (ep, events, MAXEVENTS, -1);
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
    struct epoll_event events[MAXEVENTS];
    struct epoll_event event;

    event.events = EPOLLOUT;
    event.data.fd = sockid;
    if (epoll_ctl (ep, EPOLL_CTL_ADD, sockid, &event) == -1) {
        perror ("epoll_ctl 3");
        exit (1);
    }

    while (1) {
        nevents = epoll_wait (ep, events, MAXEVENTS, -1);
        if (nevents < 0) {
            if (errno != EINTR) {
                perror ("epoll_wait");
            }
            exit (1);
        }

        for (i = 0; i < nevents; i++) {
            if (events[i].data.fd == sockid) {
                epoll_ctl (ep, EPOLL_CTL_DEL, sockid, NULL);
                return write (sockid, buf, len);
            } else {
                printf ("Socket error!\n");
                exit (1);
            }
        }
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

