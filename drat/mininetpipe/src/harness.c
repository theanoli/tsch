/* Use this to run tests with arbitrary protocols; functions
   to establish/tear down connections and to send/receive packets
   will be protocol-specific and will be defined in other files.
*/

#include "harness.h"

#define NRTTS 1000

extern char *optarg;

int 
main (int argc, char **argv)
{
    FILE *out;          /* Output data file                          */
    char s[255];        /* Empty string */
    int nrtts;
    double t0, tlast;

    ArgStruct args;     /* Arguments to be passed to protocol modules */

    int c;
    int n;

    /* Initialize vars that may change from default due to arguments */
    strcpy (s, "np.out");   /* Default output file */
    nrtts = NRTTS;

    /* Let modules initialize related vars, and possibly call a library init
       function that requires argc and argv */
    printf ("Collecting %d latency measurements.\n", nrtts);
    Init (&args, &argc, &argv);   /* This will set args.tr and args.rcv */

    args.host  = NULL;
    args.port = DEFPORT; /* just in case the user doesn't set this. */

    /* Parse the arguments. See Usage for description */
    while ((c = getopt (argc, argv, "h:o:P:r:")) != -1)
    {
        switch (c)
        {
            case 'o': strcpy (s, optarg);
                      printf ("Sending output to %s\n", s); fflush(stdout);
                      break;

            case 'h': args.tr = 1;       /* -h implies transmit node */
                      args.rcv = 0;
                      args.host = (char *) malloc (strlen (optarg) + 1);
                      strcpy (args.host, optarg);
                      break;

            case 'r': nrtts = atoi (optarg);
                      break;

	        case 'P': args.port = atoi (optarg);
		              break;

            default: 
                     PrintUsage (); 
                     exit (-12);
       }
    }
    
 
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
    char *timing;
    t0 = When ();
    for (n = 0; n < nrtts; n++) {
        if (args.tr) {
            SendData (&args);
            timing = RecvData (&args);
            if (strlen (timing) > 0) {
               fwrite (timing, strlen (timing), 1, out);
            }
            memset (args.s_ptr, 0, PSIZE);
            memset (args.r_ptr, 0, PSIZE);
            memset (timing, 0, PSIZE * 2);
        } else if (args.rcv) {
            RecvData (&args);
            SendData (&args);
            memset (args.s_ptr, 0, PSIZE);
            memset (args.r_ptr, 0, PSIZE);
        }
    }
    tlast = (When () - t0)/(2*nrtts);
    printf ("RTT: %f\n", tlast);

    // Clean up the sockets, close open FDs
    if (args.tr) {
        fclose (out);
    }
    CleanUp (&args);
    return 0;
}


void
PrintUsage (void)
{
    // TODO
    printf ("Should have usage info here!\n");
    exit (0);
}


double
When (void)
{
    struct timeval tp;
    gettimeofday (&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}


struct timespec
When2 (void)
{
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
