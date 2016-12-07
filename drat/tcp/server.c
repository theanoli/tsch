/*--------- Basic client/server code from Programming Logic:
	http://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
...but heavily modified for ping-ponging
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

int main(){
	int welcomeSocket, newsockfd;
	char buffer[64];
	struct sockaddr_in serv_addr;
	struct sockaddr_storage cli_addr;
	socklen_t clilen;
	
	int pid;

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	welcomeSocket = socket( PF_INET, SOCK_STREAM, 0 );
	
	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(7891); // htons sets byte order
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* Set all bits of the padding field to 0 */
	memset( serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero );
	
	/*---- Bind the address struct to the socket ----*/
	bind( welcomeSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr) );
	
	/*---- Listen, with 256 max connection requests queued.
 	* Note the number of max cxns will be truncated; see listen(2). ----*/
	if ( listen ( welcomeSocket, 256 ) == 0 )
		printf( "Listening\n" );
	else
		printf( "Error\n" );
	
	/*---- Accept call creates a new socket for the incoming connection ----*/
	clilen = sizeof ( cli_addr );

	while (1) {
		newsockfd = accept( welcomeSocket, (struct sockaddr *) &cli_addr, &clilen );
		
		pid = fork();
		if ( pid < 0 ) {
			error ( "Forking error" );
		} 	
		if ( pid == 0 ) {
			// We're in the child; close sockfd
			close ( welcomeSocket )
		}

	/*---- Send message to the socket of the incoming connection ----*/
		strcpy ( buffer,"Hello World\n" );
		send ( newsockfd, buffer, 13, 0 );
		
		
	}
	return 0;
}
