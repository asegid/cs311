/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/22/2013 07:52:21 PM
 *    Filename:  bitset.h
 *
 * Description:  Bitset public interface, inspired by:
 *               stackoverflow.com/questions/2633400/c-c-efficient-bit-array
 */

#include <stdint.h>

#define CHUNK_SIZE (sizeof (chunk_t) * 8)

typedef uint32_t chunk_t;

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

uint64_t bit_get (struct bitset *bs, uint64_t idx);


