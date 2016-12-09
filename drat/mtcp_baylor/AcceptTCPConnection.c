#include <stdio.h>      /* for printf() */
#include <sys/socket.h> /* for accept() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <mtcp_api.h>

void DieWithError(char *errorMessage);  /* Error handling function */

int AcceptTCPConnection(mctx_t mctx, int serv_sock)
{
    int cli_sock;                    /* Socket descriptor for client */
    
    /* Wait for a client to connect */
    if ((cli_sock = mtcp_accept ( mctx, serv_sock, NULL, NULL ) ) < 0)
        DieWithError("mtcp_accept() failed");
    
    printf ( "Handling a client\n" );

    return cli_sock;
}

