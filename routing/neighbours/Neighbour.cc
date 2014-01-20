/*
 * Neighbour.cpp
 *
 *  Created on: Dec 27, 2013
 *      Author: mti20
 */

#include "Neighbour.h"

using namespace std;

Neighbour::Neighbour(const int macAddress, CastaliaModule* castaliaModule) : macAddress(macAddress), castaliaModule(castaliaModule) {
  castaliaModule->trace() << "Constructing new neighbour from node " << macAddress;
	timestamp = 0;  // TODO
}

Neighbour::Neighbour(const int macAddress, CastaliaModule* castaliaModule, simtime_t simtime) : macAddress(macAddress), castaliaModule(castaliaModule), timestamp(simtime) {
  castaliaModule->trace() << "Constructing new neighbour from node " << macAddress;
}

Neighbour::~Neighbour() {
	// nothing to do here
}

bool Neighbour::operator==(const Neighbour& other) {
	return macAddress == other.macAddress;
}

bool Neighbour::operator!=(const Neighbour& other) {
	return !(*this == other);
}

int Neighbour::getMacAddress() const {
	return macAddress;
}

void Neighbour::updateTimestamp(simtime_t currentTime) {
	timestamp = currentTime;
}

simtime_t Neighbour::getTimestamp() const {
	return timestamp;
}
