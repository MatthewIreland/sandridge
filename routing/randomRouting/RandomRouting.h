/*
 *  RandomRouting.h
 *  Matthew Ireland, University of Cambridge
 *  2nd January MMXIV
 *
 *  Implements Random Walk Routing; that is, whenever a node receives a new
 *  packet, it picks a neighbour at random to which it should be forwarded.
 */

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <map>
#include <string>
#include "VirtualRouting.h"
#include "../neighbours/Neighbour.h"
#include "RandomNeighbourList.h"
#include "RandomRoutingPacket_m.h"

//class NeighbourList;

using namespace std;

enum RandomRoutingTimers {
  RR_TIMER_STARTUP,
  RR_TIMER_START,
  RR_TIMER_DIE,
  RR_TIMER_REDISCOVER,
  RR_TIMER_END
};

class RandomRouting: public VirtualRouting {
 private:
	int mySeqNumber;       // sequence number for data packets originating here
	RandomNeighbourList* neighbourList;
	bool isSink, printDebugInfo, startupComplete, hasEnded, isAlive;
	bool dieTimeHasPassed, nodeShouldDie;
	int minStartupDelay, maxStartupDelay, neighbourOldAge, rediscoverTime;
	int strength;

	/**** timer callbacks ****/
	void handleStartupTimerCallback();
	void handleStartTimerCallback();
	void handleDieTimerCallback();
	void handleEndTimerCallback();
	/**** end timer callbacks ****/

	void printInfo(string);  // TODO move to metaclass

	void rediscoverNeighbours();

 protected:
	void startup();
	void finish();
	void fromApplicationLayer(cPacket *, const char *);
	void fromMacLayer(cPacket *, int, double, double);
	void handleRandomRoutingControlMessage(cMessage* msg);
	void timerFiredCallback(int);
};

#endif              // _RANDOM_H_
