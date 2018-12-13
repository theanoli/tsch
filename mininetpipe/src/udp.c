#include "harness.h"

#define DEBUG 0
#define TIMEOUT 100  // in usec

void
Init (ProgramArgs *p, int *pargc, char ***pargv)
{
    ThreadArgs *targs = (ThreadArgs *)calloc (p->nthreads, sizeof (ThreadArgs));
    if (targs == NULL) {
        printf ("Error malloc'ing space for thread args!\n");
        exit (-10);
    }
    p->thread_data = targs;

    p->tids = (pthread_t *)calloc (p->nthreads, sizeof (pthread_t));
    if (p->tids == NULL) {
        printf ("Failed to malloc space for tids!\n");
        exit (-82);
    }

    UpdateProgramState (startup);
}


void 
LaunchThreads (ProgramArgs *p)
{
    int i, ret;
    cpu_set_t cpuset __attribute__((__unused__));

    ThreadArgs *targs = p->thread_data;

    for (i = 0; i < p->nthreads; i++) {
        targs[i].machineid = p->machineid;
        targs[i].threadid = i;
        snprintf (targs[i].threadname, 128, "[%s.%d]", p->machineid, i);

        if (p->rcv) {
            targs[i].port = p->port + i % NSERVERCORES;
        } else if (p->tr) {
            targs[i].port = p->port + i % p->ncli % NSERVERCORES;
        }
        targs[i].host = p->host;
        targs[i].tr = p->tr;
        targs[i].rcv = p->rcv;
        targs[i].online_wait = p->online_wait;
        targs[i].latency = p->latency;
        targs[i].no_record = p->no_record;
        targs[i].nrtts = p->nrtts;
        memcpy (targs[i].sbuff, p->sbuff, PSIZE + 1);

        printf ("Setting up filenames...\n");
        if (p->tr) {
            setup_filenames (&targs[i]);
        }

        if (p->rcv) {
            printf ("[%s] Launching thread %d...\n", p->machineid, i);
        }
        pthread_create (&p->tids[i], NULL, ThreadEntry, (void *)&targs[i]);
        
        if (p->pinthreads) {
            CPU_ZERO (&cpuset);
            CPU_SET (i % NSERVERCORES, &cpuset);
            ret = pthread_setaffinity_np (p->tids[i], sizeof (cpu_set_t), &cpuset);
            if (ret != 0) {
                printf ("[%s] Couldn't pin thread %d to core!\n", p->machineid, i);
                exit (-14);
            }
        }
    }
}


// Entry point for new threads
void *
ThreadEntry (void *vargp)
{
    ThreadArgs *targs = (ThreadArgs *)vargp; 
    Setup (targs);
    
    if (targs->tr) {
        if (targs->latency) {
            TimestampWrite (targs);
        } else {
            // FOR OPEN-LOOP CLIENTS; see TCP for closed-loop format
            // Create an additional sending thread
            pthread_t tid;
            pthread_create (&tid, NULL, SimpleTx, (void *)targs);
            
            // This thread does the receiving
            SimpleRx (targs);
        }
    } else if (targs->rcv) {
        if (targs->latency) {
            Echo (targs);
        } else {
            Echo (targs);
        
            printf ("\n");
            printf ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf ("%s Received %" PRIu64 " packets in %f seconds\n", 
                        targs->threadname, targs->counter, targs->duration);
            printf ("Throughput is %f pps\n", targs->counter/targs->duration);
            printf ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

            targs->pps = targs->counter/targs->duration;
            if (!targs->no_record) {
                record_throughput (targs);
            }

        }

        CleanUp (targs);
    }

    return 0;
}


void
Setup (ThreadArgs *p)
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
    struct timeval read_timeout;

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

        printf ("%s successfully made contact on port %d!\n", p->threadname, p->port);
        read_timeout.tv_sec = 0;
        read_timeout.tv_usec = TIMEOUT;
        setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout,
                sizeof (read_timeout));

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
            printf ("%s ", p->threadname);
            perror ("tester: server: bind on local address failed!"
                    "errno");
            exit (-6);
        }

        p->servicefd = sockfd;
    }

    establish (p);
}


void
SimpleRx (ThreadArgs *p)
{
    int n; 

    while (p->program_state != experiment) {
    }

    while (1) {
        n = read (p->commfd, p->rbuff, PSIZE);

        if (n < 0) {
            printf ("%s error: ", p->threadname);
            perror ("read from server");
            exit (1);
        }
    }
}


// This will be spawned in a different thread
void *
SimpleTx (void *vargp)
{
    ThreadArgs *p = (ThreadArgs *)vargp;
    int n;
    
    while (p->program_state != experiment) {

    }

    while (1) {
        n = write (p->commfd, p->sbuff, PSIZE);

        if (n < 0) {
            printf ("Client %d error: ", p->threadid);
            perror ("write to server");
            exit (1);
        }

        // usleep (p->sleeptime);
    }

    return 0;
}


void 
TimestampWrite (ThreadArgs *p)
{
    // Send and then receive an echoed timestamp.
    // Return a pointer to the stored timestamp. 
    // TODO this might be broken b/c of addressing
    char pbuffer[PSIZE];  // for packets
    int n, m;
    int i;
    struct timespec sendtime, recvtime;
    FILE *out;
    struct sockaddr_in *remote = NULL;
    socklen_t len;

    remote = &(p->prot.sin1);
    len = sizeof (*remote); 

    p->lbuff = malloc (PSIZE * 2);
    if (p->lbuff == NULL) {
        fprintf (stderr, "Malloc for lbuff failed!\n");
        exit (1);
    }

    if (p->tr && !p->no_record) {
        if ((out = fopen (p->latency_outfile, "wb")) == NULL) {
            fprintf (stderr, "Can't open %s for output!\n", p->latency_outfile);
            exit (1);
        }
    } else {
        out = stdout;
    }

    while (p->program_state != experiment) {
    }

    printf ("Getting ready to send timestamp packets...\n");
    fflush (stdout);
    for (i = 0; i < p->nrtts; i++) {
        sendtime = When2 ();
        snprintf (pbuffer, PSIZE, "%lld,%.9ld%-31s",
                (long long) sendtime.tv_sec, sendtime.tv_nsec, ",");

        n = sendto (p->commfd, pbuffer, PSIZE - 1, 0,
            (struct sockaddr *) remote, len); 
        if (n < 0) {
            perror ("write");
            exit (1);
        }

        debug_print (DEBUG, "pbuffer: %s, %d bytes written\n", pbuffer, n);

        memset (pbuffer, 0, PSIZE);
        
        // If the recvfrom times out (ret. -1), resend the packet and go 
        // through the loop again 
        while ((n = recvfrom (p->commfd, pbuffer, PSIZE, 0, 
                (struct sockaddr *) remote, &len)) == -1) {
            p->retransmits++;
            printf ("%d retransmits\n", (int)p->retransmits);
            fflush (stdout);

            n = sendto (p->commfd, pbuffer, PSIZE - 1, 0, 
                    (struct sockaddr *) remote, len);
            if (n < 0) {
                perror ("write");
                exit (1);
            }
        }
        recvtime = When2 ();        

        if (n == 0) {
            printf ("Skipping...\n");
            return;
        }

        memset (p->lbuff, 0, PSIZE * 2);
        m = snprintf (p->lbuff, PSIZE, "%s", pbuffer);
        snprintf (p->lbuff + m, PSIZE, "%lld,%.9ld\n", 
                (long long) recvtime.tv_sec, recvtime.tv_nsec);

        if (!p->no_record) {
            fwrite (p->lbuff, strlen (p->lbuff), 1, out);
        }
        debug_print (DEBUG, "Got timestamp: %s, %d bytes read\n", pbuffer, n);
        
        if (n < 0) {
            perror ("read");
            exit (1);
        }
    }
}


void
Echo (ThreadArgs *p)
{
    // Server-side only!
    // Loop through for expduration seconds and count each packet you send out
    // Start counting packets after a few seconds to stabilize connection(s)
    int i, m, n; 

    struct mmsghdr msgs[MAXEVENTS];      // headers
    struct iovec iovecs[MAXEVENTS];        // packet info
    char bufs[MAXEVENTS][PSIZE + 1];            // packet buffers
    struct sockaddr_in addrs[MAXEVENTS];    // return addresses

    for (i = 0; i < MAXEVENTS; i++) {
        iovecs[i].iov_base          = bufs[i];
        iovecs[i].iov_len           = PSIZE + 1;
        msgs[i].msg_hdr.msg_iov     = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen  = 1;
        msgs[i].msg_hdr.msg_name    = &addrs[i];
        msgs[i].msg_hdr.msg_namelen = sizeof (addrs[i]);
    }
    
    p->counter = 0;

    // Wait for state to get updated to warmup
    while (p->program_state == startup) {
    }

    struct timespec timeout = { .tv_nsec = 1000 * 1000UL, };
    printf ("%s Entering packet receive mode...\n", p->threadname);

    while (p->program_state != end) { 
        // Read data from client; m is number of messages received 
        m = recvmmsg (p->commfd, msgs, 2048, MSG_WAITFORONE, &timeout);
        if (DEBUG)
            printf ("Got %d messages.\n", m);

        if (m < 0) {
            // if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            if ((errno == EINTR) || (errno == EAGAIN)) {
                 continue;
            }
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

    p->tput_done = 1;
}


void
establish (ThreadArgs *p)
{
    struct protoent *proto;
    int one = 1;

    if (p->rcv) {
        p->commfd = p->servicefd;
        
        if (p->commfd < 0) {
            printf ("%s accept failed! errno=%d\n", p->threadname, errno);
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

        printf ("%s Established a connection...\n", p->threadname);
    }
}


void
ThroughputSetup (ThreadArgs *p)
{
    Setup (p);
}


void 
CleanUp (ThreadArgs *p)
{

   printf ("%s Quitting!\n", p->threadname);

   close (p->commfd);

}

