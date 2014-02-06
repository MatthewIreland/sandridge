/*
 * SandridgeApplication.h
 *
 *  Created on: Dec 29, 2013
 *      Author: mti20
 */

#ifndef SANDRIDGEAPPLICATION_H_
#define SANDRIDGEAPPLICATION_H_

#include "VirtualApplication.h"
#include "SandridgeRandomGenerator.h"
#include "SandridgeApplicationPacket_m.h"
#include "../../CastaliaIncludes.h"

using namespace std;

enum SandridgeApplicationTimers {
	SR_APPLICATION_TMR_SEND_PACKET = 1
};

class SandridgeApplication : public VirtualApplication {
private:
	int packetsSent, maxTimeBetweenReadings, minTimeBetweenReadings,
	int maxSensorValue, minSensorValue;
	bool isSink;
	int nextRandomTimer();
	int nextRandomData();
	SandridgeRandomGenerator* dataGenerator;
	SandridgeRandomGenerator* timerGenerator;
protected:
	void startup();
	void fromNetworkLayer(ApplicationPacket *, const char *, double, double);
	void timerFiredCallback(int);
};

#endif /* SANDRIDGEAPPLICATION_H_ */
