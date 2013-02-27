/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:34:58 PM
 *    Filename:  TODO
 *
 * Description:  Basic (non-threaded or forked) Sieve of Atkin
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define CHUNK_SIZE (sizeof (chunk_t) * 8)

/* Smallest data type is preferable over size == processor bus size */
typedef uint8_t chunk_t;

struct bitset {
	chunk_t *chunks;
	uint64_t n_chunks;
};

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits);
void bitset_free (struct bitset *bs);
uint64_t bindex (uint64_t idx);
uint64_t boffset (uint64_t idx);
void bit_set (struct bitset *bs, uint64_t idx);
void bit_toggle (struct bitset *bs, uint64_t idx);
void bit_clear (struct bitset *bs, uint64_t idx);
uint64_t bit_get (struct bitset *bs, uint64_t idx);
void find_candidates(struct bitset *bs, uint32_t lim, uint32_t sqrt_lim,
                     uint32_t min, uint32_t max);
void elim_composites(struct bitset *bs, uint32_t lim,
                     uint32_t min, uint32_t max);

uint64_t bindex (uint64_t idx) {
	return (idx / CHUNK_SIZE);
}

uint64_t boffset (uint64_t idx) {
	return (idx % CHUNK_SIZE);
}

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits) {
	struct bitset *bs = malloc (sizeof (*bs));
	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;
	/* Calloc is faster than malloc + memset, better performance. */
	bs->chunks = calloc (bs->n_chunks, sizeof (*bs->chunks));

	return (bs);
}

void bitset_free (struct bitset *bs) {
	free (bs->chunks);
	free (bs);
}

/* Bit operations */
void bit_set (struct bitset *bs, uint64_t idx) {
	bs->chunks[bindex(idx)] |= (1 << (boffset (idx)));
}

void bit_toggle (struct bitset *bs, uint64_t idx) {
	if (bit_get (bs, idx)) {
		bit_clear (bs, idx);
	} else {
		bit_set (bs, idx);
	}
}

void bit_clear (struct bitset *bs, uint64_t idx) {
	bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
}

uint64_t bit_get (struct bitset *bs, uint64_t idx) {
	return (bs->chunks[bindex(idx)] & (1 << (boffset (idx))));
}

void find_candidates(struct bitset *bs, uint32_t lim, uint32_t sqrt_lim,
                     uint32_t min, uint32_t max) {
	/* Put in candidate primes w/ odd num of reps */
	for (uint32_t x = min; x <= max; ++x) {
		uint64_t x_sq = x * x;
		for (int y = 1; y <= sqrt_lim; ++y) {
			uint64_t y_sq = y * y;
			uint64_t n = 4 * x_sq + y_sq;

			if ((n <= lim)
			&& (((n % 12) == 1) || (((n % 12) == 5)))) {
				bit_toggle(bs, n);
			}

			n -= x_sq;
			if ((n <= lim) && ((n % 12) == 7)) {
				bit_toggle(bs, n);
			}

			n -= 2 * y_sq;
			if ((x > y) && (n <= lim) && ((n % 12) == 11)) {
				bit_toggle(bs, n);
			}
		}
	}
}

void elim_composites (struct bitset *bs, uint32_t lim,
                      uint32_t min, uint32_t max) {
	/* Eliminate composites */
	for (uint32_t n = min; n <= max; ++n) {
		if (bit_get (bs, n)) {
			uint32_t n_sq = n * n;
			for (uint32_t k = n_sq; k < lim; k += n_sq) {
				bit_clear (bs, k);
			}
		}
	}
}

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
	uint32_t limit = UINT32_MAX;
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
