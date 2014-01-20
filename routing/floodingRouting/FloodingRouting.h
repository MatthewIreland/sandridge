/*
 *  FloodingRouting.h
 *  Matthew Ireland, University of Cambridge
 *  27th December MMXIII
 *
 *  Implements Flood Routing; that is, whenever a node receives a new packet,
 *  it forwards it to all its neighbours.
 */


#ifndef _FLOODING_H_
#define _FLOODING_H_

#include <map>
#include <string>
#include "VirtualRouting.h"
#include "../neighbours/Neighbour.h"
#include "FloodingNeighbourList.h"
#include "FloodingRoutingPacket_m.h"

//class FloodingNeighbourList;

using namespace std;

enum FloodingRoutingTimers {
  FR_TIMER_STARTUP,
  FR_TIMER_MAXSTARTUP,
  FR_TIMER_SINKDISCOVERY
};

class FloodingRouting: public VirtualRouting {
 private:
	int mySeqNumber;       // sequence number for data packets originating here
	FloodingNeighbourList* neighbourList;
	bool isSink, printDebugInfo, startupComplete;
	int minStartupDelay, maxStartupDelay;

	/**** timer callbacks ****/
	void handleStartupTimerCallback();
	/**** end timer callbacks ****/

 protected:
	void startup();
	void finish();
	void fromApplicationLayer(cPacket *, const char *);
	void fromMacLayer(cPacket *, int, double, double);
	void handleFloodingRoutingControlMessage(cMessage* msg);
	void timerFiredCallback(int);
};

#endif              // _FLOODING_H_
