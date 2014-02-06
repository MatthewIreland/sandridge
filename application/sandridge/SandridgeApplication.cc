/*
 * SandridgeApplication.cpp
 *
 *  Created on: Dec 29, 2013
 *      Author: mti20
 */

#include "SandridgeApplication.h"
#include "../../CastaliaIncludes.h"
#include "SandridgeApplicationPacket_m.h"

Define_Module(SandridgeApplication);

int SandridgeApplication::nextRandomData() {
	return (int)(dataGenerator->getRandom()/RNG_DEFAULT_M*
			(maxSensorValue-minSensorValue)+minSensorValue);
}

int SandridgeApplication::nextRandomTimer() {
	return (int)(dataGenerator->getRandom()/RNG_DEFAULT_M*
			(maxTimeBetweenReadings-minTimeBetweenReadings)+
			minTimeBetweenReadings);
}

void SandridgeApplication::startup() {
	// these are all in seconds
	maxTimeBetweenReadings = par("maxTimeBetweenReadings");
	minTimeBetweenReadings = par("minTimeBetweenReadings");
	maxSensorValue         = par("maxSensorValue");
	minSensorValue         = par("minSensorValue");

	int sensorSeed         = par("sensorSeed");
	int timerSeed          = par("timerSeed");
	int startupDelay       = par("startupDelay");

	dataGenerator  = new SandridgeRandomGenerator(sensorSeed);
	timerGenerator = new SandridgeRandomGenerator(timerSeed);

	packetsSent = 0;

	if (!isSink) {
		setTimer(SR_APPLICATION_TMR_SEND_PACKET, startupDelay+nextRandomTimer());
	}

	declareOutput("Packets received per node");
}

void SandridgeApplication::fromNetworkLayer(ApplicationPacket* rcvPacket,
		const char* source, double rssi, double lqi) {
	SandridgeApplicationPacket* appPacket =
			check_and_cast<SandridgeApplicationPacket*>(rcvPacket);
	int sequenceNumber = appPacket->getSequenceNumber();
	int dataValue      = appPacket->getDataValue();
	if (isSink) {
		trace() << "Received packet #" << sequenceNumber << " from node "
				<< source;
		collectOutput("Packets received per node", atoi(source));
	} else {
		trace() << "Non-sink node was passed a packet.";
	}
}

/* NB this method is never called at the sink node, since the timer is
 * not set :) */
void SandridgeApplication::timerFiredCallback(int index) {
	switch (index) {
	case SR_APPLICATION_TMR_SEND_PACKET: {

		SandridgeApplicationPacket* appPacket =
				new SandridgeApplicationPacket("Sandridge application packet",
						APPLICATION_PACKET);
		appPacket->setSequenceNumber(packetsSent);
		appPacket->setDataValue(nextRandomData());

		toNetworkLayer(appPacket, par("sinkNodeNetAddress"));
		setTimer(SR_APPLICATION_TMR_SEND_PACKET, nextRandomTimer());
	}
	default : {
		trace() << "ERROR! There is only one timer in SandridgeApplication "
				"and another callback has been requested!";
	}
	}
}
