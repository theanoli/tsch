#include "harness.h"

#define DEBUG 0
#define WARMUP 3
#define COOLDOWN 5

int doing_reset = 0;

void
Init (ArgStruct *p, int *pargc, char ***pargv)
{
    p->reset_conn = 0;
    
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
    int sockfd;
    struct sockaddr_in *lsin1, *lsin2;
    char *host;

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int socket_family = AF_INET;
    int s;
    char portno[7];

    struct protoent *proto;
    int flags; 

    host = p->host;
    sprintf (portno, "%d", p->port);
    flags = SOCK_DGRAM;

    // To resolve a hostname
    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family     = socket_family;
    hints.ai_socktype   = flags;

    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);

    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));

    if (!(proto = getprotobyname ("udp"))) {
        printf ("tester: protocol 'udp' unknown!\n");
        exit (555);
    }

    if (p->tr) {
        s = getaddrinfo (host, portno, &hints, &result);
        if (s != 0) {
            perror ("getaddrinfo");
            exit (-10);
        }

        for (rp = result; rp != NULL; rp = rp->ai_next) {
            sockfd = socket (rp->ai_family, rp->ai_socktype, 
                    rp->ai_protocol);
            if (sockfd == -1) {
                continue;
            }

            // This means we can use read/write or send/recv instead of
            // sendto/recvfrom
            if (connect (sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                break;
            }
            
            close (sockfd);
        }

        if (rp == NULL) {
            printf ("Invalid address %s and/or portno %s! Exiting...\n", 
                    host, portno);
            exit (-10);
        }

        lsin1->sin_port = htons (p->port);
        p->commfd = sockfd;
        freeaddrinfo (result);

    } else if (p->rcv) {
        // Receiver side
        if ((sockfd = socket (socket_family, flags, 0)) < 0) {
            printf ("tester: can't open stream socket!\n");
            exit (-4);
        }

        memset ((char *) lsin1, 0, sizeof (*lsin1));
        lsin1->sin_family       = AF_INET;
        lsin1->sin_addr.s_addr  = htonl (INADDR_ANY);
        lsin1->sin_port         = htons (p->port);

        if (bind (sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            perror ("tester: server: bind on local address failed!"
                    "errno");
            exit (-6);
        }

        p->servicefd = sockfd;
        printf ("Bound to socket %d...\n", sockfd);

    }

    printf("CCH: p->packets: %d\n", p->packet_rate);

    establish (p);
}


void *
RxThread (void *vargp)
{
    ArgStruct *p = (ArgStruct *)vargp;
    int n;

    while (1) {
        n = read (p->commfd, p->rbuff, PSIZE);

        if (n < 0) {
           perror ("read from server");
           exit (1);
        }
    }
}


void *
TxThread (void *vargp)
{
    ArgStruct *p = (ArgStruct *)vargp;
    int n;

    useconds_t sleepTime = (useconds_t) ((1.0 / p->packet_rate) * 1000000); // does this work?

    //if (DEBUG)




    //FILE *f = fopen("temp", "w");

    struct timespec ts1 =  When2();

    //int i = 0;
    //for (; i < 10000; i++) {// 1) {
    while(1) {
        n = write (p->commfd, p->sbuff, PSIZE);
        if (DEBUG)
            printf ("Sent msg %s in %d bytes to server\n", p->sbuff, n);

        if (n < 0) {
            perror ("write to server");
            exit (1);
        }

//        usleep (sleepTime);
    }

    struct timespec ts2 = When2();

    long nsecs = ts2.tv_nsec - ts1.tv_nsec;
    fprintf(f, "time is: %ld\n", nsecs);
    fprintf (f, "Sleep time is: %d\n", sleepTime);
    fclose(f);

    exit(0);

}


void
SimpleWrite (ArgStruct *p)
{
    // This will run only on the client side. Send some data, 
    // receive some data and get rid of it at function exit.
    // OK to reallocate a buffer every time we call this because
    // we don't really need to worry too much about client performance.
    // a string of length PSIZE?

    // Start send/receive threads
    printf ("Getting ready to start the threading...\n");
    int total_threads = 9;
    pthread_t tid[total_threads];
    pthread_create (&tid[0], NULL, RxThread, (void *)p);

    int i = 1;
    for (; i < total_threads; i++) {
        pthread_create(&tid[i], NULL, TxThread, (void *) p);
    }

    pthread_join (tid[0], NULL);

    for (i = 1; i < total_threads; i++) {
        pthread_join(tid[i], NULL);
    }

    printf ("Threading returned! What happened?\n");

    // I think this will never return unless the threads die?
}


void TimestampWrite (ArgStruct *p)
{
    // Send and then receive an echoed timestamp.
    // Return a pointer to the stored timestamp. 
    // TODO this might be broken b/c of addressing
    int n;
    struct timespec sendtime, recvtime;
    struct sockaddr_in *remote = NULL;
    socklen_t len;

    remote = &(p->prot.sin1);
    len = sizeof (*remote); 
    sendtime = When2 ();
    snprintf (p->s_ptr, PSIZE, "%lld,%.9ld%-31s",
            (long long) sendtime.tv_sec, sendtime.tv_nsec, ",");

    n = sendto (p->commfd, p->s_ptr, PSIZE - 1, 0,
            (struct sockaddr *) remote, len); 
    if (n < 0) {
        perror ("write");
        exit (1);
    }

    if (DEBUG)
        printf ("send buffer: %s, %d bytes written\n", p->s_ptr, n);

    memset (p->s_ptr, 0, PSIZE);
    
    n = recvfrom (p->commfd, p->s_ptr, PSIZE, 0, 
            (struct sockaddr *) remote, &len);
    if (n < 0) {
        perror ("read");
        exit (1);
    }
    recvtime = When2 ();        

    if (DEBUG)
        printf ("Got timestamp: %s, %d bytes read\n", p->s_ptr, n);
    snprintf (p->lbuff, PSIZE, "%s", p->s_ptr);
    snprintf (p->lbuff + PSIZE - 1, PSIZE, "%lld,%.9ld\n", 
            (long long) recvtime.tv_sec, recvtime.tv_nsec);
}


void
Echo (ArgStruct *p)
{
    // Server-side only!
    // Loop through for expduration seconds and count each packet you send out
    // Start counting packets after a few seconds to stabilize connection(s)
    int i, m, n; 

    struct mmsghdr msgs[MAXEVENTS];      // headers
    struct iovec iovecs[MAXEVENTS];        // packet info
    char bufs[MAXEVENTS][PSIZE + 1];            // packet buffers
    struct sockaddr_in addrs[MAXEVENTS];    // return addresses

    // Wait a few seconds to let clients come online
    if (!p->latency) {
        printf ("Waiting for clients to start up...\n");
        sleep (p->online_wait + 5);
    }

    for (i = 0; i < MAXEVENTS; i++) {
        iovecs[i].iov_base          = bufs[i];
        iovecs[i].iov_len           = PSIZE;
        msgs[i].msg_hdr.msg_iov     = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen  = 1;
        msgs[i].msg_hdr.msg_name    = &addrs[i];
        msgs[i].msg_hdr.msg_namelen = sizeof (addrs[i]);
    }
    
    p->counter = 0;

    p->program_state = warmup;
    printf ("Assuming all clients have come online! Setting alarm...\n");
    alarm (WARMUP);

    // struct timespec timeout = { .tv_nsec = 10 * 1000UL, };
    while (p->program_state != end) { 
        if (DEBUG)
            printf ("waiting for messages...\n");
        // Read data from client; m is number of messages received 
        m = recvmmsg (p->commfd, msgs, MAXEVENTS, MSG_WAITFORONE, NULL); // &timeout);
        if (DEBUG)
            printf ("Got %d messages.\n", m);

        if (m < 0) {
            perror ("read");
            exit (1);
        }

        // Process each of the messages in msgvec
        for (i = 0; i < m; i++) {
            // Echo data back to client
            bufs[i][msgs[i].msg_len] = 0;
            if (DEBUG)
                printf ("Got a packet! Contents: %s, len %d\n", bufs[i], msgs[i].msg_len);
            n = sendto (p->commfd, bufs[i], PSIZE, 0, 
                    (struct sockaddr *) &addrs[i], msgs[i].msg_hdr.msg_namelen);
            if (n < 0) {
                perror ("write");
                exit (1);
            }
        }

        // Count successfully echoed packets
        if (p->program_state == experiment) {
            p->counter += m;
        }
    }

    printf("Ending echo service...\n");
}


void
establish (ArgStruct *p)
{
    struct protoent *proto;
    int one = 1;

    if (p->rcv) {
        p->commfd = p->servicefd;
        
        if (p->commfd < 0) {
            printf ("Server: accept failed! errno=%d\n", errno);
            exit (-12);
        }

        if (!(proto = getprotobyname ("udp"))) {
            printf ("unknown protocol!\n");
            exit (555);
        }

        // For UDP, this will let multiple processes to bind to the same port;
        // here we want it so we can immediately reuse the same socket
        if (setsockopt (p->commfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (int))) {
            perror ("tester: server: unable to setsockopt!");
            exit (557);
        }
        if (setsockopt (p->commfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof (int))) {
            perror ("tester: server: unable to setsockopt!");
            exit (557);
        }
    }

    printf ("Established the connection...\n");
}


void
ThroughputSetup (ArgStruct *p)
{
    Setup (p);
}


void 
CleanUp (ArgStruct *p)
{
   printf ("Quitting!\n");

   close (p->commfd);

}


void 
Reset (ArgStruct *p)
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

}


void PrepareToReceive(ArgStruct *p)
{
    /*
     * The Berkeley sockets interface doesn't have a method to pre-post
     * a buffer for reception of data.
    */
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




