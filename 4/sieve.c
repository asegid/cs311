/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/23/2013 04:56:58 PM
 *    Filename:  sieve.c
 *
 * Description:  Thread/forkable sieve of Atkin sub functions
 */

#include <stdint.h>

#include "bitset.h"

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
