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

#define PKT_FIELD_LEN 10
#define TOK_NUM 64

/* Only 8 perfect numbers in 0..UINT64_MAX */
#define NUM_PERFECT 8

#define EMPTY_FLAG 0

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

#define TOKEN_GETSTR(js, t) (js + t.start)

#define TOKEN_PRINT(t) \
	printf("start: %d, end: %d, type: %d, size: %d\n", \
	       (t).start, (t).end, (t).type, (t).size)

#define TOKEN_STRCMP(js, t, s) \
	(strncmp(js+(t).start, s, (t).end - (t).start) == 0 \
	 && strlen(s) == (t).end - (t).start)

#define TOKEN_VALID(t) ((t.start >= 0) && (t.end > t.start))

#define TOKEN_SIZE(t) (t.end - t.start)


/** Type definitions */
struct pkt_data {
	char orig[PKT_FIELD_LEN];
	char type[PKT_FIELD_LEN];
	uint64_t min;
	uint64_t max;
	double flops;
};

/** Global variables */
uint8_t cnt = 0;
uint64_t perfects[NUM_PERFECT];
pid_t mon_pid;

/** Functions */
void sig_handle(int signo);
void init_sig_handle(void);

int main(int argc,char **argv);
void poll_server(int sock_fd);
void forkitor(void);
void monitor(void);
void compute_range(int sock_fd, uint64_t min,uint64_t max);
bool compute(uint64_t n);
double calc_perf(void);
void handle_ack(int sock_fd,char *ack_str);
int init_socket(uint16_t port);
struct pkt_data fill_packet(char *json);
void clear_packet(struct pkt_data *pkt);
void jsmn_err(int ret);

void jsmn_err(int ret)
{
	fprintf(stderr, "jsmn_parse: ");
	switch (ret) {
	case JSMN_ERROR_NOMEM:
		fprintf(stderr, "Not enough token were provided\n");
		break;
	case JSMN_ERROR_INVAL:
		fprintf(stderr, "Invalid character inside JSON string\n");
		break;
	case JSMN_ERROR_PART:
		fprintf(stderr, "The string is not a full JSON packet, "
		                "more bytes expected\n");
		break;
	case JSMN_SUCCESS:
		fprintf(stderr, "Success\n");
		break;
	/* Should NOT be reached */
	default:
		fprintf(stderr, "Unknown error occurred\n");
		break;
	}
}

void clear_packet(struct pkt_data *pkt)
{
	assert(pkt != NULL);

	pkt->orig[0] = '\0';
	pkt->type[0] = '\0';
	pkt->min = -1;
	pkt->max = -1;
	pkt->flops = -1.0f;
}

struct pkt_data fill_packet(char *json)
{
	uint8_t idx = 0;
	char *key_ptr;
	jsmn_parser parser;
	struct pkt_data pkt;
	int r;
	int sz;
	jsmntok_t tokens[TOK_NUM];
	char val[MAX_LINE];

	assert(json != NULL);
	clear_packet(&pkt);
	jsmn_init(&parser);
	r = jsmn_parse(&parser, json, tokens, TOK_NUM);
	if (r != JSMN_SUCCESS)
		jsmn_err(r);

	/* Last valid key is (last index) - 1 */
	while (TOKEN_VALID(tokens[idx + 1])) {
		/* get size of next tok. If key/val pair then val */
		sz = TOKEN_SIZE(tokens[idx + 1]);
		key_ptr = TOKEN_GETSTR(json, tokens[idx + 1]);
		strncpy(val, key_ptr, sz);
		val[sz] = '\0';

		if (TOKEN_STRCMP(json, tokens[idx], "orig")) {
			strcpy(pkt.orig, val);
		} else if (TOKEN_STRCMP(json, tokens[idx], "type")) {
			strcpy(pkt.type, val);
		} else if (TOKEN_STRCMP(json, tokens[idx], "min")) {
			pkt.min = (uint64_t) atoll(val);
		} else if (TOKEN_STRCMP(json, tokens[idx], "max")) {
			pkt.max = (uint64_t) atoll(val);
		} else if (TOKEN_STRCMP(json, tokens[idx], "flops")) {
			pkt.flops = atof(val);
		} else {
			/* Instead of jumping by two, we need to check next */
			idx -= 1;
		}
		/* Advance index and assign cur */
		idx += 2;
	}

	return (pkt);
}

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
	struct pkt_data pkt;
	int len;

	if (send(sock_fd, ack_str, strlen(ack_str), EMPTY_FLAG) == -1)
		perror("send");
	if ((len = recv(sock_fd, buf, MAX_LINE, EMPTY_FLAG)) == -1)
		perror("recv");

	buf[len] = '\0';
	pkt = fill_packet(buf);
	if ((strcmp(pkt.orig, "man") != 0) || (strcmp(pkt.type, "ack") != 0))
		printf("wrong packet read! wanted: ack, got: %s\n", pkt.type);
}


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

/* Compute numbers wrapper */
void compute_range(int sock_fd, uint64_t min, uint64_t max)
{
	uint64_t i;
	char pkt[MAX_LINE];

	for (i = min; i <= max; ++i) {
		if (compute(i)) {
			perfects[cnt] = i;
			/* Send packet over connection containing number */
			sprintf(pkt, "{\"orig\":\"cmp\", \"type\":\"add\", \"val\": %lu}\n", i);
			send(sock_fd, pkt, strlen(pkt), EMPTY_FLAG);
			printf("Found perfect number: %lu\n", (unsigned long)i);
			++cnt;
		}
	}
}

/* Monitor socket (forked) */
void monitor(void)
{
	char buf[MAX_LINE];
	int len;
	struct pkt_data pkt;
	pid_t parent_pid = getppid();
	int sock_fd;

	/* Connect to manage server */
	sock_fd = init_socket(CALC_PORT);
	handle_ack(sock_fd, "{\"orig\": \"cmp_mon\", \"type\": \"ack\"}\n");

	for ( ; ; ) {
		if ((len = recv(sock_fd, buf, MAX_LINE, EMPTY_FLAG)) == -1)
			perror("recv");
		buf[len] = '\0';
		pkt = fill_packet(buf);
		if ((strcmp(pkt.orig, "man") == 0) && (strcmp(pkt.type, "kill"))) {
			kill(parent_pid, SIGQUIT);
			wait(NULL);
			exit(EXIT_FAILURE);
		}
	}
}

/* Fork monitor */
void forkitor(void)
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
		mon_pid = child_pid;
		break;
	}
}

/* Handle server I/O for number computation */
void poll_server(int sock_fd)
{
	char buf[MAX_LINE];
	double flops = calc_perf();
	int len;
	struct pkt_data pkt;
	bool valid_range_given = false;
	char req[MAX_LINE];
	char ack[MAX_LINE];

	sprintf(ack, "{\"orig\": \"cmp\", \"type\": \"ack\", \"flops\": %lf}\n", flops);
	handle_ack(sock_fd, ack);

	sprintf(req, "{\"orig\": \"cmp\", \"type\": \"req\", \"flops\": %lf}\n", flops);
	do {
		if (send(sock_fd, req, strlen(req), EMPTY_FLAG) == -1)
			perror("send");
		if ((len = recv(sock_fd, buf, MAX_LINE, EMPTY_FLAG)) == -1)
			perror("recv");

		buf[len] = '\0';
		pkt = fill_packet(buf);

		if ((strcmp(pkt.orig, "man") == 0)
		  &&(strcmp(pkt.type, "rng") == 0)) {
			valid_range_given = (pkt.min >= 0) && (pkt.max > pkt.min);
			if (valid_range_given) {
				compute_range (sock_fd, pkt.min, pkt.max);
			}
		 }
	} while (valid_range_given);
}

void sig_handle(int signo)
{
	kill(mon_pid, signo);
	wait(NULL);
	exit(EXIT_FAILURE);
}

void init_sig_handle(void)
{
	struct sigaction handler;

	handler.sa_handler = sig_handle;
	handler.sa_flags = 0;
	sigemptyset(&(handler.sa_mask));
	sigaction(SIGHUP, &handler, NULL);
	sigaction(SIGINT, &handler, NULL);
	sigaction(SIGQUIT, &handler, NULL);
}

/* Run subprograms */
int main(int argc, char **argv)
{
	int sock_fd = init_socket(CALC_PORT);

	init_sig_handle();
	forkitor();
	poll_server(sock_fd);

	return (EXIT_SUCCESS);
}
