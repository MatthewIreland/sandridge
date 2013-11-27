/*
 * RemoteStationList.cc
 *
 *  Created on: Nov 15, 2013
 *      Author: mti20
 */

#include "RemoteStationList.h"
#include "VirtualMac.h"

RemoteStationList::RemoteStationList(CastaliaModule* castmod) {
	stationList = list<RemoteStation*>();
	cm = castmod;
}

RemoteStationList::~RemoteStationList() {
	// everything's on the stack - nothing to do here :)
}

/*
 *  Helper method to retrieve a station of given address from the list.
 */
RemoteStation* RemoteStationList::find(const int macAddress) const {
	list<RemoteStation*>::const_iterator it;
	for (it = stationList.begin(); it != stationList.end(); ++it) {
	    if ((*it)->macAddress == macAddress)
	    	return *it;
	}
	//throw RemoteStationNotFoundException(macAddress);
}

void RemoteStationList::add(const int macAddress, const int localBackoff,
		const int remoteBackoff, const int exchangeSeqNumber,
		const int retryCount) {
	if (isInList(macAddress)) return;
	RemoteStation* rs = new RemoteStation(macAddress, localBackoff, remoteBackoff,
			exchangeSeqNumber, retryCount);
	stationList.push_back(rs);
}

inline int RemoteStationList::updateLocalBackoff(const int address,
		const int newLocalBackoff) {
	return find(address)->updateLocalBackoff(newLocalBackoff);
}

int RemoteStationList::updateRemoteBackoff(const int address,
		const int newRemoteBackoff) {
	return find(address)->updateRemoteBackoff(newRemoteBackoff);
}

int RemoteStationList::updateSequenceNumber(int macAddress,
		int newSequenceNumber) {
	return find(macAddress)->updateSequenceNumber(newSequenceNumber);
}

int RemoteStationList::clearRetryCount(int macAddress) {
	return find(macAddress)->clearRetryCount();
}

int RemoteStationList::incrementRetryCount(int macAddress) {
	return find(macAddress)->incrementRetryCount();
}

/*
 *  TODO: document
 *  Note this is not implemented using find, since we try and reserve exceptions
 *  for "exceptional circumstances," instead of intentionally throwing and
 *  catching one every time we want to return false.
 */
bool RemoteStationList::isInList(const int macAddress) const {
	bool isInList = false;
	cm->trace() << "In isInList method";
	list<RemoteStation*>::const_iterator it;
	for (it = stationList.begin(); it != stationList.end(); ++it) {
	    if ((*it)->macAddress == macAddress) {
	    	cm->trace() << "Testing MAC " << (*it)->macAddress;
	    	isInList = true;
	    }
	}
	return isInList;
}

int RemoteStationList::getRetryCount(int macAddress) const {
	return find(macAddress)->getRetryCount();
}
