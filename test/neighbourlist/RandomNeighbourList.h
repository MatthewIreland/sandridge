/*
 *  RandomNeighbourList.h
 *  Matthew Ireland, University of Cambridge
 *  2nd January MMXIV
 *
 *  A list of neighbours for use in the random routing protocol. It extends
 *  the base class NeighbourList with a methods to return a random neighbour
 *  from the list.
 */

#ifndef RANDOMNEIGHBOURLIST_H_
#define RANDOMNEIGHBOURLIST_H_

#include <map>
#include <string>
#include <iostream>
#include "VirtualRouting.h"
//#include "../neighbours/Neighbour.h"
#include "NeighbourList.h"
#include "RandomRoutingPacket_m.h"

using namespace std;

class RandomNeighbourList : public NeighbourList {
public:
	RandomNeighbourList(CastaliaModule* castaliaModule);
	RandomNeighbourList(CastaliaModule* castaliaModule, int neighbourTimeout);
	RandomNeighbourList(CastaliaModule* castaliaModule,
			int neighbourTimeout, int ourMacAddress);
	list<Neighbour*> pickNeighbours();
	Neighbour* pickRandomNeighbour(int srcMacAddress, simtime_t currentTime);
	Neighbour* pickRandomNeighbour(simtime_t currentTime);  // we don't care where it came from (used if we're the originator
	void getOldNeighbours(list<Neighbour*>&, simtime_t simtime, int oldAge);
};



#endif    /* RANDOMNEIGHBOURLIST_H_ */
