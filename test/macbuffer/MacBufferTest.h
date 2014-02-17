/*
 * MacBufferTest.h
 *
 *  Created on: Jan 24, 2014
 *      Author: mti20
 */

#ifndef MACBUFFERTEST_H_
#define MACBUFFERTEST_H_

#include "../cpptest/src/cpptest.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

class MacBufferTest : public Test::Suite {
public:
	MacBufferTest();

private:
	void test_fifo();
	void test_capacity();
	//void test_debuf();

};

#endif /* MACBUFFERTEST_H_ */
