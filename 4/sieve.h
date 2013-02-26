/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/23/2013 04:56:58 PM
 *    Filename:  sieve.h
 *
 * Description:  Thread/forkable sieve of Atkin sub functions
 */

#ifndef SIEVE_H
#define SIEVE_H

#include <stdint.h>

#include "bitset.h"

void find_candidates(struct bitset *bs, uint32_t lim, uint32_t sqrt_lim,
                     uint32_t min, uint32_t max);

void elim_composites(struct bitset *bs, uint32_t lim,
                     uint32_t min, uint32_t max);

#endif
