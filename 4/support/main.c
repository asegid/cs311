/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:34:58 PM
 *    Filename:  sieve_of_atkin.c
 *
 * Description:  Basic (non-threaded or forked) Sieve of Atkin
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "bst.h"

#define CHUNK_SIZE (sizeof (chunk_t) * 8)

/* Lookup table for squares of 0-9 for happy finding */
const int squares[] = {0, 1, 4, 9, 16, 25, 36, 49, 64, 81};

typedef uint8_t chunk_t;

struct bitset {
	chunk_t *chunks;
	uint64_t n_chunks;
};

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits);
void bitset_free (struct bitset *bs);
/* Bit operations */
void bit_set (struct bitset *bs, uint64_t idx);
void bit_toggle (struct bitset *bs, uint64_t idx);
void bit_clear (struct bitset *bs, uint64_t idx);
chunk_t bit_get (struct bitset *bs, uint64_t idx);


uint64_t bindex (uint64_t idx)
{
	return (idx / CHUNK_SIZE);
}

uint64_t boffset (uint64_t idx)
{
	return (idx % CHUNK_SIZE);
}

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits)
{
	struct bitset *bs = malloc (sizeof (*bs));

	assert (bs != NULL);
	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;
	/* Calloc is faster than malloc + memset, better performance. */
	bs->chunks = calloc (bs->n_chunks, sizeof (*bs->chunks));
	assert (bs->chunks != NULL);

	return (bs);
}

void bitset_free (struct bitset *bs)
{
	if (bs != NULL) {
		if (bs->chunks != NULL) {
			free (bs->chunks);
		}
		free (bs);
	}
}

/* Bit operations */
void bit_set (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);
	bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
}

void bit_toggle (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);
	if (bit_get (bs, idx) != 0) {
		bit_clear (bs, idx);
	} else {
		bit_set (bs, idx);
	}
}

void bit_clear (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);
	bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
}

chunk_t bit_get (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);
	return (bs->chunks[bindex(idx)] & (1 << (boffset (idx))));
}

/* Sieve functions */
void candidate_primes(uint32_t lim, uint32_t sqrt_lim, struct bitset *bs,
                      uint32_t min, uint32_t max)
{
	uint64_t n;
	uint32_t x, y;
	uint64_t x_sq, y_sq;

	/* Put in candidate primes w/ odd num of reps */
	for (x = min; x <= max; ++x) {
		x_sq = x * x;
		for (y = 1; y <= sqrt_lim; ++y) {
			y_sq = y * y;
			n = 4 * x_sq + y_sq;

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

void eliminate_composites(uint32_t lim, uint32_t sqrt_lim, struct bitset *bs,
                          uint32_t min, uint32_t max)
{
	uint32_t k;
	uint64_t n;
	uint64_t n_sq;

	/* Eliminate composites */
	for (n = min; n <= max; ++n) {
		if (bit_get (bs, n) != 0) {
			n_sq = n * n;
			for (k = n_sq; k < lim; k += n_sq) {
				bit_clear (bs, k);
			}
		}
	}
}

/* Happy functions */
bool is_happy(uint32_t num) {
	uint64_t ceil;
	uint32_t cur = num;
	bool happy = false;
	bool repeating = false;
	uint32_t rem = num;
	uint64_t sum = 1;
	struct BSTree *seen = newBSTree();

	while (!happy && !repeating) {
		uint64_t mod = 10;
		uint32_t modded;
		sum = 0;

		ceil = cur * 10;
		while (mod <= ceil) {
			modded = (cur % mod) / (mod / 10);
			sum += squares[modded];
			rem -= modded;
			mod *= 10;
		}

		if (sum == 1) {
			happy = true;
		} else {
			if (containsBSTree(seen, sum)) {
				repeating = true;
			} else {
				addBSTree (seen, sum);
				cur = sum;
			}
		}
	}

	deleteBSTree (seen);
	return (happy);
}

void determine_happies(uint32_t lim, uint32_t sqrt_lim, struct bitset *bs,
                       uint32_t min, uint32_t max)
{
	uint32_t n;

	/* Remove primes that aren't happy */
	for (n = min; n <= max; ++n) {
		if (bit_get (bs, n)) {
			if (!is_happy(n)) {
				bit_clear(bs, n);
			}
		}
	}
}

int main (int argc, char **argv)
{
	uint32_t cnt = 0;
	uint64_t n;
	uint32_t limit = UINT16_MAX;//UINT32_MAX;
	struct bitset *bs = bitset_alloc (limit);
	uint32_t sqrt_lim = (uint32_t) sqrt ((double)limit);

	struct timeval start, end;
	gettimeofday(&start, NULL);
	candidate_primes(limit, sqrt_lim, bs, 1, sqrt_lim);
	eliminate_composites(limit, sqrt_lim, bs, 5, sqrt_lim);
	gettimeofday(&end, NULL);

	printf("\ntotal prime time: %lis %lius\n", (end.tv_sec -  start.tv_sec),
	                             (end.tv_usec - start.tv_usec));

	determine_happies(limit, sqrt_lim, bs, 0, limit);

	/* Print primes */
	//printf ("2, 3");
	cnt = 2;
	for (n = 5; n < limit; ++n) {
		if (bit_get (bs, n)) {
			//printf (", %u", n);
			++cnt;
		}
	}
	printf ("number of happy primes <%lu: %lu\n",
	       (unsigned long)limit, (unsigned long)cnt);
	bitset_free (bs);
}
