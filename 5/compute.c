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
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
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

#define TIME_LOOPS 30000

/** Macros */
/* Token macros taken from jsmn_test.c */
#define TOKEN_EQ(t, tok_start, tok_end, tok_type) \
	((t).start == tok_start \
	 && (t).end == tok_end  \
	 && (t).type == (tok_type))

#define TOKEN_STRCMP(js, t, s) \
	(strncmp(js+(t).start, s, (t).end - (t).start) == 0 \
	 && strlen(s) == (t).end - (t).start)

#define TOKEN_PRINT(t) \
	printf("start: %d, end: %d, type: %d, size: %d\n", \
			(t).start, (t).end, (t).type, (t).size)

#define TOKEN_VALID(t) \
	((t->start != -1) && (t->end != -1))

/** Global variables */
uint8_t cnt = 0;
uint64_t perfects[NUM_PERFECT];

/* Initialize socket for data sending */
int init_socket(uint16_t port)
{
	int sock_fd;
	struct hostent *local;
	struct sockaddr_in serv_addr;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if ((local = gethostbyname(HOME)) == NULL)
		perror("gethostbyname");

	serv_addr.sin_addr.s_addr = *((unsigned long *)local->h_addr);

	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		perror("connect");

	return (sock_fd);
}

/* Get authorization from server */
void handle_ack(int sock_fd, char *ack_str)
{
	char buf[MAX_LINE];
	if (write(sock_fd, ack_str, strlen(ack_str)) == -1)
		perror("write");
	if (read(sock_fd, buf, MAX_LINE) != 0)
		perror("read");


}

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
	char buf[MAX_LINE];
	jsmntok_t *cur;
	int idx;
	pid_t parent_pid = getppid();
	int sock_fd;
	struct sockaddr_in serv_addr;

	/* Connect to manage server */
	sock_fd = init_socket(CALC_PORT);

	char req[] = "{\"orig\": \"cmp_mon\", \"type\": \"ack\"}";
	handle_ack(req);

	/* TODO Interpret and set */

	kill(parent_pid, SIGQUIT);
	wait(NULL);
	exit(EXIT_FAILURE);
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
		break;
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
	char *buf = malloc(MAX_LINE * sizeof(char));
	jsmntok_t *cur;
	double flops = calc_perf();
	int idx = 0;
	uint64_t min;
	uint64_t max;
	jsmn_parser parser;
	char req[] = "{\"orig\": \"cmp\", \"type\": \"req\" \"flops\": %lf}";
	int sock_fd;
	jsmntok_t tokens[256];
	bool valid_range_given = false;
	char val[80];

	sprintf(val, req, flops);

	do {
		if (write(sock_fd, val, strlen(val)) == -1)
			perror("write");
		if (read(sock_fd, buf, MAX_LINE) != 0)
			perror("read");
		jsmn_init(&parser);
		if (jsmn_parse(&parser, buf, tokens, 256) != JSMN_SUCCESS)
			printf("jsmn_parse: Error\n");
		idx = 0;
		cur = &tokens[idx];

		while (TOKEN_VALID(cur)) {
			/* Grab min/max key, get then value from child */
				if (TOKEN_STRCMP(buf, *cur, "min")) {
					/* Key val pair means value next tok */
					++idx;
					cur = &tokens[idx];
					strncpy(val, (buf+cur->start),
							cur->end - cur->start);
					min = atol(val);
				}
				if (TOKEN_STRCMP(buf, *cur, "max")) {
					/* Key val pair means value next tok */
					++idx;
					cur = &tokens[idx];
					strncpy(val, (buf+cur->start),
							cur->end - cur->start);
					max = atol(val);
				}
			}
			++idx;
			cur = &tokens[idx];
		}

		valid_range_given = ((min >= 0) && (max > min));
		if (valid_range_given)
			compute_range (min, max);
	} while (valid_range_given);
}

/* Run subprograms */
int main(int argc, char **argv)
{
	double user;
	double sys;
	struct tms finish;
	int sock_fd = init_socket(CALC_PORT);
	struct tms start;

	printf("Performance (~FLOPS): %g\n", calc_perf());

	(void)times(&start);
	compute_range(1, UINT16_MAX);
	(void)times(&finish);

	sys = (double) (finish.tms_stime - start.tms_stime) / sysconf(_SC_CLK_TCK);
	user = (double) (finish.tms_utime - start.tms_utime) / sysconf(_SC_CLK_TCK);
	printf("Calc time for 1..65535: %gs (user), %gs (sys)\n", user, sys);

	for (int i = 0; i < cnt; ++i)
		printf(" %lu ", perfects[i]);


	return (EXIT_SUCCESS);
}
