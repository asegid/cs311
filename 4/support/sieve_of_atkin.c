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

int main (int argc, char **argv)
{
	uint32_t limit = UINT32_MAX;
	uint32_t sqrt_lim = sqrt (limit);
	struct bitset *bs = bitset_alloc (limit);

	/* Put in candidate primes w/ odd num of reps */
	printf("candidate primes\n");
	for (uint32_t x = 1; x <= sqrt_lim; ++x) {
		uint64_t x_sq = x * x;
		for (uint32_t y = 1; y <= sqrt_lim; ++y) {
			uint64_t y_sq = y * y;
			int64_t n = 4 * x_sq + y_sq;

			if ((n <= limit)
			&& (((n % 12) == 1) || ((n % 12) == 5))) {
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

	printf("eliminate composites\n");
	/* Eliminate composites */
	for (uint32_t n = 5; n <= sqrt_lim; ++n) {
		if (bit_get (bs, n)) {
			printf("|%u|\n", n);
			uint32_t n_sq = n * n;
			for (uint32_t k = n_sq; k <= limit; k += n_sq) {
				bit_clear (bs, k);
			}
		}
	}

	printf("print out\n");
	/* Print primes */
	printf ("2, 3");
	uint32_t cnt = 0;
	for (uint32_t n = 5; n < limit; ++n) {
		if (bit_get (bs, n)) {
			printf (", %ud", n);
			++cnt;
		}
	}

	printf("count : %ud\n", cnt);
}
