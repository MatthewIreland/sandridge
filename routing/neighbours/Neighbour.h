/*
 * Neighbour.h
 *
 *  Created on: Dec 27, 2013
 *      Author: mti20
 */

#ifndef NEIGHBOUR_H_
#define NEIGHBOUR_H_

#include "VirtualRouting.h"
//#include "FloodingRouting.h"
#include "VirtualMac.h"

class Neighbour {
private:
int macAddress;        // TODO should be final
	simtime_t timestamp;
CastaliaModule* castaliaModule;   // for tracing purposes
public:
Neighbour(const int macAddress, CastaliaModule* castaliaModule);
Neighbour(const int macAddress, CastaliaModule* castaliaModule, simtime_t timestamp);
	virtual ~Neighbour();
	bool operator==(const Neighbour& other);  // two neighbours are equal if they have equal macaddresses
	bool operator!=(const Neighbour& other);
	int getMacAddress() const;
	simtime_t getTimestamp() const;
	void updateTimestamp(simtime_t currentTime);
};

#endif /* NEIGHBOUR_H_ */
