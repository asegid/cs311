/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:34:58 PM
 *    Filename:  fork.c
 *
 * Description:  Forked sieve of atkin + happy numbers
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "bst.h"

typedef uint8_t chunk_t;

#define BMP_PATH "/ass4_baylesj"
#define CHUNK_SIZE (sizeof (chunk_t) * 8)
#define PROC_NUM 16

/* Lookup table for squares of 0-9 for happy finding */
const int squares[] = {0, 1, 4, 9, 16, 25, 36, 49, 64, 81};

/* Shared variables (global for easy thread access */
uint32_t lim; //UINT32_MAX;
uint32_t sq_lim;
struct bitset *bs;

struct bitset {
	char *share_path;
	chunk_t *chunks;
	uint64_t n_chunks;
};

struct args {
	uint32_t min;
	uint32_t max;
};

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits, char path[]);
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

uint64_t chunks_size (struct bitset *bs)
{
	return (bs->n_chunks * sizeof (chunk_t));
}

/* Create and destroy bitsets */
struct bitset *bitset_alloc (uint64_t n_bits, char path[])
{
	struct bitset *bs = malloc (sizeof (*bs));
	int share_fd;
	char *s_path = malloc (strlen(path) * sizeof (char));
	assert (bs != NULL);

	strcpy(s_path, path);
	bs->share_path = s_path;

	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;
	if ((share_fd = shm_open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		perror("shm_open");
	
	if (ftruncate(share_fd, chunks_size(bs))== -1)
		perror("ftruncate");

	if ((bs->chunks = mmap(NULL, chunks_size(bs), PROT_WRITE | PROT_READ,
	                  MAP_SHARED, share_fd, 0)) == MAP_FAILED)
		perror("mmap");

	return (bs);
}

void bitset_free (struct bitset *bs)
{
	if (bs != NULL) {
		if (bs->chunks != NULL)
			munmap(bs->chunks, chunks_size(bs));
		if (bs->share_path != NULL) {
			shm_unlink (bs->share_path);
			free (bs->share_path);
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
	if (bs->chunks[bindex(idx)] & (1 << (boffset (idx)))) {
		bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
	} else {
		bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
	}
}

void bit_clear (struct bitset *bs, uint64_t idx)
{
	assert (bs != NULL);

	bs->chunks[bindex(idx)] &= ~(1 << (boffset (idx)));
}

chunk_t bit_get (struct bitset *bs, uint64_t idx)
{
	chunk_t bit;

	assert (bs != NULL);
	bit = (bs->chunks[bindex(idx)] & (1 << (boffset (idx))));
	return (bit);
}

/* Sieve functions */
void candidate_primes(void *arg)
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
}

void eliminate_composites(void *arg)
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

void determine_happies(void *arg)
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
}

void fork_proc(uint32_t min, uint32_t max, void (*func)(void *))
{
	uint32_t i;
	pid_t children[PROC_NUM];
	uint32_t block = ((max - min) / PROC_NUM);

	for (i = 0; i < PROC_NUM; ++i) {
		struct args arg;
		arg.min = i * block + min;
		arg.max = (i + 1) * block - 1 + min;

		switch (children[i] = fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			(*func)(&arg);
			exit (EXIT_SUCCESS);
		default:
			break;
		}
	}

	for (i = 0; i < PROC_NUM; ++i) {
		wait(NULL);
	}
}

int main (int argc, char **argv)
{
	uint32_t cnt = 0;
	uint64_t n;

	/* Initialize globals */
	lim = 1000000; //UINT32_MAX;
	sq_lim = (uint32_t) sqrt ((double)lim);

	bs = bitset_alloc (lim, BMP_PATH);

	fork_proc(1, sq_lim, candidate_primes);
	fork_proc(5, lim, eliminate_composites);
	//fork_proc(1, lim, determine_happies);

	/* Print primes */
	//printf ("2, 3");
	cnt = 2;
	for (n = 5; n < lim; ++n) {
		if (bit_get (bs, n)) {
			//printf (", %u", n);
			++cnt;
		}
	}
	//printf ("number of primes <%lu: %lu\n",
	//       (unsigned long)lim, (unsigned long)cnt);
	printf("%lu\n", (unsigned long)cnt);
	bitset_free (bs);
}
