/*
 *  FloodingNeighbourList.cc
 *  Matthew Ireland, University of Cambridge
 *  27th December MMXIII
 *
 *  A list of neighbours for use in the flooding routing protocol. It extends
 *  the base class NeighbourList with a methods to return all neighbours in
 *  the list.
 */

#include "FloodingNeighbourList.h"

FloodingNeighbourList::FloodingNeighbourList(CastaliaModule* castaliaModule)
							: NeighbourList(castaliaModule) {}

FloodingNeighbourList::FloodingNeighbourList(CastaliaModule* castaliaModule,
		int neighbourTimeout) : NeighbourList(castaliaModule,
				                              neighbourTimeout) {}

FloodingNeighbourList::FloodingNeighbourList(CastaliaModule* castaliaModule,
			int neighbourTimeout, int ourMacAddress) :
					NeighbourList(castaliaModule,
							neighbourTimeout,
							ourMacAddress) {}
/**
 *  TODO: document
 *
 *  @return TODO
 */
list<Neighbour*> FloodingNeighbourList::pickAllNeighbours() {
	clean();
	return neighbourList;
}

list<Neighbour*> FloodingNeighbourList::pickNeighbours() {
	return pickAllNeighbours();
}
