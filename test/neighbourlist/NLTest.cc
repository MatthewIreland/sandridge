/*
 * NLTest.cc
 *
 *  Created on: Jan 24, 2014
 *      Author: mti20
 */

#include "MockObjects.h"
#include "NLTest.h"
#include "RandomNeighbourList.h"
#include "FloodingNeighbourList.h"
#include "MockNeighbour.h"
#include <list>

using namespace std;

NLTest::NLTest() {
	TEST_ADD(NLTest::test_flooding_nl)
	TEST_ADD(NLTest::test_random_nl)
}

void NLTest::test_flooding_nl() {
  CastaliaModule* cm = new CastaliaModule();
  FloodingNeighbourList* nl = new FloodingNeighbourList(cm);
  int i;
  for (i=0; i<100; i++)
	  nl->add(new Neighbour(i));
  list<Neighbour*> alln = nl->pickAllNeighbours();
  list<Neighbour*>::const_iterator it;
  i=0;
  for (it = alln.begin(); it != alln.end(); ++it) {
	  TEST_ASSERT((*it)->getMacAddress() == i);
	  i++;
  }

}

void NLTest::test_random_nl() {
  CastaliaModule* cm = new CastaliaModule();
  RandomNeighbourList* nl = new RandomNeighbourList(cm, 100);
  nl->add(new Neighbour(5));
  Neighbour* n = nl->pickRandomNeighbour(0.0);
  TEST_ASSERT(n->getMacAddress() == 5);
  nl->add(new Neighbour(6));
  nl->add(new Neighbour(7));
  nl->add(new Neighbour(8));
  nl->add(new Neighbour(9));
  Neighbour* n2 = nl->pickRandomNeighbour(5, 0.0);
  TEST_ASSERT(n2->getMacAddress() != 5);
}

// test program
int main(int argc, char* argv[]) {
	Test::Suite ts;
	ts.add(auto_ptr<Test::Suite>(new NLTest));

	auto_ptr<Test::Output> output(new Test::TextOutput(Test::TextOutput::Verbose));
	ts.run(*output, true);
}
