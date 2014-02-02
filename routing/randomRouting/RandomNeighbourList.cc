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
 *  The method for picking a random neighbour, when we are the initial source
 *  of the data value.
 *
 *  @param currentTime The current simulation time (so that the list can be
 *  cleaned)
 *
 *  @return A random neighbour.
 */
Neighbour* RandomNeighbourList::pickRandomNeighbour(simtime_t currentTime) {
	// when we compare the mac address, none will have address -1
	return pickRandomNeighbour(-1, currentTime);
}

/**
 * Usual method for getting a random neighbour from the neighbour list.
 *
 * @param srcMacAddress MAC address to exclude for the don't-forward-back-
 * to-where-it-came-from heuristic, i.e. the source of the packet.
 *
 * @param currentTime The current simulation time (so that the list can be
 * cleaned)
 *
 * @return A random neighbour, whose MAC address is not equal to the one
 * supplied, if that heuristic is enabled.
 *
 * If neighbour list has size 0, too bad, the null pointer will necessarily be
 * returned. This should only occur if we are the originator of the packet and
 * haven't received any setup packets yet.
 * If neighbour list has size 1, necessarily returns that neighbour.
 * If neighbour list has size 2 or more, a random number is generated to decide
 * which neighbour will be picked, but we don't pick the source MAC address,
 * since we have a choice.
 *
 * Has the side effect of cleaning the list.
 *
 */
Neighbour* RandomNeighbourList::pickRandomNeighbour(int srcMacAddress,
		simtime_t currentTime) {
	castaliaModule->trace() << "Picking random neighbour";
	if (sinkPointer != NULL) {
		castaliaModule->trace() << "Sink is a neighbour. Returning directly.";
		return sinkPointer;
	}
	clean(currentTime);
	int randomNeighbourNumber;
	if (neighbourList.size() == 0) {
		castaliaModule->trace() << "Empty neighbour list. Returning null.";
		return  NULL;
	} else if (neighbourList.size() == 1) {
		castaliaModule->trace() << "Neighbour list of size 1. " <<
				"Returning only option.";
		return neighbourList.front();
	} else {

		randomNeighbourNumber  = round((double(rand())/RAND_MAX) * double(neighbourList.size()));
		castaliaModule->trace() << "Picked random neighbour number: "
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
				castaliaModule->trace() << "random neighbour number is 0. testing mac address";
				if ((*it)->getMacAddress() != srcMacAddress) {
					castaliaModule->trace() << "mac address is not equal.";
					return *it;
				} else {
					castaliaModule->trace() << "mac address is equal. trying alternatives.";
					if (previousNeighbour != NULL) {
						castaliaModule->trace() << "previous neighbour not null. returning that.";
						return previousNeighbour;
					} else {
						castaliaModule->trace() << "previous neighbour is null. continuing.";
						continue; // randomNeighbourNumber will still be 0, so next neighbour will be returned, since neighbours have unique mac addresses. if this is the last neighbour, it's fine, since we'll return our only option below.
					}
				}
			} else {
				castaliaModule->trace()
						<< "Decrementing random neighbour number.";
				randomNeighbourNumber--;
			}
			previousNeighbour = *it;
		}
		castaliaModule->trace() << "have no option but to return front of list.";  // see comment next to "continue" above.
		return neighbourList.front();
	}
	return NULL;
}

/*
 * pickNeighbours()
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
	castaliaModule->trace() << "getOldNeighbours called";
	list<Neighbour*>::const_iterator it;
	for (it = neighbourList.begin(); it != neighbourList.end(); ++it) {
		simtime_t age = simtime - (*it)->getTimestamp();
		castaliaModule->trace() << "Neighbour age: " << age;
		if (age > oldAge) {
			castaliaModule->trace() << "Neighbour is old: pushing to list!";
			oldNeighbours.push_back(*it);
		}
	}
}
