/*****************************************************************************/
/* Borrows heavily from...                                                   */
/*****************************************************************************/
/* "NetPIPE" -- Network Protocol Independent Performance Evaluator.          */
/* Copyright 1997, 1998 Iowa State University Research Foundation, Inc.      */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation.  You should have received a copy of the     */
/* GNU General Public License along with this program; if not, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/*     * tcp.c              ---- TCP calls source                            */
/*     * tcp.h              ---- Include file for TCP calls and data structs */
/*****************************************************************************/
/* ...as modified for IX                                                     */
/*****************************************************************************/

#include "harness.h"

int doing_reset = 0;

void
Init (ArgStruct *p, int *pargc, char ***pargv)
{
    // Probably don't need to actually do anything here...
    p->reset_conn = 0;
}


void
Setup (ArgStruct *p)
{
    p->s_ptr = (char *) malloc (PSIZE);
    p->r_ptr = (char *) malloc (PSIZE);

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

    if ((sockfd = socket (socket_family, SOCK_STREAM, 0)) < 0) {
        printf ("tester: can't open stream socket! errno=%d\n", errno);
        exit (-4);
    }

    if (!(proto = getprotobyname ("tcp"))) {
        printf ("tester: protocol 'tcp' unknown!\n");
        exit (555);
    }

    if (setsockopt (sockfd, proto->p_proto, 
                TCP_NODELAY, &one, sizeof (one)) < 0) {
        printf ("tester: setsockopt: TCP_NODELAY failed! errno=%d\n", errno);
        exit (556);
    }

    // Sender stuff first...
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
            memmove (addr->h_addr, (char *) &(lsin1->sin_addr.s_addr),
                    addr->h_length);
        }

        lsin1->sin_port = htons (p->port);
        p->commfd = sockfd;
    
    } else if (p->rcv) {
        // Now receiver stuff...
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
    struct timespec sendtime;  // this wil get ping-ponged to measure latency

    bytesLeft = p->bufflen;
    bytesWritten = 0;
    
    if (p->tr) {
        // Transmitter (client) should send timestamp
        sendtime = When2 ();
        snprintf (p->s_ptr, PSIZE, "%lld,%.9ld%-31s",
                (long long) sendtime.tv_sec, sendtime.tv_nsec, ",");
        q = p->s_ptr;
    } else {
        // Receiver echoes back whatever it got
        q = p->r_ptr; 
    }

    p->bufflen = strlen (q);
    bytesLeft = p->bufflen;

    while ((bytesLeft > 0) &&
            (bytesWritten = write (p->commfd, q, bytesLeft)) > 0) {
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
           (bytesRead = read (p->commfd, q, bytesLeft)) > 0) {
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
        while (connect (p->commfd, (struct sockaddr *) &(p->prot.sin1), 
                    sizeof (p->prot.sin1)) < 0) {
            if (!doing_reset || errno != ECONNREFUSED) {
                printf ("Client: cannot connect! errno=%d\n", errno);
                exit (-10);
            }
        }
    } else if (p->rcv) {
        listen (p->servicefd, 5);
        p->commfd = accept (p->servicefd, (struct sockaddr *) &(p->prot.sin2),
                &clen);
        
        if (p->commfd < 0) {
            printf ("Server: accept failed! errno=%d\n", errno);
            exit (-12);
        }

        // Attempt to set TCP_NODELAY; may or may not be propagated to accepted
        // sockets. 
        if (!(proto = getprotobyname ("tcp"))) {
            printf ("unknown protocol!\n");
            exit (555);
        }

        if (setsockopt (p->commfd, proto->p_proto, TCP_NODELAY, &one, sizeof (one))
                < 0) {
            printf ("setsockopt: TCP_NODELAY failed! errno=%d\n", errno);
            exit (556);
        }

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

      q = write (p->commfd, quit, 5);
      q = read (p->commfd, quit, 5);
      close (p->commfd);

   } else if (p->rcv) {

      q = read (p->commfd,quit, 5);
      q = write (p->commfd,quit,5);
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


