#include "TCPEchoServer.h"  /* TCP echo server includes */
#include <pthread.h>        /* for POSIX threads */
#include <mtcp_api.h>

void *ThreadMain(void *arg);            /* Main program of a thread */
void *RunServerThread ( void *arg ); 
mctx_t initializeServerThread ( int core );  

/* Structure of arguments to pass to client thread */
struct ThreadArgs
{
	int cli_sock;   // Socket descriptor for clt
	mctx_t mctx; 
};

// Declared out here to make accessible to threads
unsigned short serv_port;  // server port; gets set via args
static char *conf_file = NULL; 

int main(int argc, char *argv[])
{
	int ncores; 
	int i; 
	int ret;  
	pthread_t *app_thread;          // Thread IDs from pthread_create()
	int *core_id; 
	struct mtcp_conf mcfg; 
	
	if (argc != 4)     
	{
	    fprintf(stderr,"Usage:  %s <SERVER PORT> <NUM CONTEXTS> <CONFIG>\n", argv[0]);
	    exit(1);
	}
	
	serv_port = atoi(argv[1]);	// First arg: local port
	ncores = atoi(argv[2]); 
	conf_file = argv[3];

	mtcp_getconf ( &mcfg );
	mcfg.num_cores = 8;
	mtcp_setconf ( &mcfg ); 

	ret = mtcp_init ( conf_file );
	if ( ret ) {
		DieWithError ( "Failure to initialize mtcp" ); 
	}

	// TODO
	// mtcp_register_signal ( SIGINT, SignalHandler ); 

	app_thread = malloc ( ncores * sizeof ( pthread_t )  );
	core_id = malloc ( ncores * sizeof ( int ) );
	if ( ( app_thread == NULL ) || ( core_id == NULL ) ) {
		DieWithError ( "Error allocating thread array" ); 
	}

	// Create a thread per core; each will have a socket descriptor that
	// clients can connect to.
	for ( i = 0; i < ncores; i++ ) {
		core_id[i] = i; 
		if ( pthread_create ( &app_thread[i], NULL, RunServerThread, (void *)&core_id[i] ) != 0 ) {
			DieWithError ( "Couldn't create mTCP threads" );
		}
	}

	for ( ;; ) {

	}

	return 0; 
	// Will never get here
}

void *RunServerThread ( void *core_id  ) {
	struct ThreadArgs *threadArgs; // Pointer to argument structure for thread
	int listener;
	int cli_sock; // Socket descriptor for client connection
	pthread_t tid; 
	mctx_t mctx; 

	mctx = initializeServerThread( *(int *)core_id ); 
	if ( !mctx ) {
		DieWithError ( "Failed to create mtcp context" ); 
	}	

	listener = CreateTCPServerSocket ( mctx, serv_port );
	if ( listener < 0 ) {
		DieWithError ( "Failed to create mtcp context" ); 
	}	
	
	cli_sock = AcceptTCPConnection ( mctx, listener ); 

	for (;;)
	{
		/* Create separate memory for client argument */
		if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) 
		       == NULL)
		    DieWithError("malloc() failed");
		threadArgs->cli_sock = cli_sock;
		threadArgs->mctx = mctx; 
		
		/* Create client thread */
		if (pthread_create(&tid, NULL, ThreadMain, (void *) threadArgs) != 0)
		    DieWithError("pthread_create() failed");
		
		printf("with thread %ld\n", (long int) tid);
	}
	
	mtcp_destroy_context ( mctx );
	pthread_exit( NULL ); 
}

void *ThreadMain(void *threadArgs)
{
	mctx_t mctx; 
	int cli_sock; // Socket descriptor for client connection
	
	// Guarantees that thread resources are deallocated on return
	pthread_detach(pthread_self()); 
	
	// Extract socket file descriptor from argument
	cli_sock = (( struct ThreadArgs * ) threadArgs )->cli_sock;
	mctx = (( struct ThreadArgs * ) threadArgs )->mctx; 
	free(threadArgs);	// Deallocate memory for argument

	HandleTCPClient ( mctx, cli_sock );

	return (NULL);
}

mctx_t
initializeServerThread ( int core ) 
{
	mctx_t mctx; 

	mtcp_core_affinitize(core); 

	mctx = mtcp_create_context( core ); 
	
	return mctx; 
}

