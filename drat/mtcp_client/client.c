/*--------- Basic client/server code from Programming Logic:
	http://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
...but heavily modified for ping-ponging
*/

#include <mtcp_api.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
//#include <types.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#define PSIZE 32

struct Threadargs {
	char *hostname;
	int portno; 
	char *fname; 
	int exp_duration;
};

extern int errno;
int clientSocket;
FILE *fd; 

struct timespec diff ( struct timespec start, struct timespec end ); 
void *runClientThread ( void *args ); 

struct timespec 
timestamp (void)
{
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec; 
}

void
recv_packet ( mctx_t mctx, int sockfd ) 
{
	int n;
	char *str;
	char *str_copy;
	char *secs;
	char *ns; 
	char *rtt;
	struct timespec parsed_start, end, tdiff;

	str = malloc ( PSIZE * sizeof ( char ) ); 
	if ( str == NULL ) {
		perror ( "Malloc error" ); 
		return;
	}

	memset ( str, '\0', PSIZE ); 

	n = mtcp_read ( mctx, sockfd, str, PSIZE ); 
	while ( n <= 0 ) {
		printf ( "Read less than 1 byte...\n" );
		if ( n < 0 ) {
			perror ( "Error reading from socket..." ); 
			free ( str );
			return; 
		}
		n = mtcp_read ( mctx, sockfd, str, PSIZE ); 
	}

	end = timestamp (); 	

	str_copy = malloc ( PSIZE * sizeof ( char ) );
	if ( str_copy == NULL ) {
		perror ( "Malloc error" );
		free ( str );
		return;
	}
	strncpy ( str_copy, str, PSIZE );

	secs = strtok ( str, "." );
	ns = strtok ( NULL, "." ); 

	if ( ( secs == NULL ) || ( ns == NULL ) ) {
		perror ( "STRTOK returned NULL prematurely" );
		free ( str_copy ); 
		free ( str );
		return;
	}

	parsed_start.tv_sec = atoi ( secs ); 
	parsed_start.tv_nsec = atoi ( ns ); 

	tdiff = diff ( parsed_start, end ); 

	rtt = malloc ( PSIZE * 2 * sizeof ( char ) ); 
	if ( rtt == NULL ) {
		free ( str );
		free ( str_copy );
		perror ( "Malloc error" ); 
		return;
	}
	memset ( rtt, '\0', PSIZE * 2 ); 

	snprintf ( rtt, PSIZE, "%lld,%.9ld", 
		(long long) end.tv_sec, end.tv_nsec ); 
	snprintf ( rtt + strlen ( rtt ), PSIZE, ",%lld,%.9ld\n", 
		(long long) tdiff.tv_sec, tdiff.tv_nsec );

	if ( fprintf ( fd, "%s", rtt ) <= 0 ) {
		printf ( "Nothing written!\n" );
	}
	
	free ( rtt );  // current time timestamp + rtt
	free ( str_copy ); 
	free ( str );
}

void 
send_packet ( mctx_t mctx, int sockfd ) 
{
	int n; 
	char *str;
	struct timespec start; 

	printf ( "Writing\n" );
	str = malloc ( PSIZE * sizeof ( char ) ); 
	if ( str == NULL ) {
		perror ( "Malloc error" ); 
		return;
	}

	memset ( str, '\0', PSIZE ); 

	start = timestamp (); 
	snprintf ( str, PSIZE, "%lld.%.9ld", 
		(long long) start.tv_sec, start.tv_nsec ); 
	n = mtcp_write ( mctx, sockfd, str, PSIZE ); 
	if ( n < 0 ) {
		perror ( "Error writing to socket..." );
		free ( str ); 
		return; 
	}

	free ( str );
}

struct timespec
diff ( struct timespec start, struct timespec end )
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

mctx_t
createServerContext ( int core )
{
	mctx_t mctx; 

	mtcp_core_affinitize(core); 

	mctx = mtcp_create_context( core ); 
	
	return mctx; 
}
 
int 
main ( int argc, char **argv )
{
	struct mtcp_conf mcfg;
	int ret;

	pthread_t tid;
	struct Threadargs threadargs; 

	if ( argc < 5 ) {
		perror ( "You need arguments: <hostname> <portno> <fname> <exp_duration (s)>" ); 
		return 1; 
	}

	threadargs.hostname = argv[1];
	threadargs.portno = atoi ( argv[2] );
	threadargs.fname = argv[3]; 
	threadargs.exp_duration = atoi ( argv[4] ); 

	// Setup mtcp stuff
	mtcp_getconf ( &mcfg );
	mcfg.num_cores = 1;
	mtcp_setconf ( &mcfg ); 

	ret = mtcp_init ( "epserver.conf" );
	if ( ret ) {
		perror ( "Failure to initialize mtcp" ); 
		return 1;
	}

	if ( pthread_create ( &tid, NULL, runClientThread, 
		(void *)&threadargs) ) {
		perror ( "Failed to create client thread\n" );
		return 1;
	}

	pthread_join ( tid, NULL ); 

	fclose ( fd ); 	
	return 0;
}

void *
runClientThread ( void *targs ) 
{
	struct hostent *server;
	struct sockaddr_in serverAddr;
	time_t start; 
	int ret;

	struct Threadargs *args;  
	mctx_t mctx; 	

	args = (struct Threadargs *) targs;

	// For now use only the first core; later will include -N option to handle more?
	mctx = createServerContext ( 0 ); 
	if ( !mctx ) {
		perror ( "Failed to create mtcp context" ); 
		return NULL;
	}	

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	clientSocket = mtcp_socket( mctx, AF_INET, SOCK_STREAM, 0);

	printf ( "Your port number is %d, writefile %s, duration %d, host %s\n", 
				args->portno, 
				args->fname, 
				args->exp_duration, 
				args->hostname ); 

	server = gethostbyname(args->hostname);
    	if (server == NULL) {
        	perror ("ERROR, no such host\n");
        	return NULL;
    	}

	

	/*---- Configure settings of the remote address struct ----*/
	/* Address family = Internet */
	serverAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(args->portno);
	/* Set IP address to hostname */
	serverAddr.sin_addr.s_addr = inet_addr ( args->hostname );
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	/*---- Connect the socket to the server using the address struct ----*/
	printf ( "Connecting...\n" ); 
	ret = mtcp_connect(mctx, clientSocket, (struct sockaddr *) &serverAddr, 
			(socklen_t)sizeof ( serverAddr ));
	
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror("mtcp_connect");
			mtcp_close(mctx, clientSocket);
			return NULL;
		}
	}

	printf ( "Getting ready to send packets...\n" ); 

	fd = fopen ( args->fname, "a" ); 
	if ( fd == NULL ) {
		perror ( "Couldn't create file descriptor" ); 
		return NULL; 	 
	}
	
	printf ( "Starting timer...\n" );
	start = time ( 0 ); 
	while ( (time ( 0 ) - start) < args->exp_duration ) {
		send_packet ( mctx, clientSocket ); 
		recv_packet ( mctx, clientSocket ); 
	}
	printf ( "Done!\n" ); 

	mtcp_destroy_context ( mctx );
	return NULL;
}
