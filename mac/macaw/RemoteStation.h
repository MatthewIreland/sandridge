/*
 * RemoteStation.h
 *
 *  Created on: Nov 15, 2013
 *      Author: mti20
 *
 *  All update methods return the old value of the parameter they are updating.
 */

#ifndef REMOTESTATION_H_
#define REMOTESTATION_H_

class RemoteStation {
private:
	int localBackoff;
	int remoteBackoff;
	int exchangeSequenceNumber;
	int retryCount;
	bool ack;                // have sent/received ack for latest communication?
public:
	const int macAddress;     // TODO would making remote station list a friend class allow us to keep this private?
	RemoteStation(const int macAddress, const int localBackoff,
				  const int remoteBackoff, const int exchangeSeqNumber,
				  const int retryCount);
	virtual ~RemoteStation();
	int updateLocalBackoff(const int newLocalBackoff);
	inline int getLocalBackoff() const { return localBackoff; };
	int updateRemoteBackoff(const int newRemoteBackoff);
	inline int getRemoteBackoff() const { return remoteBackoff; };
	int updateESN(const int newSequenceNumber);  // returns the new sequence number
	inline int updateSequenceNumber(const int newSequenceNumber) { return updateESN(newSequenceNumber); };
	inline int getESN() const { return exchangeSequenceNumber; };
	int clearRetryCount();
	inline int getRetryCount() const { return retryCount; };
	int incrementRetryCount();       // returns the old retry count
};

inline bool operator==(const RemoteStation& lhs, const RemoteStation& rhs) {return (lhs.macAddress == rhs.macAddress); }
inline bool operator!=(const RemoteStation& lhs, const RemoteStation& rhs) {return !operator==(lhs,rhs);}
inline bool operator< (const RemoteStation& lhs, const RemoteStation& rhs) {return (lhs.macAddress) < (rhs.macAddress); }
inline bool operator> (const RemoteStation& lhs, const RemoteStation& rhs) {return  operator< (rhs,lhs);}
inline bool operator<=(const RemoteStation& lhs, const RemoteStation& rhs) {return !operator> (lhs,rhs);}
inline bool operator>=(const RemoteStation& lhs, const RemoteStation& rhs) {return !operator< (lhs,rhs);}

#endif /* REMOTESTATION_H_ */
