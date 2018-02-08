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
    char s[512];        /* Empty string for filepath */
    char cpy_s[512];	/* For basename */
    int default_outdir;     /* bool; use default outdir? */
    char *outdir;

    int default_outfile;    /* bool; use default outfile? */
    char *outfile;

    int nrtts;
    int sleep_interval; /* How long to sleep b/t latency pings (usec) */
    // double duration;

    int c, i;

    /* Initialize vars that may change from default due to arguments */
    default_outdir = 1;
    default_outfile = 1;
    sleep_interval = 0;
    nrtts = NRTTS;
    args.latency = 1;  // Default to do latency; this is arbitrary
    args.expduration = 20;  // Seconds; default for throughput, will get
                            // overridden for latency or by args
    args.online_wait = 0;

    signal (SIGINT, SignalHandler);
    signal (SIGALRM, SignalHandler);

    args.host = NULL;
    args.port = DEFPORT; /* just in case the user doesn't set this. */

    // This gets swapped for instances that specify a host (-H opt)
    args.tr = 0;
    args.rcv = 1;

    /* Parse the arguments. See Usage for description */
    while ((c = getopt (argc, argv, "o:d:H:r:c:P:s:tu:lw:h:R")) != -1)
    {
        switch (c)
        {
            case 'o': default_outfile = 0;
		      outfile = optarg;
                      printf ("Sending output to file %s\n", outfile); 
                      fflush(stdout);
                      break;

            case 'd': default_outdir = 0;
                      outdir = optarg;
                      printf ("Sending output to directory %s\n", outdir); 
                      fflush(stdout);
                      break;

            case 'H': args.tr = 1;       /* -H implies transmit node */
                      args.rcv = 0;
                      args.host = (char *) malloc (strlen (optarg) + 1);
                      strcpy (args.host, optarg);
                      break;

            case 'r': nrtts = atoi (optarg);
                      break;

    	    case 'c': args.ncli = atoi (optarg);
		              break;

            case 'P': args.port = atoi (optarg);
                      break;

            case 's': sleep_interval = atoi (optarg);
                      break;

            case 't': args.latency = 0;
                      break;

            case 'u': args.expduration = atoi (optarg);
                      break;

            case 'w': args.online_wait = atoi (optarg);  // How long to wait for clients to come up
                      break;

            case 'l': args.collect_stats = 1;
                      break; 

            case 'h': PrintUsage ();
                      exit (0);

            case 'R': args.packet_rate = atoi(optarg);
                      break;

            default: 
                     PrintUsage (); 
                     exit (-12);
       }
    }
    
    /* Let modules initialize related vars, and possibly call a library init
       function that requires argc and argv */
    Init (&args, &argc, &argv);   /* This will set args.tr and args.rcv */

    /* FOR LATENCY */
    if (args.latency) {
        // Some huge number, don't actually care
        args.expduration = 1000000;

        // Use default filename construction for results
        if (default_outfile) {
            int exp_timestamp;
                exp_timestamp = (int) time (0);
            outfile = (char *) malloc (256);
            snprintf (outfile, 256, "%s-%d-r%d-s%d.out", whichproto, 
                exp_timestamp, nrtts, sleep_interval);
            }

        if (default_outdir) {
            outdir = (char *) malloc (256);
            snprintf (outdir, 256, "default");
        }

        snprintf (s, 512, "results/%s/%s", outdir, outfile);
        memcpy (cpy_s, s, 512);
        printf ("Results going into %s\n", s); 
        mkdir (dirname (cpy_s), 0777);

        Setup (&args);

        if (args.tr) {
            if ((out = fopen (s, "wb")) == NULL) {
                fprintf (stderr,"Can't open %s for output\n", s);
                exit (1);
            }
        } else {
            out = stdout;
        }
     
        // Get some number of latency measurements
        printf ("Collecting latency measurements to file %s.\n", s);

        int n;
        double rtt;
        double t0;
        
        if (args.tr) {
            // Warm up
            for (n = 0; n < (nrtts / 4); n++) {
                //SimpleWrite (&args);
                //usleep (sleep_interval);
            }

            // Take measurements
            t0 = When ();
            for (n = 0; n < nrtts; n++) {
                TimestampWrite (&args); 
                fwrite (args.lbuff, strlen (args.lbuff), 1, out);
                usleep (sleep_interval);
            }

            args.duration = When () - t0;
            rtt = args.duration/nrtts;

            // Note these are inflated by the I/O done to record individual
            // packet RTTs as well as sleep time (if any)
            printf ("\n");
            printf ("Average RTT: %f\n", rtt);
            printf ("Experiment duration: %f for %d packets\n", 
                    args.duration, nrtts);
            printf ("Printed results to file %s\n", s);

        } else if (args.rcv) {
            Echo (&args);
        }

    } else {
        /* FOR THROUGHPUT */

        // If there's no file specified, do not record results 
        if (args.rcv) {
            if (default_outfile) {
                printf ("No file specified; not recording results!\n");	
                args.outfile = NULL;
            } else {
                if (default_outdir) {
                    outdir = (char *) malloc (256);
                    snprintf (outdir, 256, "default");
                }
                
                snprintf (s, 512, "results/%s/%s", outdir, outfile);
                memcpy (cpy_s, s, 512);
                printf ("Results going into %s\n", s); 
                mkdir (dirname (cpy_s), 0777);

                if ((out = fopen (s, "ab")) == NULL) {
                    fprintf (stderr,"Can't open %s for output\n", s);
                    exit (1);
                }
                args.outfile = s;
            }
        }

        // Create the string that will be echoed
        char *abcs = "abcdefghijklmnopqrstuvwxyz";
        for (i = 0; i < PSIZE; i++) {
            args.sbuff[i] = abcs[i % strlen (abcs)];
        }
        args.sbuff[PSIZE] = '\0';
        printf ("Garbage string: %s\n", args.sbuff);

        ThroughputSetup (&args);

        // Get throughput measurements
        if (args.tr) {
            // Send some huge number of packets
            printf ("Sending a ton of packets to the server...\n");
            while (1) {
                SimpleWrite (&args);
            }
        } else if (args.rcv) {
            printf ("Getting ready to receive packets...\n");

            Echo (&args);

            if (!default_outfile) {
                fprintf (out, "%d,%f\n", args.ncli, args.counter/args.duration);
            }
                printf ("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                printf ("Received %" PRIu64 " packets in %f seconds\n", 
                        args.counter, args.duration);
            printf ("Throughput is %f pps\n", args.counter/args.duration);
                printf ("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            }
    }

    ExitStrategy ();
    return 0;
}


void
PrintUsage (void)
{
    printf ("\n");
    printf ("To run server, run '[sudo] ./NPxyz'\n");
    printf ("To run client, run '[sudo] ./NPxyz -H server-hostname ...'\n");
    printf ("(mTCP is the only transport that needs sudo [for now])\n\n");
    printf ("Options (client only unless otherwise specified):\n");
    printf ("\t-o\twhere to write latency output; default\n"
            "\t\tresults/timestamp+opts\n");
    printf ("\t-H\tIP/hostname of receiver\n");
    printf ("\t-r\tnumber of latency measurements to collect\n"
            "\t\t(ping packets to send); default 1000\n");
    printf ("\t-P\t(client AND server) port number, default 8000\n");
    printf ("\t-s\tsleeptime for throttling client send rate\n"
            "(default 0)\n");
    printf ("\t-t\tmeasure throughput (default latency)\n");
    printf ("\t-h\t(client or server) print usage\n");
    printf ("\t-R\t Packet sending rate in pkts/sec\n");
    printf ("\n");
    exit (0);
}


void
CollectStats (ArgStruct *p)
{
    int pid = fork ();

    if (pid == 0) {
        printf ("[server] Launching collectl...\n");
        fflush (stdout);
        char nsamples[128];
        
        snprintf (nsamples, 128, "-c%d", p->expduration);

        if (p->outfile == NULL) {
            // Nowhere to save results; dump to terminal
            char *argv[4];

            argv[0] = "collectl";
            argv[1] = "-sc";
            argv[2] = nsamples;
            argv[3] = NULL;
            execvp ("collectl", argv);

        } else {
            // else save results to file
            char *argv[8];

            argv[0] = "collectl";
            argv[1] = "-P";
            argv[2] = "-f";
            argv[3] = p->outfile;
            argv[4] = "-sc";
            argv[5] = nsamples;
            argv[6] = "-oaz";
            argv[7] = NULL;
            execvp ("collectl", argv);
        }
    }
}


void
ExitStrategy (void)
{
    // Clean up the sockets, close open FDs
    if (args.tr) {
        // TODO fclose (out);
    }
    CleanUp (&args);
}


void
SignalHandler (int signum) {
    if (signum == SIGINT) {
        printf ("Got a SIGINT...\n");
        ExitStrategy ();
        exit (0);
    } else if (signum == SIGALRM) {
        // We only need to set a new alarm if this is a throughput experiment
        // Otherwise just let the clock run; latency duration is measured by 
        // number of packets (-r option).
        if (!args.latency) {
            if (args.program_state == warmup) {
                printf ("Starting to count packets for throughput...\n");
                args.program_state = experiment; 
                if (args.collect_stats) {
                    CollectStats(&args);
                }
                args.t0 = When ();
                alarm (args.expduration);
            } else if (args.program_state == experiment) {
                // Experiment has completed; let it keep running without counting
                // packets to allow other servers to finish up
                args.program_state = cooldown;
                args.duration = When () - args.t0;
                printf ("Experiment over, stopping counting packets...\n");
                alarm (COOLDOWN);
            } else if (args.program_state == cooldown) {
                // The last signal; end the experiment by setting p->tput_done
                args.program_state = end;
            }
        }
    }
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

