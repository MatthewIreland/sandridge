/*
 * RemoteStationList.h
 *
 *  Created on: Dec 18, 2013
 *      Author: mti20
 */

#ifndef REMOTESTATIONLIST_H_
#define REMOTESTATIONLIST_H_

#include <list>
#include "RemoteStation.h"
#include "RemoteStationNotFoundException.h"
#include "VirtualMac.h"

using namespace std;

class RemoteStationList {
private:
	list<RemoteStation*> stationList;
protected:
	RemoteStation* find(const int address) const;  // return pointer to remote station
	                                               // object with this address
	// don't want people to use operator==() (unimplemented & protected)
	bool operator==(const RemoteStationList& rsl);
	CastaliaModule* cm;
public:
	RemoteStationList(CastaliaModule*);
	virtual ~RemoteStationList();
	void add(const int address, const int localBackoff, const int remoteBackoff,
		 const int exchangeSeqNumber, const int retryCount);
	bool isInList(const int macAddress) const;

	/**** Methods that return the old value ****/
	int updateLocalBackoff(int macAddress, int newLocalBackoff);
	inline int getLocalBackoff(int macAddress) const { return find(macAddress)->getLocalBackoff(); };
	int updateRemoteBackoff(int macAddress, int newRemoteBackoff);
	inline int getRemoteBackoff(int macAddress) const { return find(macAddress)->getRemoteBackoff(); };
	inline int getESN(const int macAddress) const { return  find(macAddress)->getESN(); };
	int clearRetryCount(int macAddress);
	int getRetryCount(int macAddress) const;
	int incrementRetryCount(int macAddress);

	/**** NOTE: this returns the new sequence number, not the old one ****/
	int updateSequenceNumber(int macAddress, int newSequenceNumber);
	inline int updateESN(const int macAddress, int newSequenceNumber) { return updateSequenceNumber(macAddress, newSequenceNumber); };
};

#endif /* REMOTESTATIONLIST_H_ */
