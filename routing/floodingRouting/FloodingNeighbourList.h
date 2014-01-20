/*
 *  FloodingNeighbourList.h
 *  Matthew Ireland, University of Cambridge
 *  27th December MMXIII
 *
 *  A list of neighbours for use in the flooding routing protocol. It extends
 *  the base class NeighbourList with a methods to return all neighbours in
 *  the list.
 */

#ifndef FLOODINGNEIGHBOURLIST_H_
#define FLOODINGNEIGHBOURLIST_H_

#include <map>
#include <string>
#include "VirtualRouting.h"
//#include "../neighbours/Neighbour.h"
#include "../neighbours/NeighbourList.h"
#include "FloodingRoutingPacket_m.h"

using namespace std;

class FloodingNeighbourList : public NeighbourList {
public:
	FloodingNeighbourList(CastaliaModule* castaliaModule);
	FloodingNeighbourList(CastaliaModule* castaliaModule, int neighbourTimeout);
	FloodingNeighbourList(CastaliaModule* castaliaModule,
			int neighbourTimeout, int ourMacAddress);
	list<Neighbour*> pickNeighbours();
	list<Neighbour*> pickAllNeighbours();
};



#endif    /* FLOODINGNEIGHBOURLIST_H_ */
