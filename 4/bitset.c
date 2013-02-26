/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:52:21 PM
 *    Filename:  bitset.c
 *
 * Description:  Bitset function implementations, inspired by:
 *               stackoverflow.com/questions/2633400/c-c-efficient-bit-array
 */

#include <string.h>
#include <stdlib.h>

#include "bitset.h"

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

