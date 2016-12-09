/*--------- Basic client/server code from Programming Logic:
	http://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
...but heavily modified for ping-ponging
*/

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

#define PSIZE 64

extern int errno;
int clientSocket;

void *send_packets ( void *threadno );

struct timespec 
timestamp (void)
{
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec; 
}

char *
generate_randstring ( int len ) 
{
	char *str; 
	int i; 

	printf ( "Your string is being generated...\n" ); 

	char *charset = "abcdefghijklmnopqrstuvwxyz"; 

	str = malloc ( PSIZE * sizeof ( char ) ); 
	for ( i = 0; i < (PSIZE - 1); i++ ) {
		str[i] = charset[rand() % (strlen ( charset ))]; 
	}

	str[PSIZE - 1] = '\0'; 
	printf ( "Your string is %s\n", str ); 

	return str; 
}

void 
send_and_receive ( int sockfd, int threadno ) 
{
	int n; 
	char *str;
	struct timespec start, parsed_start, end, diff; 
	char *secs;
	char *ns; 

	secs = malloc ( 11 * sizeof ( char ) );
	ns = malloc ( 10 * sizeof ( char ) ); 
	if ( ( secs == NULL ) || ( ns == NULL ) ) {
		perror ( "Malloc error" ); 
		return;
	}

	str = malloc ( PSIZE * sizeof ( char ) ); 
	if ( str == NULL ) {
		perror ( "Malloc error" ); 
		return;
	}

	start = timestamp (); 
	snprintf ( str, PSIZE, "%lld.%.9ld", threadno, 
		(long long) start.tv_sec, start.tv_nsec ); 
	n = write ( sockfd, str, PSIZE ); 
	if ( n < 0 ) {
		perror ( "Error writing to socket..." );
		return; 
	}

	memset ( str, '\0', PSIZE ); 

	n = read( sockfd, str, PSIZE ); 
	if ( n < 0 ) {
		perror ( "Error reading from socket..." ); 
		return; 
	}

	end = timestamp (); 	
	strncpy ( secs, str, 10 ); 
	strncpy ( ns + 10 * sizeof ( char ), str, 9 );	

	parsed_start->tv_sec = atoi ( secs ); 
	parsed_start->tv_nsec = atoi ( ns ); 

	diff = diff (  ); 

	printf ( "%s\n", str ); 

	free ( str ); 
}

timespec
diff ( timespec start, timespec end )
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

int 
main ( int argc, char **argv )
{
	int portno; 
	int nthreads;
	int i; 
	pthread_t *tid;
	int *threadnum; 
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	
	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);

	if ( argc < 3 ) {
		perror ( "You need two arguments: <portno> <nthreads>" ); 
		return 1; 
	}
	portno = atoi ( argv[1] );
	nthreads = atoi ( argv[2] ); 

	printf ( "Your port number is %d, %d threads\n", portno, nthreads ); 
	
	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	serverAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(portno);
	/* Set IP address to localhost */
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	/*---- Connect the socket to the server using the address struct ----*/
	addr_size = sizeof serverAddr;
	connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

	tid = malloc ( nthreads * sizeof ( int ) ); 
	threadnum = malloc ( nthreads * sizeof ( int ) ); 
	if (( tid == NULL ) || ( threadnum == NULL )) {
		perror ( "Error allocating arrays for thread IDs" );
		return 1;
	}

	/*---- Spawn nthreads threads and have each flood cxn with packets ----*/
	for ( i = 0; i < nthreads; i++ ) {
		threadnum[i] = i; 
		if ( pthread_create ( &tid[i], NULL, send_packets, &threadnum[i] ) ) {
			perror ( "Error creating thread" ); 
			return 1; 
		} 		
	}

	while ( 1 ) {
		// spin forever while the threads run...
	}
	// will never get here
	return 0;
}

void *
send_packets ( void *threadno ) 
{
	pthread_detach ( pthread_self () ); 

	while ( 1 ) {
		send_and_receive ( clientSocket, *(int *)threadno ); 
	}

	return NULL; 
}
