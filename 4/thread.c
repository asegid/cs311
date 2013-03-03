/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:34:58 PM
 *    Filename:  thread.c
 *
 * Description:  Threaded sieve of atkin + happy numbers
 */

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "bst.h"

typedef uint32_t chunk_t;

#define CHUNK_SIZE (sizeof (chunk_t) * 8)
#define THREAD_NUM 2

/* Lookup table for squares of 0-9 for happy finding */
const int squares[] = {0, 1, 4, 9, 16, 25, 36, 49, 64, 81};

/* Shared variables (global for easy thread access */
uint32_t lim; //UINT32_MAX;
uint32_t sq_lim;
uint32_t block;
struct bitset *bs;

struct bitset {
	pthread_mutex_t *mutices;
	chunk_t *chunks;
	uint64_t n_chunks;
};

struct args {
	uint32_t min;
	uint32_t max;
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
	int i;
	struct bitset *bs = malloc (sizeof (*bs));

	assert (bs != NULL);
	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;
	/* Calloc is faster than malloc + memset, better performance. */
	bs->chunks = calloc (bs->n_chunks, sizeof (*bs->chunks));
	assert (bs->chunks != NULL);
	bs->mutices = calloc (bs->n_chunks, sizeof(pthread_mutex_t));
	assert (bs->mutices != NULL);

	for (i = 0; i < bs->n_chunks; ++i) {
		if (pthread_mutex_init(&(bs->mutices[i]), NULL) > 0) {
			perror("pthread_mutex_init");
		}
	}

	return (bs);
}

void bitset_free (struct bitset *bs)
{
	if (bs != NULL) {
		if (bs->mutices != NULL) {
			free (bs->mutices);
		}
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

	pthread_mutex_lock(&(bs->mutices[bindex(idx)]));
	bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
	pthread_mutex_unlock(&(bs->mutices[bindex(idx)]));
}

void bit_toggle (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);

	pthread_mutex_lock(&(bs->mutices[bindex(idx)]));
	if (bs->chunks[bindex(idx)] & (1 << (boffset (idx)))) {
		bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
	} else {
		bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
	}
	pthread_mutex_unlock(&(bs->mutices[bindex(idx)]));
}

void bit_clear (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);

	pthread_mutex_lock(&(bs->mutices[bindex(idx)]));
	bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
	pthread_mutex_unlock(&(bs->mutices[bindex(idx)]));
}

chunk_t bit_get (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);
	chunk_t bit;

	pthread_mutex_lock(&(bs->mutices[bindex(idx)]));
	bit = (bs->chunks[bindex(idx)] & (1 << (boffset (idx))));
	pthread_mutex_unlock(&(bs->mutices[bindex(idx)]));

	return (bit);
}

/* Sieve functions */
void *candidate_primes(void *arg)
{
	struct args *in = (struct args *)arg;
	uint64_t n;
	uint32_t x, y;
	uint64_t x_sq, y_sq;

	/* Put in candidate primes w/ odd num of reps */
	for (x = in->min; x <= in->max; ++x) {
		x_sq = x * x;
		for (y = 1; y <= sq_lim; ++y) {
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

	return (NULL);
}

void *eliminate_composites(void *arg)
{
	struct args *in = (struct args *)arg;
	uint32_t k;
	uint64_t n;
	uint64_t n_sq;

	/* Eliminate composites */
	for (n = in->min; n <= in->max; ++n) {
		if (bit_get (bs, n)) {
			n_sq = n * n;
			for (k = n_sq; k < lim; k += n_sq) {
				bit_clear (bs, k);
			}
		}
	}

	return (NULL);
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

void *determine_happies(void *arg)
{
	struct args *in = (struct args *)arg;
	uint32_t n;

	/* Remove primes that aren't happy */
	for (n = in->min; n <= in->max; ++n) {
		if (bit_get (bs, n)) {
			if (!is_happy(n)) {
				bit_clear(bs, n);
			}
		}
	}

	return (NULL);
}

void spawn_threads(pthread_attr_t attr, void *(*func)(void *))
{
	pthread_t threads[THREAD_NUM];

	for (int i = 0; i < THREAD_NUM; ++i) {
		struct args arg;
		arg.min = i * block;
		arg.max = (i + 1) * block - 1;
		if (pthread_create(&threads[i], &attr, (*func), (void *)&arg))
			perror("pthread_create");
	}

	for (uint16_t i = 0; i < THREAD_NUM; ++i) {
		pthread_join(threads[i], NULL);
	}
}

int main (int argc, char **argv)
{
	uint32_t cnt = 0;
	uint64_t n;

	/* Initialize globals */
	lim = UINT32_MAX; //UINT32_MAX;
	sq_lim = (uint32_t) sqrt ((double)lim);
	block = (lim / THREAD_NUM);

	bs = bitset_alloc (lim);
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	spawn_threads(attr, candidate_primes);
	spawn_threads(attr, eliminate_composites);
	spawn_threads(attr, determine_happies);

	/* Print primes */
	//printf ("2, 3");
	cnt = 2;
	for (n = 5; n < lim; ++n) {
		if (bit_get (bs, n)) {
			//printf (", %u", n);
			++cnt;
		}
	}
	printf ("number of happy primes <%lu: %lu\n",
	       (unsigned long)lim, (unsigned long)cnt);
	bitset_free (bs);
}
