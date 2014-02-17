/*
 * NeighbourList.h
 *
 *  Created on: Dec 27, 2013
 *      Author: mti20
 */

#ifndef NEIGHBOURLIST_H_
#define NEIGHBOURLIST_H_

#include <list>
#include "MockNeighbour.h"
#include <iostream>

class Neighbour;

using namespace std;

#define NEIGHBOUR_TIMEOUT 999999999

class NeighbourList {
protected:
	int previousRandomNeighbour;
	int sinkAddress;
	int neighbourTimeout;
	int ourMacAddress;
	long xn, a, c, m;
	Neighbour* sinkPointer;
	list<Neighbour*> neighbourList;
	CastaliaModule* castaliaModule;  // for trace
public:
	NeighbourList(CastaliaModule* castaliaModule);
	NeighbourList(CastaliaModule* castaliaModule, int neighbourTimeout);
	NeighbourList(CastaliaModule* castaliaModule, int neighbourTimeout, int ourMacAddress);
	virtual ~NeighbourList();
	void add(Neighbour* newNeighbour);   // TODO legacy, delete asap
	void add(Neighbour* newNeighbour, simtime_t simtime);   // adds neighbour if not already in list, otherwise updates timestamp
	void clean(simtime_t simtime);   // cleans neighbours whose timestamps are older than certain amount
	void clean();  // TODO delete this
	virtual list<Neighbour*> pickNeighbours() = 0;
};

#endif /* NEIGHBOURLIST_H_ */
