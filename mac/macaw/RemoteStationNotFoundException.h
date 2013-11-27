/*
 * RemoteStationNotFoundException.h
 *
 *  Created on: Dec 18, 2013
 *      Author: mti20
 */

#ifndef REMOTESTATIONNOTFOUNDEXCEPTION_H_
#define REMOTESTATIONNOTFOUNDEXCEPTION_H_

class RemoteStationNotFoundException {
public:
	const int address;
	RemoteStationNotFoundException(const int address);
	virtual ~RemoteStationNotFoundException();
};

#endif /* REMOTESTATIONNOTFOUNDEXCEPTION_H_ */
