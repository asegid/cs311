/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  03/09/2013 03:02:05 PM
 *    Filename:  mps.c
 *
 * Description:  al;sdkfj
 *
 *
 */

/*
--- /               \
---/ processing unit \---- output
  --------------------
      |    |    |
    ---MODIFIERS---

+
-
<<
>>
%
define flop as an operation with floating point numbers
(refresher a floating point number is [+/- sign][exponent][mantissa])
slight issue: not at FLOPS are the same. modulo takes a different amount
of time then say a divide or a plus operation.
All of these operations are actually performed on the cpu, and take
different amounts of effort to perform
*/

#define TIMEOUT .05

start = clock();
int i = 0;

do {
	a % b;
	elapsed = (clock() - start)/CLOCKS_PER_SEC;
	i += 2;
} while (elapsed < TIMEOUT);
