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

extern int errno;
int clientSocket;
FILE *fd; 

struct timespec diff ( struct timespec start, struct timespec end ); 

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
initializeServerThread ( int core ) 
{
	mctx_t mctx; 

	mtcp_core_affinitize(core); 

	mctx = mtcp_create_context( core ); 
	
	return mctx; 
}
 
int 
main ( int argc, char **argv )
{
	char *hostname;
	//char ip[100];
	
	int portno; 
	char *fname; 
	int exp_duration;

	struct hostent *server;
	struct sockaddr_in serverAddr;
	struct sockaddr_in localAddr;
	time_t start; 

	struct mtcp_conf mcfg;
	mctx_t mctx; 
	int ret;

	// Setup mtcp stuff
	mtcp_getconf ( &mcfg );
	mcfg.num_cores = 8;
	mtcp_setconf ( &mcfg ); 

	ret = mtcp_init ( "epserver.conf" );
	if ( ret ) {
		perror ( "Failure to initialize mtcp" ); 
		return 1;
	}

	mctx = initializeServerThread( 0 ); 
	if ( !mctx ) {
		perror ( "Failed to create mtcp context" ); 
		return 1;
	}	


	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	clientSocket = mtcp_socket( mctx, AF_INET, SOCK_STREAM, 0);

	if ( argc < 5 ) {
		perror ( "You need arguments: <hostname> <portno> <fname> <exp_duration (s)>" ); 
		return 1; 
	}
	hostname = argv[1];
	portno = atoi ( argv[2] );
	fname = argv[3]; 
	exp_duration = atoi ( argv[4] ); 

	printf ( "Your port number is %d, writefile %s, duration %d, host %s\n", 
				portno, fname, exp_duration, hostname ); 

	server = gethostbyname(hostname);
    	if (server == NULL) {
        	perror ("ERROR, no such host\n");
        	return 1;
    	}

	//hostname_to_ip(hostname, ip);
	//printf ( "%s resolved to %s\n", hostname, ip );
 	
	/*---- Configure settings of the local address struct ----*/
	/* Address family = Internet */
	localAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	localAddr.sin_port = htons(portno);
	/* Set IP address to localhost */
	localAddr.sin_addr.s_addr = inet_addr("10.0.0.4");
	// memcpy ( (char *)&serverAddr.sin_addr.s_addr, 
	//	(char *)server->h_addr, server->h_length );
	/* Set all bits of the padding field to 0 */
	memset(localAddr.sin_zero, '\0', sizeof localAddr.sin_zero);  
	// mtcp_bind ( mctx, clientSocket, (struct sockaddr *)&localAddr, sizeof ( localAddr ) );


	serverAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(portno);
	/* Set IP address to localhost */
	//memcpy ( (char *)&serverAddr.sin_addr.s_addr, 
	//	(char *)server->h_addr, server->h_length );
	serverAddr.sin_addr.s_addr = inet_addr ( "10.0.0.4" );
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	/*---- Connect the socket to the server using the address struct ----*/
	ret = mtcp_connect(mctx, clientSocket, (struct sockaddr *) &serverAddr, 
			(socklen_t)sizeof ( serverAddr ));

	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror("mtcp_connect");
			mtcp_close(mctx, clientSocket);
			return -1;
		}
	}

	fd = fopen ( fname, "a" ); 
	if ( fd == NULL ) {
		perror ( "Couldn't create file descriptor" ); 
		return 1; 	 
	}

	start = time ( 0 ); 
	while ( (time ( 0 ) - start) < exp_duration ) {
		send_packet ( mctx, clientSocket ); 
		recv_packet ( mctx, clientSocket ); 
	}

	mtcp_destroy_context ( mctx );
	fclose ( fd ); 	
	return 0;
}

