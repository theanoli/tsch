/* Use this to run tests with arbitrary protocols; functions
   to establish/tear down connections and to send/receive packets
   will be protocol-specific and will be defined in other files.
*/

#include "harness.h"

extern char *optarg;

int main(int argc, char **argv)
{
    FILE        *out;           /* Output data file                          */
    


    char        s[255],s2[255],delim[255],*pstr; /* Generic strings          */
    int         *memcache;      /* used to flush cache                       */

    int         len_buf_align,  /* meaningful when args.cache is 0. buflen   */
                                /* rounded up to be divisible by 8           */
                num_buf_align;  /* meaningful when args.cache is 0. number   */
                                /* of aligned buffers in memtmp              */

    int         c,              /* option index                              */
                i, j, n, nq,    /* Loop indices                              */
                asyncReceive=0, /* Pre-post a receive buffer?                */
                bufalign=16*1024,/* Boundary to align buffer to              */
                errFlag,        /* Error occurred in inner testing loop      */
                nrepeat,        /* Number of time to do the transmission     */
                nrepeat_const=0,/* Set if we are using a constant nrepeat    */
                len,            /* Number of bytes to be transmitted         */
                inc=0,          /* Increment value                           */
                perturbation=DEFPERT, /* Perturbation value                  */
                pert,
                reset_connection,/* Reset the connection between trials      */
		debug_wait=0;	/* spin and wait for a debugger		     */
   
    ArgStruct   args;           /* Arguments for all the calls               */

    double      t, t0, t1, t2,  /* Time variables                            */
                tlast,          /* Time for the last transmission            */
                latency;        /* Network message latency                   */

    Data        bwdata[NSAMP];  /* Bandwidth curve data                      */

    int         integCheck=0;   /* Integrity check                           */

    /* Initialize vars that may change from default due to arguments */

    strcpy(s, "np.out");   /* Default output file */

    /* Let modules initialize related vars, and possibly call a library init
       function that requires argc and argv */


    Init(&args, &argc, &argv);   /* This will set args.tr and args.rcv */

    args.preburst = 0; /* Default to not bursting preposted receives */
    args.bidir = 0; /* Turn bi-directional mode off initially */
    args.cache = 1; /* Default to use cache */
    args.upper = end;
    args.host  = NULL;
    args.soffset=0; /* default to no offsets */
    args.roffset=0; 
    args.syncflag=0; /* use normal mpi_send */
    args.use_sdp=0; /* default to no SDP */
    args.port = DEFPORT; /* just in case the user doesn't set this. */

    /* Parse the arguments. See Usage for description */
    while ((c = getopt(argc, argv, "o:h:P:")) != -1)
    {
        switch(c)
        {
            case 'o': strcpy(s,optarg);
                      printf("Sending output to %s\n", s); fflush(stdout);
                      break;

            case 'h': args.tr = 1;       /* -h implies transmit node */
                      args.rcv = 0;
                      args.host = (char *)malloc(strlen(optarg)+1);
                      strcpy(args.host, optarg);
                      break;

	    case 'P': 
		      args.port = atoi(optarg);
		      break;

            default: 
                     PrintUsage(); 
                     exit(-12);
       }
   }

   Setup(&args);

   if( args.bidir && end > args.upper ) {
      end = args.upper;
      if( args.tr ) {
         printf("The upper limit is being set to %d Bytes\n", end);
         printf("due to socket buffer size limitations\n\n");
      }  
   }


   if( args.tr )                     /* Primary transmitter */
   {
       if ((out = fopen(s, "w")) == NULL)
       {
           fprintf(stderr,"Can't open %s for output\n", s);
           exit(1);
       }
   }
   else out = stdout;

   /* For simplicity's sake, even if the real test below will be done in
    * bi-directional mode, we still do the ping-pong one-way-at-a-time test
    * here to estimate the one-way latency. Unless it takes significantly
    * longer to send data in both directions at once than it does to send data
    * one way at a time, this shouldn't be too far off anyway.
    */
   t0 = When();
      for( n=0; n<100; n++) {
         if( args.tr) {
            SendData(&args);
            RecvData(&args);
            if( asyncReceive && n<99 )
               PrepareToReceive(&args);
         } else if( args.rcv) {
            RecvData(&args);
            if( asyncReceive && n<99 )
               PrepareToReceive(&args);
            SendData(&args);
         }
      }
   tlast = (When() - t0)/200;

   /* Sync up and Reset before freeing the buffers */
   
   /* Free the buffers and any other module-specific resources. */
   if(args.cache)
      FreeBuff(args.r_buff_orig, NULL);
   else
      FreeBuff(args.r_buff_orig, args.s_buff_orig);

}
