#include <stdio.h>      /* for printf() */
#include <sys/socket.h> /* for accept() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <mtcp_api.h>

void DieWithError(char *errorMessage);  /* Error handling function */

int AcceptTCPConnection(mctx_t mctx, int serv_sock)
{
    int cli_sock;                    /* Socket descriptor for client */
    struct sockaddr_in cli_addr; /* Client address */
    unsigned int clilen;            /* Length of client address data structure */

    /* Set the size of the in-out parameter */
    clilen = sizeof(cli_addr);
    
    /* Wait for a client to connect */
    if (cli_sock = mtcp_accept(mctx, serv_sock, (struct sockaddr *) &cli_addr, 
           &clilen) < 0)
        DieWithError("mtcp_accept() failed");
    
    /* cli_sock is connected to a client! */
    
    printf("Handling client %s\n", inet_ntoa(cli_addr.sin_addr));

    return cli_sock;
}

