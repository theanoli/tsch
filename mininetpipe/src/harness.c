/* Use this to run tests with arbitrary protocols; functions
   to establish/tear down connections and to send/receive packets
   will be protocol-specific and will be defined in other files.
*/

#include "harness.h"

#define NRTTS 1000

extern char *optarg;

// Initialize these here so they are accessible to signal handler
ArgStruct args;     /* Arguments to be passed to protocol modules */
FILE *out;          /* Output data file                          */

int 
main (int argc, char **argv)
{
    char s[255];        /* Empty string */
    int default_out;    /* bool; use default outfile? */
    int nrtts;
    int sleep_interval; /* How long to sleep b/t latency pings (usec) */
    double t0, duration;

    int c;

    /* Initialize vars that may change from default due to arguments */
    default_out = 1;
    sleep_interval = 0;
    nrtts = NRTTS;

    signal (SIGINT, SignalHandler);

    args.host = NULL;
    args.port = DEFPORT; /* just in case the user doesn't set this. */

    // This gets swapped for instances that specify a host (-H opt)
    args.tr = 0;
    args.rcv = 1;

    /* Parse the arguments. See Usage for description */
    while ((c = getopt (argc, argv, "o:H:r:P:s:h")) != -1)
    {
        switch (c)
        {
            case 'o': default_out = 0;
                      memset (s, 0, 255);
                      strcpy (s, optarg);
                      printf ("Sending output to %s\n", s); fflush(stdout);
                      break;

            case 'H': args.tr = 1;       /* -H implies transmit node */
                      args.rcv = 0;
                      args.host = (char *) malloc (strlen (optarg) + 1);
                      strcpy (args.host, optarg);
                      break;

            case 'r': nrtts = atoi (optarg);
                      break;

	        case 'P': args.port = atoi (optarg);
		              break;

            case 's': sleep_interval = atoi (optarg);
                      break;

            case 'h': PrintUsage ();
                      exit (0);

            default: 
                     PrintUsage (); 
                     exit (-12);
       }
    }
    
    if (default_out) {
        int exp_timestamp;
        exp_timestamp = (int) time (0);
        snprintf (s, 255, "results/%s-%d-r%d-s%d.out", whichproto,
                exp_timestamp, nrtts, sleep_interval);
    }

    /* Let modules initialize related vars, and possibly call a library init
       function that requires argc and argv */
    Init (&args, &argc, &argv);   /* This will set args.tr and args.rcv */
    Setup (&args);

    if (args.tr) {
        if ((out = fopen (s, "wb")) == NULL) {
            fprintf (stderr,"Can't open %s for output\n", s);
            exit (1);
        }
    } else {
        out = stdout;
    }
 
    /* Get some number of latency measurements */
    // printf ("Collecting %d latency measurements to file %s.\n", nrtts, s);

    // char *timing;
    // int n;
    // double rtt;
    //
    // t0 = When ();
    // if (args.tr) {
    //     for (n = 0; n < nrtts; n++) {
    //         SendData (&args);
    //         timing = RecvData (&args);

    //         if ((strlen (timing) > 0) && (n > 1)) {
    //            fwrite (timing, strlen (timing), 1, out);
    //         }
    //         usleep (sleep_interval);
    //     }

    //     duration = When () - t0;
    //     rtt = duration/nrtts;
    //     
    //     // Note these are inflated by the I/O done to record individual
    //     // packet RTTs
    //     printf ("\n");
    //     printf ("Average RTT: %f\n", rtt);
    //     printf ("Experiment duration: %f\n", duration);
    //     printf ("Printed results to file %s\n", s);

    // } else if (args.rcv) {
    //     while (1) {
    //         RecvData (&args);
    //         SendData (&args);
    //     }
    // }

    /* Get throughput measurements */
    uint64_t counter = 0;

    t0 = When ();
    if (args.tr) {
        // Send some huge number of packets
        while (1) {
            SendData (&args);
            RecvData (&args);
        }
    } else if (args.rcv) {
        // Give clients time to ramp up send rate/stabilize
        while ((t0 + 5) > When ()) {
            RecvData (&args);
            SendData (&args);
        }

        // Start counting packets for EXPDURATION seconds
        printf ("Starting counting packets for throughput...\n");
        t0 = When ();
        while ((duration = When () - t0) < EXPDURATION) {
            RecvData (&args);
            SendData (&args);
            counter++;
        }

        printf ("Received %" PRIu64 " packets in %f seconds\n", counter, duration);
        printf ("Throughput is %f PPS\n", counter/duration);
    }

    ExitStrategy ();
    return 0;
}


void
PrintUsage (void)
{
    printf ("\n");
    printf ("To run server, run 'sudo ./NPxyz'\n");
    printf ("To run client, run 'sudo ./NPxyz -H server-hostname ...'\n\n");
    printf ("Options (client only unless otherwise specified):\n");
    printf ("\t-o\twhere to write latency output; default\n"
            "\t\tresults/timestamp+opts\n");
    printf ("\t-H\tIP/hostname of receiver\n");
    printf ("\t-r\tnumber of latency measurements to collect\n"
            "\t\t(ping packets to send); default 1000\n");
    printf ("\t-P\t(client AND server) port number, default 8000\n");
    printf ("\t-s\tsleeptime for throttling client send rate\n"
            "(default 0)\n");
    printf ("\t-h\t(client or server) print usage\n");
    printf ("\n");
    exit (0);
}


void
ExitStrategy (void)
{
    // Clean up the sockets, close open FDs
    if (args.tr) {
        fclose (out);
    }
    CleanUp (&args);
}


void
SignalHandler (int signum) {
    printf ("Got signal %d\n...", signum);
    ExitStrategy ();
    exit (0);
}


double
When (void)
{
    // Low-resolution timestamp for coarse-grained measurements
    struct timeval tp;
    gettimeofday (&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}


struct timespec
When2 (void)
{
    // More precise timestamping function; uses machine time instead of
    // clock time to avoid sync issues
    struct timespec spec;

    clock_gettime (CLOCK_MONOTONIC, &spec);
    return spec;
}


struct timespec
diff (struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}
