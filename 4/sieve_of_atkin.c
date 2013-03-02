/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:34:58 PM
 *    Filename:  sieve_of_atkin.c
 *
 * Description:  Basic (non-threaded or forked) Sieve of Atkin
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define CHUNK_SIZE (sizeof (chunk_t) * 8)

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


uint64_t bindex (uint64_t idx) {
	return (idx / CHUNK_SIZE);
}

uint64_t boffset (uint64_t idx) {
	return (idx % CHUNK_SIZE);
}

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits) {
	struct bitset *bs = malloc (sizeof (*bs));

	assert (bs != NULL);
	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;
	/* Calloc is faster than malloc + memset, better performance. */
	bs->chunks = calloc (bs->n_chunks, sizeof (*bs->chunks));
	assert (bs->chunks != NULL);

	return (bs);
}

void bitset_free (struct bitset *bs) {
	if (bs != NULL) {
		if (bs->chunks != NULL) {
			free (bs->chunks);
		}
		free (bs);
	}
}

/* Bit operations */
void bit_set (struct bitset *bs, uint64_t idx) {
	assert (bs != NULL);
	bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
}

void bit_toggle (struct bitset *bs, uint64_t idx) {
	assert (bs != NULL);
	if (bit_get (bs, idx) != 0) {
		bit_clear (bs, idx);
	} else {
		bit_set (bs, idx);
	}
}

void bit_clear (struct bitset *bs, uint64_t idx) {
	assert (bs != NULL);
	bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
}

chunk_t bit_get (struct bitset *bs, uint64_t idx) {
	assert (bs != NULL);
	return (bs->chunks[bindex(idx)] & (1 << (boffset (idx))));
}

int main (int argc, char **argv)
{
	uint32_t cnt = 0;
	uint32_t k;
	uint64_t n;
	uint64_t n_sq;
	uint32_t x, y;
	uint64_t x_sq, y_sq;
	uint32_t limit = UINT32_MAX;
	struct bitset *bs = bitset_alloc (limit);
	uint32_t sqrt_lim = (uint32_t) sqrt ((double)limit);

	/* Put in candidate primes w/ odd num of reps */
	for (x = 1; x <= sqrt_lim; ++x) {
		x_sq = x * x;
		for (y = 1; y <= sqrt_lim; ++y) {
			y_sq = y * y;
			n = 4 * x_sq + y_sq;

			if ((n <= limit)
			&& (((n % 12) == 1) || (((n % 12) == 5)))) {
				bit_toggle(bs, n);
			}

			n -= x_sq;
			if ((n <= limit) && ((n % 12) == 7)) {
				bit_toggle(bs, n);
			}

			n -= 2 * y_sq;
			if ((x > y) && (n <= limit) && ((n % 12) == 11)) {
				bit_toggle(bs, n);
			}
		}
	}

	/* Eliminate composites */
	for (n = 5; n <= sqrt_lim; ++n) {
		if (bit_get (bs, n) != 0) {
			n_sq = n * n;
			for (k = n_sq; k < limit; k += n_sq) {
				bit_clear (bs, k);
			}
		}
	}

	/* Print primes */
	//printf ("2, 3");
	cnt = 2;
	for (n = 5; n < limit; ++n) {
		if (bit_get (bs, n)) {
			//printf (", %u", n);
			++cnt;
		}
	}
	printf("\ncount : %lu\n", (unsigned long)cnt);
}
