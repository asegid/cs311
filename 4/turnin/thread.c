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
#include <sys/times.h>
#include <unistd.h>

#include "bst.h"

typedef uint32_t chunk_t;

#define CHUNK_SIZE (sizeof (chunk_t) * 8)
#define THREAD_NUM 4
#define CHUNKS_PER_MUTEX 1024

/* Lookup table for squares of 0-9 for happy finding */
const int squares[] = { 0, 1, 4, 9, 16, 25, 36, 49, 64, 81 };

/* Shared variables (global for easy access) */
uint32_t lim;
uint32_t sq_lim;
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

/* Function prototypes */

/* Find the mutex associated with index */
uint64_t bmutex(uint64_t idx)
{
	return (idx / (CHUNK_SIZE * CHUNKS_PER_MUTEX));
}

/* Chunk corresponding to index returned */
uint64_t bindex(uint64_t idx)
{
	return (idx / CHUNK_SIZE);
}

/* Place in chunk corresponding to index returned */
uint64_t boffset(uint64_t idx)
{
	return (idx % CHUNK_SIZE);
}

uint64_t chunks_size(struct bitset * bs)
{
	return (bs->n_chunks * sizeof(chunk_t));
}

/* Create and destroy bitsets */
struct bitset *bitset_alloc(uint64_t n_bits)
{
	uint32_t i;
	uint32_t num_mutexes;
	pthread_mutexattr_t attr;
	struct bitset *bs = malloc(sizeof(*bs));

	/* Initialize chunk memory */
	assert(bs != NULL);
	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;
	bs->chunks = calloc(bs->n_chunks, sizeof(*bs->chunks));
	assert(bs->chunks != NULL);

	/* Initialize mutexes */
	bs->mutices =
	    calloc((bs->n_chunks / CHUNKS_PER_MUTEX),
		   sizeof(pthread_mutex_t));
	assert(bs->mutices != NULL);
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	num_mutexes = (bs->n_chunks / CHUNKS_PER_MUTEX);
	for (i = 0; i < num_mutexes; ++i) {
		if (pthread_mutex_init(&(bs->mutices[i]), &attr) > 0) {
			perror("pthread_mutex_init");
		}
	}

	return (bs);
}

void bitset_free(struct bitset *bs)
{
	if (bs != NULL) {
		if (bs->mutices != NULL)
			free(bs->mutices);
		if (bs->chunks != NULL)
			free(bs->chunks);
		free(bs);
	}
}

/* Bit operations */
void bit_set(struct bitset *bs, uint64_t idx, bool use_mutex)
{
	assert(bs != NULL);
	if (use_mutex) {
		if (pthread_mutex_lock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_lock");
	}

	bs->chunks[bindex(idx)] |= 1 << (boffset(idx));

	if (use_mutex) {
		if (pthread_mutex_unlock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_unlock");
	}
}

void bit_toggle(struct bitset *bs, uint64_t idx, bool use_mutex)
{
	assert(bs != NULL);

	if (use_mutex) {
		if (pthread_mutex_lock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_lock");
	}

	if (bs->chunks[bindex(idx)] & (1 << (boffset(idx)))) {
		bs->chunks[bindex(idx)] &= ~(1 << (boffset(idx)));
	} else {
		bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
	}

	if (use_mutex) {
		if (pthread_mutex_unlock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_unlock");
	}
}

void bit_clear(struct bitset *bs, uint64_t idx, bool use_mutex)
{
	assert(bs != NULL);

	if (use_mutex) {
		if (pthread_mutex_lock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_lock");
	}

	bs->chunks[bindex(idx)] &= ~(1 << (boffset(idx)));

	if (use_mutex) {
		if (pthread_mutex_unlock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_unlock");
	}
}

chunk_t bit_get(struct bitset *bs, uint64_t idx, bool use_mutex)
{
	chunk_t bit;
	assert(bs != NULL);

	if (use_mutex) {
		if (pthread_mutex_lock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_lock");
	}

	bit = (bs->chunks[bindex(idx)] & (1 << (boffset(idx))));

	if (use_mutex) {
		if (pthread_mutex_unlock(&(bs->mutices[bmutex(idx)])) != 0)
			perror("pthread_mutex_unlock");
	}
	return (bit);
}

/* Sieve functions */
void *candidate_primes(void *arg)
{
	struct args *in = (struct args *) arg;
	int64_t n;
	uint64_t x, y;
	uint64_t x_sq, y_sq;

	/* Put in candidate primes w/ odd num of reps */
	for (x = in->min; x <= in->max; ++x) {
		x_sq = x * x;
		for (y = 1; y <= sq_lim; ++y) {
			y_sq = y * y;
			n = 4 * x_sq + y_sq;

			if ((n <= lim)
			    && (((n % 12) == 1) || (((n % 12) == 5)))) {
				bit_toggle(bs, n, true);
			}

			n -= x_sq;
			if ((n <= lim) && ((n % 12) == 7)) {
				bit_toggle(bs, n, true);
			}

			n -= 2 * y_sq;
			if ((x > y) && (n <= lim) && ((n % 12) == 11)) {
				bit_toggle(bs, n, true);
			}
		}
	}

	pthread_exit(NULL);
}

void *eliminate_composites(void *arg)
{
	struct args *in = (struct args *) arg;
	uint64_t k;
	uint64_t n;
	uint64_t n_sq;

	/* Eliminate composites */
	for (n = in->min; n <= in->max; ++n) {
		if (bit_get(bs, n, true)) {
			n_sq = n * n;
			for (k = n_sq; k <= lim; k += n_sq) {
				bit_clear(bs, k, true);
			}
		}
	}

	pthread_exit(NULL);
}

/* Happy functions */
bool is_happy(uint32_t num)
{
	uint64_t ceil;
	uint32_t cur = num;
	uint32_t digit;
	bool happy = false;
	bool repeating = false;
	uint64_t mod;
	uint32_t rem = num;
	uint64_t sum = 1;
	struct BSTree *seen = newBSTree();

	while (!happy && !repeating) {
		mod = 10;
		sum = 0;

		ceil = cur * 10;
		while (mod <= ceil) {
			digit = (cur % mod) / (mod / 10);
			sum += squares[digit];
			rem -= digit;
			mod *= 10;
		}

		if (sum == 1) {
			happy = true;
		} else if (containsBSTree(seen, sum)) {
			repeating = true;
		} else {
			addBSTree(seen, sum);
			cur = sum;
		}
	}
	deleteBSTree(seen);
	return (happy);
}

void *determine_happies(void *arg)
{
	struct args *in = (struct args *) arg;
	uint64_t i;

	/* Remove primes that aren't happy */
	for (i = in->min; i <= in->max; ++i) {
		/*
		 * Note that each thread should only access separate pieces of
		 * array, there should be zero overlap
		 */
		if (bit_get(bs, i, false)) {
			if (!is_happy(i)) {
				bit_clear(bs, i, false);
			}
		}
	}

	pthread_exit(NULL);
}

void spawn_threads(pthread_attr_t * attr, uint32_t min,
		   uint32_t max, void *(*func) (void *))
{
	struct args arg;
	uint32_t i;
	pthread_t threads[THREAD_NUM];
	uint32_t block = ((max - min) / THREAD_NUM);

	for (i = 0; i < THREAD_NUM; ++i) {
		arg.min = i * block + min;
		/* Handle rounding errors by just rounding up on last thread */
		if (i == (THREAD_NUM - 1))
			arg.max = max;
		else
			arg.max = ((i + 1) * block) - 1 + min;

		if (pthread_create
		    (&threads[i], attr, (*func), (void *) &arg) != 0)
			perror("pthread_create");
	}

	for (i = 0; i < THREAD_NUM; ++i) {
		if (pthread_join(threads[i], NULL) != 0)
			perror("pthread_join");
	}
}

void print_on(struct bitset *bits, char *str)
{
	uint64_t i;
	uint64_t cnt = 0;
	uint64_t maxval = (uint64_t) bits->n_chunks * CHUNK_SIZE;

	for (i = 0; i < maxval; ++i) {
		if (bit_get(bits, i, false))
			++cnt;
	}
	printf("count of %s: %lu\n", str, (unsigned long) cnt);
}

void print_times(struct tms timer, char *str)
{
	/* Note, NOT the same as CLOCKS_PER_SEC */
	long clocks_per_second = sysconf(_SC_CLK_TCK);
	printf("User time for %s (self + children): %lfs\n", str,
	       (double) (timer.tms_utime +
			 timer.tms_cutime) / clocks_per_second);
	printf("System time for %s (self + children): %lfs\n", str,
	       (double) (timer.tms_stime +
			 timer.tms_cstime) / clocks_per_second);
}

int main(int argc, char **argv)
{
	struct tms timer;

	/* Initialize globals */
	lim = UINT32_MAX;
	sq_lim = (uint32_t) sqrt((double) lim);
	printf("Max value: %lu\n", (unsigned long) lim);
	bs = bitset_alloc(lim);

	/* Run program */
	bit_set(bs, 2, false);
	bit_set(bs, 3, false);
	printf("finding candidate primes...\n");
	spawn_threads(NULL, 1, sq_lim, candidate_primes);
	printf("eliminating composite results...\n");
	spawn_threads(NULL, 5, lim, eliminate_composites);
	if (times(&timer) == -1)
		perror("times");
	print_times(timer, "primes");
	print_on(bs, "primes");

	printf("finding happy number status...\n");
	spawn_threads(NULL, 1, lim, determine_happies);
	print_on(bs, "happy primes");

	/* Cleanup */
	bitset_free(bs);

	return ((errno == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
