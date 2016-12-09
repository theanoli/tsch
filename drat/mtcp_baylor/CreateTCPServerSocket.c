#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */
#include <mtcp_api.h>

#define MAXPENDING 1024    /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage);  /* Error handling function */

int CreateTCPServerSocket ( mctx_t mctx, unsigned short port )
{
	int listener; 
	int ret; 
	struct sockaddr_in serv_addr;
	
	/* Create socket for incoming connections */
	listener = mtcp_socket( mctx, PF_INET, SOCK_STREAM, 0 );
	if ( listener < 0 ) {
		DieWithError("socket() failed");
	}

	// ret = mtcp_setsock_nonblock ( mctx, listener ); 
	// if ( ret < 0 ) {
//		DieWithError( "Failed to set nonblocking mode." );
//	}

	/* Construct local address structure */
	memset(&serv_addr, 0, sizeof(serv_addr));   /* Zero out structure */
	serv_addr.sin_family = AF_INET;                /* Internet address family */
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	serv_addr.sin_port = htons(port);              /* Local port */
	
	/* Bind to the local address */
	ret = mtcp_bind(mctx, listener, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); 
	if ( ret < 0 ) {
		DieWithError("bind() failed");
	}	

	/* Mark the socket so it will listen for incoming connections */
	ret = mtcp_listen(mctx, listener, MAXPENDING); 
	if ( ret < 0 )
		DieWithError("listen() failed");
	
	return listener;
}

