/*
 * McprngTest.cc
 *
 *  Created on: Jan 24, 2014
 *      Author: mti20
 */

#include "McprngTest.h"
#include "SandridgeRandomGenerator.h"
#include <unordered_map>

#define TEST_SIZE 4294967296
#define DEG_FREEDOM 4294967295

McprngTest::McprngTest() {
	TEST_ADD(McprngTest::test_randomness)
}

void McprngTest::test_randomness() {
  SandridgeRandomGenerator* rnd = new SandridgeRandomGenerator();
  long randoms [TEST_SIZE];
  unordered_map<long, long> observedFrequencies;
  long k  = DEG_FREEDOM+1;
  for (int i=0; i<k; i++) {
    randoms[i] = rnd->getRandom();
    observedFrequencies[randoms[i]] = observedFrequencies[randoms[i]] + 1;
  }

  // OLD: 95% confidence, 100 degress of freedom
  // from http://sites.stat.psu.edu/~mga/401/tables/Chi-square-table.pdf
  //const double chiSquared = 124.342;

  // NEW: 95% confidence, 4294967295 degrees of freedom
  // courtesy of WolframAlpha
  const double chiSquared = 18446744065119617025.0;


  double T = 0;
  long n   = TEST_SIZE;
  long m   = 4294967296;
  double pi = 1/m;
  double numerator;

  for (long i=0; i<k; i++) {
    numerator = (double(observedFrequencies[i]) - m*pi);
    numerator = numerator*numerator;
    T += (numerator/(m*pi));
  }
  cout << "T: " << T << endl;
  TEST_ASSERT(T <= chiSquared);
}


// test program
int main(int argc, char* argv[]) {
	Test::Suite ts;
	ts.add(auto_ptr<Test::Suite>(new McprngTest));

	auto_ptr<Test::Output> output(new Test::TextOutput(Test::TextOutput::Verbose));
	ts.run(*output, true);
}
