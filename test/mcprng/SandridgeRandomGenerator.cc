/*
 * SandridgeRandomGenerator.cc
 *
 *  Created on: Dec 29, 2013
 *      Author: mti20
 */

#include "SandridgeRandomGenerator.h"

SandridgeRandomGenerator::SandridgeRandomGenerator() : a(RNG_DEFAULT_A),
                                                       c(RNG_DEFAULT_C),
                                                       m(RNG_DEFAULT_M),
                                                       xn(RNG_DEFAULT_SEED) {}

SandridgeRandomGenerator::SandridgeRandomGenerator(const int seed)
                                                     : a(RNG_DEFAULT_A),
                                                       c(RNG_DEFAULT_C),
                                                       m(RNG_DEFAULT_M),
                                                       xn(seed) {}

SandridgeRandomGenerator::SandridgeRandomGenerator(const int seed,
		const int a, const int c, const int m)       : a(a),
                                                       c(c),
                                                       m(m),
                                                       xn(seed) {}

SandridgeRandomGenerator::~SandridgeRandomGenerator() {
	// everything is on the stack: nothing to delete here
}

