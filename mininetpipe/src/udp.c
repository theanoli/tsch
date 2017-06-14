#include "harness.h"

void
Init (ArgStruct *p, int *pargc, char ***pargv)
{
    p->reset_conn = 0;
}

void
Setup (ArgStruct *p)
{
    p->s_str = (char *) malloc (PSIZE);
    p->r_str = (char *) malloc (PSIZE);

    if ((p->s_ptr == NULL) || (p->r_ptr == NULL)) {
        printf ("Malloc error!\n");
        exit (1);
    }

    int one = 1;
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
        printf ("tester: can't open stream socket! errno=%d\n", errno);
        exit (-4);
    }

    if (!(proto = getprotobyname ("udp"))) {
        printf ("tester: protocol 'udp' unknown!\n");
        exit (555);
    }

    // Don't need to set NODELAY; no such thing in UDP
    
    if (p->tr) {
        // Sender side
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

        lsin->sin_port = htons (p->port);
        p->commfd = sockfd;

    } else if (p->rcv) {
        // Receiver side
        memset ((char *) lsin1, 0, sizeof (*lsin1));
        lsin1->sin_family       = AF_INET;
        lsin1->sin_addr.s_addr  = htonl (INADDR_ANY);
        lsin1->sin_port         = htons (p->port);

        if (bind (sockfd, (struct sockaddr *) lsin1, sizeof (*lsin1)) < 0) {
            printf ("tester: server: bind on local address failed!"
                    "errno=%d", errno);
            exit (-6);
        }

        p->servicefd = sockfd;
        printf ("Listening on socket %d...\n", sockfd);

    }

    establish (p);

}


void
SendData (ArgStruct *p)
{
    int bytesWritten, bytesLeft;
    char *q;
    struct timespec sendtime;

    bytesLeft = p->bufflen;
    bytesWritten = 0;

    if (p->tr) {
        sendtime = When2 ();
        snprintf (p->s_ptr, PSIZE, "%lld, %.9ld%-31s",
                (long long) sendtime.tv_sec, sendtime.tv_nsec, ",");
        q = p->s_ptr;
    } else {
        q = p->r_ptr;
    }

    p->bufflen = strlen (q);
    bytesLeft = p->bufflen;

    while ((bytesLeft > 0) &&
            (bytesWritten = send (p->commfd, q, bytesLeft, 0)) > 0) {
        bytesLeft -= bytesWritten;
        q += bytesWritten;
    }
    
    if (bytesWritten == -1) {
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
           (bytesRead = recv (p->commfd, q, bytesLeft, 0)) > 0) {
        bytesLeft -= bytesRead;
        q += bytesRead;
    }

    if ((bytesLeft > 0) && (bytesRead == 0)) {
        printf ("tester: \"end of file\" encountered on reading from socket\n");

    } else if (bytesRead == -1) {
        printf ("tester: read: error encountered, errno=%d\n", errno);
        exit (401);
    }

    if (p->tr) {
        struct timespec recvtime = When2 ();
        if ((buf = malloc (PSIZE * 2)) == NULL) {
            printf ("Malloc error!\n");
            exit (1);
        }

        snprintf (buf, PSIZE, "%s", p->r_ptr);
        snprintf (buf + PSIZE - 1, PSIZE, "%lld,%.9ld\n",
                (long long) recvtime.tv_sec, recvtime.tv_nsec);
        return buf;
    } else {
        // No timestamp from the receiver
        return NULL;
    }
}


void
establish (ArgStruct *p)
{
    int one = 1;
    socklen_t clen;
    struct protoent *proto;

    clen = (socklen_t) sizeof (p->prot.sin2);

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
        // any messages sent to that port will be delivered to all processes
        if (setsockopt (p->commfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (int))) {
            printf ("tester: server: unable to setsockopt! errno=%d\n", errno);
            exit (557);
        }
    }

    printf ("Connection established!\n");
}

void 
CleanUp (ArgStruct *p)
{
   char *quit = "QUIT";
   int q;

   printf ("Quitting!\n");

   if (p->tr) {

      q = send (p->commfd, quit, 5, 0);
      q = recv (p->commfd, quit, 5, 0);
      close (p->commfd);

   } else if (p->rcv) {

      q = recv (p->commfd, quit, 5, 0);
      q = send (p->commfd, quit, 5, 0);
      close (p->commfd);
      close (p->servicefd);

   }

   // Shut up the compiler
   if (q > 0) 
       return;
   else
       return;
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




