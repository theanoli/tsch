#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <mtcp_api.h>

#define RCVBUFSIZE 64   /* Size of receive buffer */

void DieWithError(char *errorMessage);  /* Error handling function */

void HandleTCPClient( mctx_t mctx, int cli_sock )
{
	char buf[RCVBUFSIZE];	// Buffer for echo string
	int recvMsgSize;		// Size of received message

    	/* Receive message from client */
	if ((recvMsgSize = mtcp_recv( mctx, cli_sock, buf, RCVBUFSIZE, 0 )) < 0)
        	DieWithError("recv() failed");

	/* Send received string and receive again until end of transmission */
	while (recvMsgSize > 0)      /* zero indicates end of transmission */
	{
		/* Echo message back to client */
		if ( mtcp_write ( mctx, cli_sock, buf, recvMsgSize, 0 ) != recvMsgSize )
			DieWithError("send() failed");
		
		/* See if there is more data to receive */
		if ( ( recvMsgSize = recv ( mctx, cli_sock, buf, RCVBUFSIZE, 0 )) < 0 )
			DieWithError("recv() failed");
	}
	
	close(cli_sock);    /* Close client socket */
}

