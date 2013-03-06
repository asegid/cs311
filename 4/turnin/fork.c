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
#include <limits.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "bst.h"

typedef uint32_t chunk_t;

#define BMP_PATH "/baylesj_bitmap"
#define SEM_PATH "/baylesj_semaphore"
#define CHUNK_SIZE (sizeof (chunk_t) * 8)
#define PROC_NUM 4
#define CHUNKS_PER_SEMAPHORE 1024

/* Lookup table for squares of 0-9 for happy finding */
const int squares[] = { 0, 1, 4, 9, 16, 25, 36, 49, 64, 81 };

/* Shared variables (global for easy access) */
struct bitset *bs;
uint32_t lim;
pid_t procs[PROC_NUM];
sem_t *sem;
uint32_t sq_lim;

struct bitset {
	char chunks_path[PATH_MAX];
	char sems_path[PATH_MAX];
	sem_t *sems;
	chunk_t *chunks;
	uint64_t n_chunks;
};

struct args {
	uint32_t min;
	uint32_t max;
};

/* Function prototypes */

/* Find the semaphore associated with index */
uint64_t bsemaphore(uint64_t idx)
{
	return (idx / (CHUNK_SIZE * CHUNKS_PER_SEMAPHORE));
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
struct bitset *bitset_alloc(uint64_t n_bits, char bits_path[],
			    char sem_path[])
{
	struct bitset *bs = malloc(sizeof(*bs));
	uint32_t i;
	int share_fd;
	int sem_fd;
	uint64_t sem_size;
	uint64_t num_sems;

	assert(bs != NULL);
	strcpy(bs->chunks_path, bits_path);
	strcpy(bs->sems_path, sem_path);
	bs->n_chunks = (n_bits / CHUNK_SIZE) + 1;

	/* Map memory */
	shm_unlink(bits_path);
	if ((share_fd =
	     shm_open(bits_path, O_RDWR | O_CREAT,
		      S_IRUSR | S_IWUSR)) == -1)
		perror("shm_open");
	if (ftruncate(share_fd, chunks_size(bs)) == -1)
		perror("ftruncate");
	if ((bs->chunks =
	     mmap(NULL, chunks_size(bs), PROT_WRITE | PROT_READ,
		  MAP_SHARED, share_fd, 0)) == MAP_FAILED)
		perror("mmap");

	/* Map semaphores */
	shm_unlink(sem_path);
	sem_size = (bs->n_chunks / CHUNKS_PER_SEMAPHORE) * sizeof(sem_t);
	printf("sem_size = %lu\n", (unsigned long) sem_size);
	if ((sem_fd =
	     shm_open(SEM_PATH, O_CREAT | O_RDWR,
		      S_IRUSR | S_IWUSR)) == -1)
		perror("shm_open");
	if (ftruncate(sem_fd, sem_size) == -1)
		perror("ftruncate");
	if ((bs->sems = mmap(NULL, sem_size, PROT_WRITE | PROT_READ,
			     MAP_SHARED, sem_fd, 0)) == MAP_FAILED)
		perror("map failed");

	/* Initialize semaphores */
	num_sems = (bs->n_chunks / CHUNKS_PER_SEMAPHORE);
	for (i = 0; i < num_sems; ++i) {
		if (sem_init(&(bs->sems[i]), 1, 1) == -1)
			perror("sem_init");
	}

	return (bs);
}

void bitset_free(struct bitset *bs)
{
	if (bs != NULL) {
		if (bs->chunks != NULL)
			munmap(bs->chunks, chunks_size(bs));
		shm_unlink(bs->chunks_path);
		shm_unlink(bs->sems_path);
		free(bs);
	}
}

/* Bit operations */
void bit_set(struct bitset *bs, uint64_t idx, bool use_sem)
{
	assert(bs != NULL);
	if (use_sem) {
		if (sem_wait(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_wait");
	}

	bs->chunks[bindex(idx)] |= 1 << (boffset(idx));

	if (use_sem) {
		if (sem_post(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_post");
	}
}

void bit_toggle(struct bitset *bs, uint64_t idx, bool use_sem)
{
	assert(bs != NULL);

	if (use_sem) {
		if (sem_wait(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_wait");
	}

	if (bs->chunks[bindex(idx)] & (1 << (boffset(idx)))) {
		bs->chunks[bindex(idx)] &= ~(1 << (boffset(idx)));
	} else {
		bs->chunks[bindex(idx)] |= 1 << (boffset(idx));
	}

	if (use_sem) {
		if (sem_post(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_post");
	}
}

void bit_clear(struct bitset *bs, uint64_t idx, bool use_sem)
{
	assert(bs != NULL);
	if (use_sem) {
		if (sem_wait(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_wait");
	}

	bs->chunks[bindex(idx)] &= ~(1 << (boffset(idx)));

	if (use_sem) {
		if (sem_post(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_post");
	}
}

chunk_t bit_get(struct bitset *bs, uint64_t idx, bool use_sem)
{
	chunk_t bit;
	assert(bs != NULL);

	if (use_sem) {
		if (sem_wait(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_wait");
	}

	bit = (bs->chunks[bindex(idx)] & (1 << (boffset(idx))));

	if (use_sem) {
		if (sem_post(&(bs->sems[bsemaphore(idx)])) == -1)
			perror("sem_post");
	}
	return (bit);
}

/* Sieve functions */
void candidate_primes(struct args in)
{
	int64_t n;
	uint64_t x, y;
	uint64_t x_sq, y_sq;

	/* Put in candidate primes w/ odd num of reps */
	for (x = in.min; x <= in.max; ++x) {
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
}

void eliminate_composites(struct args in)
{
	uint64_t k;
	uint64_t n;
	uint64_t n_sq;

	/* Eliminate composites */
	for (n = in.min; n <= in.max; ++n) {
		if (bit_get(bs, n, true)) {
			n_sq = n * n;
			for (k = n_sq; k <= lim; k += n_sq) {
				bit_clear(bs, k, true);
			}
		}
	}
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

void determine_happies(struct args in)
{
	uint64_t i;

	/* Remove primes that aren't happy */
	for (i = in.min; i <= in.max; ++i) {
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
}

void fork_proc(uint32_t min, uint32_t max, void (*func) (struct args))
{
	struct args arg;
	uint32_t i;
	pid_t child;
	uint32_t block = ((max - min) / PROC_NUM);

	for (i = 0; i < PROC_NUM; ++i) {
		arg.min = i * block + min;
		/* Handle rounding errors by just rounding up on last thread */
		if (i == (PROC_NUM - 1))
			arg.max = max;
		else
			arg.max = ((i + 1) * block) - 1 + min;

		switch (child = fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			(*func) (arg);
			_exit(EXIT_SUCCESS);
		default:
			procs[i] = child;
			break;
		}
	}

	for (i = 0; i < PROC_NUM; ++i) {
		wait(NULL);
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

void sig_handle(int signo)
{
	int sig = (signo == SIGINT) ? SIGINT : SIGQUIT;

	for (uint32_t i = 0; i < PROC_NUM; ++i)
		kill(procs[i], sig);

	for (uint32_t i = 0; i < PROC_NUM; ++i)
		wait(NULL);

	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	struct tms timer;
	struct sigaction handler;

	/* Initialize globals */
	lim = UINT32_MAX;
	sq_lim = (uint32_t) sqrt((double) lim);
	printf("Max value: %lu\n", (unsigned long) lim);
	bs = bitset_alloc(lim, BMP_PATH, SEM_PATH);

	/* Install a signal handler */
	handler.sa_handler = sig_handle;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGHUP, &handler, NULL);
	sigaction(SIGINT, &handler, NULL);
	sigaction(SIGQUIT, &handler, NULL);

	/* Run program */
	bit_set(bs, 2, false);
	bit_set(bs, 3, false);
	printf("finding candidate primes...\n");
	fork_proc(1, sq_lim, candidate_primes);
	printf("eliminating composite results...\n");
	fork_proc(5, lim, eliminate_composites);
	if (times(&timer) == -1)
		perror("times");
	print_times(timer, "primes");
	print_on(bs, "primes");

	printf("finding happy number status...\n");
	fork_proc(1, lim, determine_happies);
	print_on(bs, "happy primes");

	/* Cleanup */
	bitset_free(bs);

	return ((errno == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
