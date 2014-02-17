/*
 * NLTest.h
 *
 *  Created on: Jan 24, 2014
 *      Author: mti20
 */

#ifndef NLTEST_H_
#define NLTEST_H_

#include "../cpptest/src/cpptest.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

class NLTest : public Test::Suite {
public:
	NLTest();

private:
	void test_flooding_nl();
	void test_random_nl();

};

#endif /* NLTEST_H_ */
