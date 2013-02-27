/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:34:58 PM
 *    Filename:  sieve_of_atkin.c
 *
 * Description:  Basic (non-threaded or forked) Sieve of Atkin
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "bitset.h"
#include "sieve.h"

/* Optimize for quad core processors */
#define THREAD_CNT 4

void print (struct bitset *bs, uint32_t limit) {
	/* Print primes */
	printf ("2, 3");
	uint32_t cnt = 2;
	for (uint32_t n = 5; n < limit; ++n) {
		if (bit_get (bs, n)) {
			printf (", %u", n);
			++cnt;
		}
	}
	printf("\ncount : %u\n", cnt);
}

int main (int argc, char **argv)
{
	uint32_t limit = 100;//UINT32_MAX;
	uint32_t sqrt_lim = sqrt (limit);

	struct bitset *bs = bitset_alloc (limit);

	// Read chapters on shared memory and threading for this segment

	//// SOCKET NOTES
	// based on a file (use an fd)
	// AF_INET, SOCK_STREAM, 0 (only 1 protocol for domain and type)
	/*int i;
	int listen_fd;
	int sock_fd;
	struct sockaddr_in cli_addr;
	struct sockaddr_in serv_addr;

	listen_fd = socket (AF_INET, SOCK_STREAM, 0);
	// Need to zero out. Equivalent to memset(&serv_addr, 0, sizeof(serv_addr));
	bzero(&serv_addr, sizeof(serv_addr)); // LEGACY
	serv_addr.sin_family = AF_INET;
	// htonl changes endianness. Most CPUs are bi-endianness, but the
	// network is big endian (x86 is little endian). Conv to network byte order.
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_PORT);

	bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof (serv_addr));
	// LISTENQ is the number of connections that the system will accept
	listen(listen_fd, LISTENQ);
	*///// END NOTES

	// Share a condition variable so that we know when all candidates are
	// found so we can eliminate composites
	/* Put in candidate primes w/ odd num of reps */
	find_candidates (bs, limit, sqrt_lim, 1, sqrt_lim);

	/* Eliminate composites */
	elim_composites (bs, limit, 5, sqrt_lim);

	/* Print results */
	print (bs, limit);

	/* Free bitset */
	bitset_free (bs);
}
