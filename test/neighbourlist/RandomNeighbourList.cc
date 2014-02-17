/*
 *  RandomNeighbourList.cc
 *  Matthew Ireland, University of Cambridge
 *  2nd January MMXIV
 *
 *  A list of neighbours for use in the random routing protocol. It extends
 *  the base class NeighbourList with a methods to return a random neighbour
 *  from the list.
 */

#include "RandomNeighbourList.h"
#include <stdio.h>
#include <cstdlib>
#include <cmath>

using namespace std;

RandomNeighbourList::RandomNeighbourList(CastaliaModule* castaliaModule)
							: NeighbourList(castaliaModule) {}

RandomNeighbourList::RandomNeighbourList(CastaliaModule* castaliaModule,
		int neighbourTimeout) : NeighbourList(castaliaModule,
				                              neighbourTimeout) {}

RandomNeighbourList::RandomNeighbourList(CastaliaModule* castaliaModule,
			int neighbourTimeout, int ourMacAddress) :
					NeighbourList(castaliaModule,
							neighbourTimeout,
							ourMacAddress) {}
/**
 *  TODO: document
 *
 *  @return TODO
 */
Neighbour* RandomNeighbourList::pickRandomNeighbour(simtime_t currentTime) {
	pickRandomNeighbour(-1, currentTime);  // when we compare the mac address, none will have address -1
}

/*
 * Used in random routing.
 * If neighbour list has size 0, too bad, the null pointer will necessarily be
 * returned. This should only occur if we are the originator of the packet and
 * haven't received any setup packets yet.
 * If neighbour list has size 1, necessarily returns that neighbour.
 * If neighbour list has size 2 or more, a random number is generated to decide
 * which neighbour will be picked, but we don't pick the source MAC address,
 * since we have a choice.
 */
Neighbour* RandomNeighbourList::pickRandomNeighbour(int srcMacAddress, simtime_t currentTime) {
	cout << "Picking random neighbour";
	if (sinkPointer != NULL) {
		cout << "Sink is a neighbour. Returning directly.";
		return sinkPointer;
	}
	clean(currentTime);
	int randomNeighbourNumber;
	if (neighbourList.size() == 0) {
		cout << "Empty neighbour list. Returning null.";
		return  NULL;
	} else if (neighbourList.size() == 1) {
		cout << "Neighbour list of size 1. " <<
				"Returning only option.";
		return neighbourList.front();
	} else {

		randomNeighbourNumber  = round((double(rand())/RAND_MAX) * double(neighbourList.size()));
		cout << "Picked random neighbour number: "
				<< randomNeighbourNumber
			    << ". (Neighbour List has size " << neighbourList.size()
			    << ".)";

		/** heuristic -- don't pick same neighbour twice **/
		//while (randomNeighbourNumber != previousRandomNeighbour) {
			//randomNeighbourNumber = (rand()+1) % neighbourList.size();
		//}
		previousRandomNeighbour = randomNeighbourNumber;
		/** end heuristic -- don't pick same neighbour twice **/

		list<Neighbour*>::const_iterator it;
		Neighbour* previousNeighbour = NULL;
		for (it = neighbourList.begin(); it != neighbourList.end(); ++it) {
			if (randomNeighbourNumber == 0) {
				cout << "random neighbour number is 0. testing mac address";
				if ((*it)->getMacAddress() != srcMacAddress) {
					cout << "mac address is not equal.";
					return *it;
				} else {
					cout << "mac address is equal. trying alternatives.";
					if (previousNeighbour != NULL) {
						cout << "previous neighbour not null. returning that.";
						return previousNeighbour;
					} else {
						cout << "previous neighbour is null. continuing.";
						continue; // randomNeighbourNumber will still be 0, so next neighbour will be returned, since neighbours have unique mac addresses. if this is the last neighbour, it's fine, since we'll return our only option below.
					}
				}
			} else {
				cout
						<< "Decrementing random neighbour number.";
				randomNeighbourNumber--;
			}
			previousNeighbour = *it;
		}
		cout << "have no option but to return front of list.";  // see comment next to "continue" above.
		return neighbourList.front();
	}
	return NULL;
}

/*
 * TODO document
 *
 * copies the list on return, which isn't ideal, but since there's only one
 * element (and that's a pointer), it's acceptable.
 */
list<Neighbour*> RandomNeighbourList::pickNeighbours() {
	list<Neighbour*> nl;
	nl.push_back(pickRandomNeighbour(999));
	return nl;
}

/*
 *  Intended as a way of avoiding neighbours timing out if they're alive but
 *  just quiet.
 */
void RandomNeighbourList::getOldNeighbours(list<Neighbour*>& oldNeighbours, simtime_t simtime, int oldAge) {
	cout << "getOldNeighbours called";
	list<Neighbour*>::const_iterator it;
	for (it = neighbourList.begin(); it != neighbourList.end(); ++it) {
		simtime_t age = simtime - (*it)->getTimestamp();
		cout << "Neighbour age: " << age;
		if (age > oldAge) {
			cout << "Neighbour is old: pushing to list!";
			oldNeighbours.push_back(*it);
		}
	}
}
