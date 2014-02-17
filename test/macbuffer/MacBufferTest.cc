/*
 * MacBufferTest.cc
 *
 *  Created on: Jan 24, 2014
 *      Author: mti20
 */

#include "MacBufferTest.h"
#include "MacBuffer.h"

template class MacBuffer<int>;

MacBufferTest::MacBufferTest() {
	TEST_ADD(MacBufferTest::test_fifo)
	TEST_ADD(MacBufferTest::test_capacity)
}

void MacBufferTest::test_fifo() {
  CastaliaModule* cm = new CastaliaModule();
  MacBuffer<int>* buf = new MacBuffer<int>(cm, 200, true);
	for (int i=0; i<100; i++) {
		buf->insertPacket(i);
	}
	for (int i=0; i<100; i++) {
		int pkt = buf->peek();
		TEST_ASSERT(pkt == i);
		buf->removeFirst();
	}
	TEST_ASSERT(buf->isEmpty());
}

void MacBufferTest::test_capacity() {
  CastaliaModule* cm = new CastaliaModule();
  MacBuffer<int>* buf = new MacBuffer<int>(cm, 10, true);
	try {
		for (int i=0; i<10; i++) {
			buf->insertPacket(i);
		}
	} catch (MacBufferFullException &e) {
		TEST_FAIL("mac buffer full exception thrown too soon")
	}
	try {
	  buf->insertPacket(10);
		TEST_FAIL("mac buffer full exception not thrown when full")
	} catch (MacBufferFullException &e) {
		// success
		return;
	}

}

// test program
int main(int argc, char* argv[]) {
	Test::Suite ts;
	ts.add(auto_ptr<Test::Suite>(new MacBufferTest));

	auto_ptr<Test::Output> output(new Test::TextOutput(Test::TextOutput::Verbose));
	ts.run(*output, true);
}
