/*
 * RemoteStation.cpp
 *
 *  Created on: Dec 18, 2013
 *      Author: mti20
 */

#include "RemoteStation.h"

RemoteStation::RemoteStation(int macAddress, int localBackoff, int remoteBackoff,
	                         int exchangeSeqNumber, int retryCount) :
	                         macAddress(macAddress), localBackoff(localBackoff),
	                         remoteBackoff(remoteBackoff),
	                         exchangeSequenceNumber(exchangeSeqNumber),
	                         retryCount(retryCount), ack(false) {}

RemoteStation::~RemoteStation() {
	// nothing to do here: all on the stack!
}

/*
 *  Updates the local backoff and returns the old backoff.
 */
int RemoteStation::updateLocalBackoff(int newLocalBackoff) {
	const int oldLocalBackoff = localBackoff;
	localBackoff = newLocalBackoff;
	return oldLocalBackoff;
}

int RemoteStation::updateESN(const int newSequenceNumber) {
	exchangeSequenceNumber = newSequenceNumber;
	return exchangeSequenceNumber;
}

/*
 *  Updates the remote backoff and returns the old backoff.
 */
int RemoteStation::updateRemoteBackoff(int newRemoteBackoff) {
	const int oldRemoteBackoff = remoteBackoff;
	remoteBackoff = newRemoteBackoff;
	return oldRemoteBackoff;
}

/*
 *  TODO: document this.
 *  Resets the ack to false, since if we are going through the initial
 *  handshaking, we shan't have yet had a chance to send or receive an
 *  acknowledgement of the data.
 *
 *  NOTE this returns the new sequence number, not the old one.
 */
//int RemoteStation::updateSequenceNumber(const int newSequenceNumber) {
//	exchangeSequenceNumber = newSequenceNumber;
//	ack=false;
//	return exchangeSequenceNumber;
//}

int RemoteStation::incrementRetryCount() {
	int oldRetryCount = retryCount;
	retryCount++;
	return oldRetryCount;
}

int RemoteStation::clearRetryCount() {
	const int oldRetryCount = retryCount;
	retryCount = 0;
	return oldRetryCount;
}
