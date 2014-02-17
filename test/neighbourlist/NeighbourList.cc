/*
 * NeighbourList.cpp
 *
 *  Created on: Dec 27, 2013
 *      Author: mti20
 */

#include "NeighbourList.h"

NeighbourList::NeighbourList(CastaliaModule* castaliaModule) : castaliaModule(castaliaModule) {
	neighbourList = list<Neighbour*>();
	previousRandomNeighbour=-1;
	xn = 10;
	a = 1664525;
	c = 1013904223;
	m = 4294967296;
	//sinkAddress = castaliaModule->par("sinkMacAddress");
	sinkAddress = 40;
	sinkPointer = NULL;   // note that sink can't time out
	neighbourTimeout = 9999;
	ourMacAddress = -1;
}

NeighbourList::NeighbourList(CastaliaModule* castaliaModule, int neighbourTimeout) : castaliaModule(castaliaModule), neighbourTimeout(neighbourTimeout) {
	neighbourList = list<Neighbour*>();
	previousRandomNeighbour=-1;
	xn = 10;
	a = 1664525;
	c = 1013904223;
	m = 4294967296;
	//sinkAddress = castaliaModule->par("sinkMacAddress");
	sinkAddress = 40;
	sinkPointer = NULL;   // note that sink can't time out
	ourMacAddress = -1;
}

NeighbourList::NeighbourList(CastaliaModule* castaliaModule, int neighbourTimeout, int ourMacAddress) : castaliaModule(castaliaModule), neighbourTimeout(neighbourTimeout), ourMacAddress(ourMacAddress) {
	neighbourList = list<Neighbour*>();
	previousRandomNeighbour=-1;
	xn = 10;
	a = 1664525;
	c = 1013904223;
	m = 4294967296;
	//sinkAddress = castaliaModule->par("sinkMacAddress");
	sinkAddress = 40;
	sinkPointer = NULL;   // note that sink can't time out
}

NeighbourList::~NeighbourList() {
	// everything on the stack, nothing to do here :)
}

// TODO legacy -- delete!
void NeighbourList::add(Neighbour* newNeighbour) {
	add(newNeighbour, 0);
}

/*
 *  Should be called whenever we receive a packet from station at the MAC layer.
 */
void NeighbourList::add(Neighbour* newNeighbour, simtime_t simtime) {
  cout << "NeighbourList: Received new neighbour. Simtime: " << simtime << ".";
	int newMacAddress = newNeighbour->getMacAddress();
	cout << "Neighbour's mac address is " << newMacAddress;
	if (newMacAddress == ourMacAddress) {  // purely for robustness: make sure
		cout <<         // we can't add ourself
				"ERROR: tried to add self to NL.";
		return;
	}
	list<Neighbour*>::const_iterator it;
	for (it = neighbourList.begin(); it != neighbourList.end(); ++it) {
		cout << "Testing to see if it's equal to neighbour " << (*it)->getMacAddress();
		//if ((*it) == newNeighbour) {  // TODO fix operator overloading
		if ((*it)->getMacAddress() == newNeighbour->getMacAddress()) {
			cout << "It's already in the list. Returning with no side effects.";
			(*it)->updateTimestamp(simtime);
			return;
		}
		cout << "Wasn't equal";
	}
	cout << "It's not already in list. Pushing it.";
	if (newMacAddress == sinkAddress) {
		cout << "Added sink to neighbour list.";
		sinkPointer = newNeighbour;
	}
	neighbourList.push_back(newNeighbour);
}

void NeighbourList::clean(simtime_t simtime) {
	return;
	cout << "Clean called with time: " << simtime << ", neighbour timeout: " << neighbourTimeout;
	list<Neighbour*>::iterator it;
	for (it = neighbourList.begin(); it != neighbourList.end(); ++it) {
		simtime_t age = simtime - (*it)->getTimestamp();
		cout << "Neighbour address: "
				<< (*it)->getMacAddress() << ". Age: " << age;
		if (age > neighbourTimeout) {
			cout << "Deleting timed out neighbour!";
			//neighbourList.erase(it);
		}
	}
}

// this method does nothing!
void NeighbourList::clean() {
	return;
}
