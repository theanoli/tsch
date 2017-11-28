#include "harness.h"

#define DEBUG 0

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
    struct hostent *addr;
    struct protoent *proto;
    int socket_family = AF_INET;

    host = p->host;
    lsin1 = &(p->prot.sin1);
    lsin2 = &(p->prot.sin2);

    memset ((char *) lsin1, 0, sizeof (*lsin1));
    memset ((char *) lsin2, 0, sizeof (*lsin2));

    if ((sockfd = socket (socket_family, SOCK_DGRAM, 0)) < 0) {
        perror ("tester: can't open stream socket!");
        exit (-4);
    }

    if (!(proto = getprotobyname ("udp"))) {
        printf ("tester: protocol 'udp' unknown!\n");
        exit (555);
    }

    // Don't need to set NODELAY; no such thing in UDP
    
    if (p->tr) {
        // Sender side: get info about the server
        if (atoi (host) > 0) {
            lsin1->sin_family = AF_INET;
            lsin1->sin_addr.s_addr = inet_addr (host);

        } else {
            if ((addr = gethostbyname (host)) == NULL) {
                printf ("tester: invalid hostname '%s'\n", host);
                exit (-5);
            }

            lsin1->sin_family = addr->h_addrtype;
            memmove (addr->h_addr, (char *) &(lsin1->sin_addr.s_addr), 
                    addr->h_length);

        }

        lsin1->sin_port = htons (p->port);
        p->commfd = sockfd;

    } else if (p->rcv) {
        // Receiver side
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

    establish (p);
}


void
SimpleWrite (ArgStruct *p)
{
    // This will run only on the client side. Send some data, 
    // receive some data and get rid of it at function exit.
    // OK to reallocate a buffer every time we call this because
    // we don't really need to worry too much about client performance.
    // TODO any reason to use a random string? Any reason to force
    // a string of length PSIZE?
    char buffer[PSIZE];
    int n;

    struct sockaddr_in *remote = NULL;
    socklen_t len;

    remote = &(p->prot.sin1);
    len = sizeof (*remote); 

    snprintf (buffer, PSIZE, "%s", "hello, world!");

    n = sendto (p->commfd, buffer, PSIZE, 0, 
            (struct sockaddr *) remote, len);
    if (n < 0) {
        perror ("write to server");
        exit (1);
    }

    memset (buffer, 0, PSIZE);

    n = recvfrom (p->commfd, buffer, PSIZE, 0,
            (struct sockaddr *) remote, &len);
    if (n < 0) {
        perror ("read from server");
        exit (1);
    }
}


void TimestampWrite (ArgStruct *p)
{
    // Send and then receive an echoed timestamp.
    // Return a pointer to the stored timestamp. 

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
    double tnull, t0, duration;
    int countstart = 0;
    struct sockaddr_in *remote = NULL;
    socklen_t len;

    remote = &(p->prot.sin2);
    len = sizeof (*remote);

    p->counter = 0;

    t0 = 0;  // Silence compiler
    tnull = When ();

    // Wait a few seconds to let clients come online
    if (!p->latency) {
        printf ("Waiting for clients to start up...\n");
        while ((duration = When () - tnull) < (0.001 * p->ncli + 2)) {

        }
    }

    // Add a two-second delay to let clients stabilize
    tnull = When ();  // Restart timer
    printf ("Assuming all clients have come online...\n");
    while ((duration = When () - tnull) < (p->expduration + 2)) {

        if (!p->latency) {
            if ((duration > 2) && (countstart == 0)) {
                printf ("Starting to count packets for throughput...\n");
                countstart = 1; 
                t0 = When ();
            }
        }

        int n; 
        char *q;

        // Read data from client 
        q = p->r_ptr;
        n = recvfrom (p->commfd, q, PSIZE - 1, MSG_DONTWAIT, 
                (struct sockaddr *) remote, &len);
	
	// Recvfrom is in nonblocking mode b/c of MSG_DONTWAIT
        if (n < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                continue;
            }
            perror ("read");
            exit (1);
        }

        // Echo data back to client
        n = sendto (p->commfd, p->r_ptr, PSIZE - 1, 0, 
                (struct sockaddr *) remote, len);
        if (n < 0) {
            perror ("write");
            exit (1);
        }

        // Count successfully echoed packets
        if (countstart) {
            (p->counter)++;
        }
                
    }

    p->duration = When () - t0;
}


void
establish (ArgStruct *p)
{
    struct protoent *proto;
    int one = 1;


    if (p->tr) {
        // Don't need to do anything for UDP
    } else if (p->rcv) {
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




