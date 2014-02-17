/*
 * MockNeighbour.h
 *
 */

#ifndef NEIGHBOUR_H_
#define NEIGHBOUR_H_

#include "MockObjects.h"

class Neighbour {
private:
	int macAddress;
public:
	Neighbour(const int macAddress);
	Neighbour(const int macAddress, CastaliaModule* castaliaModule);
	Neighbour(const int macAddress, CastaliaModule* castaliaModule, simtime_t simtime);
	bool operator==(const Neighbour& other);  // two neighbours are equal if they have equal macaddresses
	bool operator!=(const Neighbour& other);
	int getMacAddress() const;
	simtime_t getTimestamp() const;
	void updateTimestamp(simtime_t currentTime);
	virtual ~Neighbour();
};

#endif /* MOCKNEIGHBOUR_H_ */
