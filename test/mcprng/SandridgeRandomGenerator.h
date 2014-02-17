/*
 * SandridgeRandomGenerator.h
 *
 *  Created on: Dec 29, 2013
 *      Author: mti20
 */

#ifndef SANDRIDGERANDOMGENERATOR_H_
#define SANDRIDGERANDOMGENERATOR_H_

/*  Default parameters to use if none specified in the constructor. Parameters
 *  chosen are the recommended ones in Numerical Recipes (3rd edition). From
 *  Knuth, provided that the offset c is nonzero, the LCG will have a full
 *  period for all seed values if and only if: c and m are relatively prime,
 *  (a-1) is divisible by all prime factors of m and (a-1) is a multiple of 4
 *  if m is a multiple of 4 (Hull-Dobell theorem).
 *
 *  m = "modulus"
 *  a = "multiplier"
 *  c = "increment"
 */
#define  RNG_DEFAULT_SEED   1000         // arbitrarily picked
#define  RNG_DEFAULT_A      1664525
#define  RNG_DEFAULT_C      1013904223
#define  RNG_DEFAULT_M      4294967296   // = 2^32


/*
 *  Uses the mixed congruential method for generating pseudorandom integers.
 */
class SandridgeRandomGenerator {
private:
	const long a, c, m;
	long xn;
public:
	SandridgeRandomGenerator();
	SandridgeRandomGenerator(const int seed);
	SandridgeRandomGenerator(const int seed, const int a, const int c, const int m);
	inline long getRandom() { return (xn = (((a*xn)+c)%m)); }
	virtual ~SandridgeRandomGenerator();
};

#endif /* SANDRIDGERANDOMGENERATOR_H_ */
