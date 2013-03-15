/*
 *       Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *      Created:  03/06/2013 08:03:28 AM
 *     Filename:  compute.c
 *
 * Collaborators: Meghan Gorman (algorithm development)
 *   Description: Compute's job is to compute perfect numbers, by testing
 *                all numbers from its starting point onward, subject to
 *                constraints. There may be multiple copies running
 */

/** Includes */
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/** Local Includes */
#include "jsmn.h"

/** Literal constants */
#define LISTEN_Q 1024
#define MAX_LINE 4096
#define MAX_SOCK_ADDR 128

/* Only 8 perfect numbers in 0..UINT64_MAX */
#define NUM_PERFECT 8

#define HOME "localhost"
#define CALC_PORT 44479
#define CALC_PORT_STR "44479"
#define CTRL_PORT 44480
#define CTRL_PORT_STR "44480"

#define TIME_LOOPS 30000

/** Global variables */
uint8_t cnt = 0;
uint64_t perfects[NUM_PERFECT];

/** Functions */
/* Calculate performance (FLOPS...ish...) */
double calc_perf (void)
{
	uint32_t array[TIME_LOOPS];
	double elapsed;
	struct tms finish;
	uint32_t i;
	uint32_t j;
	struct tms start;

	(void)times(&start);
	/* Calculate IOPS */
	for (i = 0; i < TIME_LOOPS; ++i) {
		array[ i ] = 0;
		for (j = 0; j < TIME_LOOPS; ++j) {
			array[ i ] += j;
		}
	}
	(void)times(&finish);
	elapsed = ((double) (finish.tms_stime - start.tms_stime) +
	           (double) (finish.tms_utime - start.tms_utime) / 1000000.0f) /
	           sysconf(_SC_CLK_TCK);

	return ((TIME_LOOPS * TIME_LOOPS) / elapsed);
}

/* Determine if n is a perfect number */
bool compute (uint64_t n)
{
	uint32_t div;
	uint64_t i;

	/* Maximum proper factor is n / 2 (and 2). n is improper */
	bool is_perfect = false;
	uint64_t max_factor_idx = (n / 2) + 1;
	bool proper[ max_factor_idx ];
	uint32_t sq_n = (uint32_t) sqrt ((double) n);
	uint64_t sum = 0;

	/* Have to initialize variable length array manually */
	for (i = 0; i < max_factor_idx; ++i)
		proper[ i ] = false;

	/* 1 is special case: proper factor where partner is not proper */
	proper[ 1 ] = true;

	/* Of all pairs, the maximum smaller valued pair is sq_n */
	for (div = 2; div <= sq_n; ++div) {
		if ((n % div) == 0) {
			proper[ n / div ] = true;
			proper[ div ] = true;
		}
	}

	for (i = 1; i < max_factor_idx; ++i) {
		if (proper[ i ] == true) {
			sum += i;
		}
	}

	if (sum == n)
		is_perfect = true;

	return (is_perfect);
}

/* Monitor socket (forked) */
void monitor(void)
{
	pid_t parent_pid = getppid();
	bool received_kill_order = false;
	char recv_line [MAX_LINE];
	int sock_fd;
	struct sockaddr_in serv_addr;

	/* Connect to manage server */
	sock_fd = init_socket(CTRL_PORT);

	/* TODO HANDSHAKE? */

	/* Read is blocking, so only need to call once */
	if (read(sock_fd, recv_line, MAX_LINE) == -1)
		perror("read");

	/* TODO Interpret and set */

	/* Kill parent, then kill self */
	if (received_kill_order) {
		kill(parent_pid);
		wait(NULL);
		exit(EXIT_FAILURE);
	}
}

/* Fork monitor */
int forkitor(void)
{
	int child_pid = -1;

	switch(child_pid = fork()) {
	case -1:
		/* Error case */
		perror("fork");
		break;
	case 0:
		/* Child case */
		monitor();
		break;
	default:
		/* Parent case */
	}

	return (child_pid);
}

/* Compute numbers wrapper */
compute_range(uint64_t min, uint64_t max)
{
	uint64_t i;

	for (i = min; i <= max; ++i) {
		if (compute(i)) {
			perfects[cnt] = i;
			++cnt;
		}
	}
}

/* Handle server I/O for number computation */
void poll_server(void)
{
	bool valid_range_given = false;
	uint64_t min;
	uint64_t max;

	do {
		
		if (valid_range_given) {
			compute_range (min, max);
		}
	} while (valid_range_given);
}

/* Initialize socket for data sending */
int init_socket(uint16_t port)
{
	int sock_fd;
	struct hostent local;
	struct sockaddr_in serv_addr;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (gethostbyname(&local) == NULL)
		perror("gethostbyname");

	serv_addr.sin_addr.s_addr = *((unsigned long *)local->h_addr);

	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		perror("connect");

	return (sock_fd);
}

/* Run subprograms */
int main(int argc, char **argv)
{
	double elapsed;
	struct tms finish;
	int sock_fd = init_socket(CALC_PORT);
	struct tms start;

	printf("Performance (~FLOPS): %g\n", calc_perf());

	(void)times(&start);
	compute_range(1, 1000000);
	(void)times(&finish);
	elapsed = ((double) (finish.tms_stime - start.tms_stime) +
	           (double) (finish.tms_utime - start.tms_utime) / 1000000.0f) /
	           sysconf(_SC_CLK_TCK);
	
	printf("Calc time for 1..65535: %gs\n", elapsed);

	return (EXIT_SUCCESS);
}
