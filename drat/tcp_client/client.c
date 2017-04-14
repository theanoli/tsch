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
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#define PSIZE 32

extern int errno;
int clientSocket;
int do_write;
FILE *fd; 

struct timespec diff ( struct timespec start, struct timespec end ); 

struct timespec 
timestamp (void)
{
	// Time since machine started up
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec; 
}

void
recv_packet ( int sockfd ) 
{
	int n, t;
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

	t = 0;  // bytes received so far
	while ( 1 ) {
		n = read ( sockfd, str + t, PSIZE ); 
		
		if ( n < 0 ) {
 			perror ( "Error reading from socket..." ); 
 			free ( str );
 			return; 
		}

		t += n;
		
		if ( n == 0 || t == PSIZE ) {
			// no more bytes
			break;
		}
		printf ( "Multiple rounds needed for read\n" ); 
	}

	if ( do_write ) {
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
}

void 
send_packet ( int sockfd ) 
{
	int n; 
	char *str;
	struct timespec start; 

	str = malloc ( PSIZE * sizeof ( char ) ); 
	if ( str == NULL ) {
		perror ( "Malloc error" ); 
		return;
	}

	memset ( str, '\0', PSIZE ); 

	start = timestamp (); 
	snprintf ( str, PSIZE, "%lld.%.9ld", 
		(long long) start.tv_sec, start.tv_nsec ); 
	n = write ( sockfd, str, PSIZE ); 
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

int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}
 
int 
main ( int argc, char **argv )
{
	char *hostip;
	
	int ret; 
	int sleep;
	char c;
	int portno; 
	char *fname;
	int exp_duration;
	char fpath[140] = "../results/";

	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	int packet_counter; 
	time_t start; 

	sleep = 0;
	do_write = 0;
	fname = NULL; 


	char *usage = "usage: ./client -h hostip -p portno -o outfile -d experiment duration -w, where -w means 'do write'. If -w is not used, do not need an outfile.";
	opterr = 0; 
	while (( c = getopt ( argc, argv, "h:p:o:d:s:w" ) ) != -1 )
		switch ( c ) {
			case 'h':
				// host IP
				hostip = optarg;
				break;
			case 'p':
				// portno
				portno = atoi ( optarg );
				break;
			case 'o':
				// output file
				fname = optarg;
				break;
			case 'd':
				// experiment duration
				exp_duration = atoi ( optarg );
				break;
			case 'w':
				// do write?
				do_write = 1;
				break;
			case 's': 
				// sleep duration (usec)
				sleep = atoi ( optarg );
				break;
			case '?':
				fprintf ( stderr, "Unknown option char \
						`\\x%x'.\n", optopt );
				fprintf ( stderr, "%s\n", usage );
				return 1;
			default:
				return 1;
		}

	if (do_write && (fname == NULL)) {
		perror (usage);
		perror ("No file name given!");
		return 1;
	} else if (do_write && strlen (fname) > 128) {
		perror ("File name must be less than 128 bytes!");
		return 1;
	}

	printf ("Your port number is %d, duration %d, host %s\n", 
				portno, exp_duration, hostip); 


	clientSocket = socket (AF_INET, SOCK_STREAM, 0);

	printf ("Connecting to %s\n", hostip); 	

	/*---- Configure settings of the server address struct ----*/
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (portno);
	serverAddr.sin_addr.s_addr = inet_addr (hostip);
	memset (serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
	
	/*---- Connect the socket to the server using the address struct ----*/
	addr_size = sizeof serverAddr;
	ret = connect (clientSocket, (struct sockaddr *) &serverAddr, addr_size);
	
	if (ret < 0) {
		printf ("Connect failed!\n");
		exit (1);
	}
	
	/*---- Open a file for infodump if needed, then run ----*/
	if ( do_write ) {
		strcat ( fpath, fname ); 
		fd = fopen ( fpath, "w" ); 
		if ( fd == NULL ) {
			perror ( "Couldn't create file descriptor" ); 
			return 1; 	 
		}

	}

	printf ( "Connection established; beginning send/receive of packets.\n" );

	packet_counter = 0;
	start = time ( 0 ); 
	while ( (time ( 0 ) - start) < exp_duration ) {
		send_packet ( clientSocket ); 
		recv_packet ( clientSocket ); 
		packet_counter++; 
		usleep ( sleep );
	}

	if ( do_write ) {
		fclose ( fd ); 	
	}

	printf ( "Sent/received %d packets over %ds\n", 
		packet_counter, exp_duration );
	return 0;
}

