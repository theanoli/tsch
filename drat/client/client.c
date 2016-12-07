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

#define PSIZE 64

// struct timespec timestamp_ns (void)
// {
//     struct timespec spec;
// 
//     clock_gettime(CLOCK_REALTIME, &spec);
//     return spec; 
// }

char *generate_randstring ( int len ) 
{
	char *str; 
	int i; 

	char *charset = "abcdefghijklmnopqrstuvwxyz"; 

	str = malloc ( 64 * sizeof ( char ) ); 
	for ( i = 0; i < (PSIZE - 1); i++ ) {
		str[i] = rand() % (strlen ( charset )); 
	}

	str[PSIZE - 1] = '\0'; 
	return str; 
}

void send_and_receive ( int sockfd ) 
{
	int n; 
	char *buf;
//	struct timespec start, end; 

	buf = malloc ( 64 * sizeof ( char ) );
	if ( buf == NULL ) {
		error ( "Malloc error" );
	}

//	clock_gettime(CLOCK_REALTIME, &start); 

	n = write ( sockfd, buf, PSIZE ); 
	if ( n < 0 ) {
		error ( "Error writing to socket..." );
	}

	// Eh do we care? 
	memset ( buf, '\0', 64 ); 

	n = read( sockfd, buf, PSIZE ); 
	if ( n < 0 ) {
		error ( "Error reading from socket..." ); 
	}
	printf ( "%s\n", buf ); 

//	clock_gettime(CLOCK_REALTIME, &end); 
}

int main(){
	int clientSocket;
	char buffer[64];
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	
	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
	
	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	serverAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(8080);
	/* Set IP address to localhost */
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	/*---- Connect the socket to the server using the address struct ----*/
	addr_size = sizeof serverAddr;
	connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
	
	/*---- Read the message from the server into the buffer ----*/
	send_and_receive( clientSocket ); 	
	
	/*---- Print the received message ----*/
	printf("Data received at %s: %s", timestamp_ns(), buffer);   
	
	return 0;
}
