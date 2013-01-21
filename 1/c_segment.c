/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  01/14/2013 08:08:04 AM
 *    Filename:  sieve.c
 *
 * Description:  Implementation of the Sieve of Eratosthenes
 */

#include <stdio.h>
#include <stdlib.h>

#define SPECIAL -1
#define UNMARKED 0
#define COMPOSITE 1
#define PRIME 2

#define DEFAULT_VALUE 1000L

struct primes {
	long count;
	long *values;
};

struct primes sieve_eratosthenes(long n)
{
	long candidates[n];
	long count;
	long m;
	long k;
	long multiple;
	struct primes results;

	/* Mark 1 as special */
	candidates[1] = SPECIAL;

	/* Set count to all (excepting 0,1), decrement as we go! */
	count = n - 2;

	/* Set k=1. While k < sqrt(n)... */
	k = 1;
	while (k * k < n) {
		m = k + 1;
		/* Find first # greater than k not IDed as composite */
		while (candidates[m] == COMPOSITE) {
			++m;
		}

		/* Mark multiples as composite */
		multiple = m;
		while (multiple < n) {
			if (candidates[multiple] != COMPOSITE) {
				count -= 1;
			}
			candidates[multiple] = COMPOSITE;
			multiple += m;
		}

		/* Put m on list */
		candidates[m] = PRIME;

		/* Set k=m and repeat */
		k = m;
	}
	printf("\n");

	/* Build the results to return */
	results.values = malloc(count * sizeof(long));
	results.count = 0;
	for (long i = 0; i < n; ++i) {
		if (candidates[i] == PRIME || candidates[i] == UNMARKED) {
			results.values[results.count] = i;
			results.count += 1;
		}
	}

	return (results);
}

void print_primes(struct primes results)
{
	printf("Results contain %ld primes\n", results.count);
	for (long i = 0; i < results.count; ++i) {
		printf("%ld ", results.values[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct primes results;
	if (argc == 2) {
		printf("Using user value of %ld\n", atol(argv[1]));
		results = sieve_eratosthenes(atol(argv[1]));
	} else {
		printf("Using default value of %ld\n", DEFAULT_VALUE);
		results = sieve_eratosthenes(DEFAULT_VALUE);
	}

	print_primes(results);

	return (0);
}
