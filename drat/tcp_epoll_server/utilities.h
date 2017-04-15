#include <time.h>
#include <fcntl.h>

struct timespec 
timestamp (void)
{
    struct timespec spec;

    clock_gettime (CLOCK_MONOTONIC, &spec);
    return spec; 
}
/*----------------------------------------------------------------------------*/
struct timespec
diff ( struct timespec start, struct timespec end )
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000 + end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}
/*----------------------------------------------------------------------------*/
int
setsock_nonblock (int sockfd)
{
	// TODO why do we need to do this? 
	int flags, s;
	
	flags = fcntl (sockfd, F_GETFL, 0);
	if (flags < 0) {
		perror ("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl (sockfd, F_SETFL, flags);
	if (s < 0) {
		perror ("fcntl");
		return -1; 
	}
	
	return 0;
}

