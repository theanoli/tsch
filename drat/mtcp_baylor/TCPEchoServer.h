#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <mtcp_api.h>	/* for mTCP stuff */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(mctx_t mctx, int clntSocket);   /* TCP client handling function */
int CreateTCPServerSocket(mctx_t mctx, unsigned short port); /* Create TCP server socket */
int AcceptTCPConnection(mctx_t mctx, int servSock);  /* Accept TCP connection request */
