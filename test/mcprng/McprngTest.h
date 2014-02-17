/*
 * McprngTest.h
 *
 *  Created on: Jan 24, 2014
 *      Author: mti20
 */

#ifndef MCPRNGTEST_H_
#define MCPRNGTEST_H_

#include "../cpptest/src/cpptest.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

class McprngTest : public Test::Suite {
public:
	McprngTest();

private:
	void test_randomness();

};

#endif /* MCPRNGTEST_H_ */
