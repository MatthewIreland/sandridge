/*
 * MockNeighbour.cc
 *
 */

#include "MockObjects.h"
#include "MockNeighbour.h"

Neighbour::Neighbour(int macAddress) : macAddress(macAddress) {}

Neighbour::Neighbour(int macAddress, CastaliaModule* castaliaModule)
							: macAddress(macAddress) {}

Neighbour::Neighbour(const int macAddress, CastaliaModule* castaliaModule, simtime_t simtime)
							: macAddress(macAddress) {}

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
	return;
}

simtime_t Neighbour::getTimestamp() const {
	return 0.0;
}
