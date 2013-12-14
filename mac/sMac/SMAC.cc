/**
 *  SMAC.cc
 *  Matthew Ireland, mti20, University of Cambridge
 *  14th November MMXIII
 *
 *  State, timer, and method definitions for the S-MAC protocol implementation.
 *
 */


#include "SMAC.h"
#include "../macBuffer/MacBuffer.h"
#include "../../CastaliaIncludes.h"
#include <cstdlib>

/**
 *  Register the protocol with Castalia.
 */
Define_Module(SMAC);

/**
 *  Entry point to the protocol from the Castalia simulator.
 *  Imports parameters from the .ned file and initialises timers, the state
 *  machine, and internal state.
 */
void SMAC::startup() {
	setState(SMAC_STATE_STARTUP);
	trace() << "SMAC: starting up!";

	// seed random generator with mac address for later use
	if (SELF_MAC_ADDRESS == 1)
	srand(SELF_MAC_ADDRESS);

	// initialise parameters from .ned file
	printDebuggingInfo = par("printDebugInfo");
	sendDataEnabled    = par("sendDataEnabled");

	/* all these outputs refer to packets /successfully/ received, i.e. if we're
	 * in the wrong state when they come in, we won't count them! Overheard
	 * packets don't count. It may be in the future that we want to break them
	 * up, into packets received from individual senders.                     */
	declareOutput("Number of DATA packets sent");
	declareOutput("Number of SYNC packets sent");
	declareOutput("Number of acks sent");
	declareOutput("Number of DATA packets received");
	declareOutput("Number of SYNCs received");
	declareOutput("Number of acks received");

	// initialise internal state
	currentSequenceNumber = 0;

	printInfo("Resizing schedule table");
	scheduleTable.resize(MAX_NODES);
	printInfo("Initialising schedule table");
	for (long i=0; i<MAX_NODES; i++) {
		scheduleTable.at(i) = 0;
	}
	printInfo("Schedule table initialised");

	scount = getRandom(int(par("minScount")), int(par("maxScount")));
	numWakeups = 0;
	sinkMacAddress = par("sinkMacAddress");
	isSink = (SELF_MAC_ADDRESS == sinkMacAddress);
	trace() << "Initialised scount to " << scount;

	syncBroadcastTimeMin = par("syncBroadcastTimeMin");
	syncBroadcastTimeMax = par("syncBroadcastTimeMax");

	int syncListenPeriodMs = par("syncListenPeriod");
	int rtsListenPeriodMs = par("rtsListenPeriod");
	int listenSleepPeriodMs = par("listenSleepPeriod");

	syncListenPeriod = double(syncListenPeriodMs)/1000.0;
	rtsListenPeriod = double(rtsListenPeriodMs)/1000.0;
	listenSleepPeriod = double(listenSleepPeriodMs)/1000.0;

	sendTimerMin = par("sendTimerMin");
	sendTimerMax = par("sendTimerMax");

	constOverhead = par("constOverhead");

	overheardRts = false;
	overheardCts = false;

	// initial startup time (seconds)
	int minInitialScheduleWaitTimeMs = par("minInitialScheduleWaitTime");
	int maxInitialScheduleWaitTimeMs = par("maxInitialScheduleWaitTime");
	minInitialScheduleWaitTime = double(minInitialScheduleWaitTimeMs)/1000.0;
	maxInitialScheduleWaitTime = double(maxInitialScheduleWaitTimeMs)/1000.0;

	// timeouts
	int dataTimeoutMs = par("dataTimeout");
	dataTimeout = double(dataTimeoutMs)/1000.0;
	int ctsTimeoutMs = par("ctsTimeout");
	ctsTimeout = double(ctsTimeoutMs)/1000.0;
	int ackTimeoutMs = par("ackTimeout");
	ackTimeout = double(ackTimeoutMs)/1000.0;
	int listenTimeoutMs = par("listenTimeout");
	listenTimeout = double(listenTimeoutMs)/1000.0;

	trace() << "My MAC address: " << SELF_MAC_ADDRESS;

	macBuffer = new MacBuffer<SMacPacket*>(this, 25, true);

	// ... and go ...
	initialisationComplete = false;
	setState(SMAC_STATE_LISTEN_FOR_SCHEDULE);
	setTimer(SMAC_TIMER_BS, getRandomSeconds(minInitialScheduleWaitTime,
			maxInitialScheduleWaitTime));
	trace() << "Set BS timer for " << getTimer(SMAC_TIMER_BS) << "s.";

	printInfo("Startup method complete");
}

/**
 *  Called by the simulator to signal a timer interrupt.
 *  Dispatches flow of control to an appropriate handler method for the timer
 *  that has fired.
 *
 *  @param timer Index of the timer that fired (corresponding to definition
 *               in the enumerated type SmacTimers). Passed in by the simulator.
 */
void SMAC::timerFiredCallback(int timer) {
	trace() << "Timer fired: " << SmacTimerNames[timer]
	        << " in state " << SmacStateNames[currentState];
	switch(timer) {
	case SMAC_TIMER_WAKEUP         :   handleWakeupTimerCallback();
	                                   break;
	case SMAC_TIMER_SYNC_BROADCAST :   handleSyncBroadcastTimerCallback();
	                                   break;
	case SMAC_TIMER_RTS_LISTEN     :   handleRtsListenTimerCallback();
	                                   break;
	case SMAC_TIMER_LISTEN_TIMEOUT :   handleListenTimeoutCallback();
	                                   break;
	case SMAC_TIMER_SEND           :   handleSendTimerCallback();
	                                   break;
	case SMAC_TIMER_BS             :   handleBsTimerCallback();
	                                   break;
	case SMAC_TIMER_CTSREC_TIMEOUT :   handleCtsRecTimeoutTimerCallback();
	                                   break;
	case SMAC_TIMER_DATA_TIMEOUT   :   handleDataTimeoutCallback();
	                                   break;
	case SMAC_TIMER_ACK_TIMEOUT    :   handleAckTimeoutCallback();
	                                   break;
	case SMAC_TIMER_WFBSTX         :   handleWfBsTxTimerCallback();
	                                   break;
	default : printNonFatalError("Unrecognised timer callback");  break;
	}
}

/**
 *  Handler method for SMAC_TIMER_WAKEUP.
 *  Turns the radio on and either
 *     1) moves to the SYNC_BCAST_WAIT state if it's time to send a
 *        SYNC packet, or
 *     2) moves to the LISTEN_FOR_SYNC state directly in all other
 *        circumstances.
 */
void SMAC::handleWakeupTimerCallback() {
	printInfo("Handling WAKEUP timer");
	wakeUp();
	setTimer(SMAC_TIMER_RTS_LISTEN, syncListenPeriod);
	setTimer(SMAC_TIMER_LISTEN_TIMEOUT, double(getListenTimeoutValue())/1000.0);
	if ((--scount) == 0) {
		printInfo("scount is 0. Sending sync");
		setState(SMAC_STATE_SYNC_BCAST_WAIT);

		setTimer(SMAC_TIMER_SYNC_BROADCAST,
				getRandomSeconds(syncBroadcastTimeMin, syncBroadcastTimeMax));
		trace() << "set SB timer for "
				<< getTimer(SMAC_TIMER_SYNC_BROADCAST)
				<< "ms";
	} else {
		setState(SMAC_STATE_LISTEN_FOR_SYNC);
	}

}

/**
 *  Handler method for SMAC_TIMER_RTS_LISTEN.
 *  The timer fires when it is time to transition from the LISTEN_FOR_SYNC
 *  state to the LISTEN_FOR_RTS state.
 *  Additionally, we check the buffer from the layer above, and if we have
 *  a newly generated sensor reading to send, check the buffer before
 *  initiating a handshake.
 */
void SMAC::handleRtsListenTimerCallback() {
	setState(SMAC_STATE_LISTEN_FOR_RTS);
	if (!bufferIsEmpty()) {
		setTimer(SMAC_TIMER_SEND, getRandomSeconds(sendTimerMin, sendTimerMax));
		trace() << "Set SEND timer for " << getTimer(SMAC_TIMER_SEND) << "ms";
	}
}

/**
 *  Handler method for SMAC_TIMER_SYNC_BROADCAST.
 *  The timer was set to a random time after awakening, when it was determined
 *  that a SYNC packet should be sent on this period.
 *  The handler method therefore broadcasts a SYNC packet, resets the number
 *  of periods after which another SYNC packet must be sent to a random value
 *  and ordinarily transitions to the LISTEN_FOR_SYNC state.
 *  Exceptionally, the timer interrupt may have been delayed, so in this case
 *  we may have to transition directly to the LISTEN_FOR_RTS state.
 */
void SMAC::handleSyncBroadcastTimerCallback() {
	printInfo("Handling SYNC BROADCAST timer");
	if (currentState == SMAC_STATE_SYNC_BCAST_WAIT) {
		broadcastSync();
		scount = getRandom(int(par("minScount")), int(par("maxScount")));
		if (getTimer(SMAC_TIMER_RTS_LISTEN) != 0) {
			setState(SMAC_STATE_LISTEN_FOR_SYNC);
		} else {
			setState(SMAC_STATE_LISTEN_FOR_RTS);
		}
	} else {
		printNonFatalError("SYNC BROADCAST timer fired in wrong state. "
				           "Ignoring.");
	}
}

/**
 *  Handler method for SMAC_TIMER_SEND.
 *  Initiates the handshake for sending a newly generated sensor reading.
 */
void SMAC::handleSendTimerCallback() {
	if (currentState == SMAC_STATE_LISTEN_FOR_RTS) {
		sendBufferedDataPacket();
	} else {
		printInfo("Communication is progress or asleep. "
				  "Delaying transmission.");
	}
}

/**
 *  Handler method for SMAC_TIMER_LISTEN_TIMEOUT.
 *  The timer fires when it's the end of our active period and time to go
 *  to sleep. We therefore initiate the process of turning off the radio and
 *  setting the timer for the next wakeup (done in the goToSleep() method), and
 *  transition to the SLEEP state.
 */
void SMAC::handleListenTimeoutCallback() {
	if (currentState == SMAC_STATE_SYNC_BCAST_WAIT) {
		opp_error("LISTEN timer fired in SYNC_BCAST_WAIT state. Fatal.");
	}
	active = false;
	if (currentState == SMAC_STATE_LISTEN_FOR_RTS) {
		setState(SMAC_STATE_SLEEP);
		goToSleep();
	}
}

/**
 *  Handler method for SMAC_TIMER_BS.
 *  The timer is set in the startup() method, and if it fires, it means that
 *  we have not received a schedule from a neighbouring node within the
 *  standard initialisation period, so must create a schedule of our own
 *  and broadcast it on the wireless medium. The method duly performs these
 *  actions and then we transition to the SLEEP state.
 *
 *  To avoid going to sleep prematurely (before the Sync packet has been fully
 *  transmitted by the radio) the WFBSTX timer is set for 0.2ms, and the node
 *  goes to sleep upon its firing.
 */
void SMAC::handleBsTimerCallback() {
	printInfo("BS timer callback... creating schedule");
	createSchedule();

	// (re)broadcast the schedule
	broadcastSync();

	// set timer for next wakeup
	// the getWakeupTimerValue() can optionally be used in place of the
	// constant 0.5s, but it's perfectly correct to do it this way (and adds
	// a bit of safety margin at the beginning), since the 100ms error will be
	// cleaned up by the first call to getListenTimeoutValue() later.
	setTimer(SMAC_TIMER_WAKEUP, 0.5);

	// finish off by going to sleep
	initialisationComplete = true;
	setTimer(SMAC_TIMER_WFBSTX, 0.0002);
}

/**
 *  Handler method for SMAC_TIMER_WFBSTX.
 *  The timer is set in the handleBsTimerCallback() method in order that the
 *  broadcasted SYNC packet might have enough time to be transmitted before
 *  the radio is put to sleep.
 *
 *  This handler method therefore puts the radio to sleep and sets the
 *  state of the state machine accordingly.
 */
void SMAC::handleWfBsTxTimerCallback() {
	setState(SMAC_STATE_SLEEP);
	goToSleep();
}

/**
 *  Handler method for SMAC_TIMER_CTSREC_TIMEOUT.
 *  The timer fires when we time out on not receiving a CTS from a station to
 *  which we have transmitted an RTS, to request that we can transmit one
 *  of our sensor readings.
 *
 *  If the LISTEN_TIMEOUT timer has already fired (i.e. we should be asleep),
 *  then we go to sleep and try sending the packet again when we are next
 *  active (our neighbour to whom we are transmitting the packet will have
 *  also gone to sleep, so retrying immediately would be pointless).
 *
 *  Otherwise, if we should be and are still awake, and the maximum number of
 *  retries have not been exceeded, we retry the transmission (the retry count
 *  is incremented for us in the sendBufferedDataPacket() method).
 *
 *  If the maximum number of retries has been exceeded, then the node to whom
 *  we are trying to transmit the data has most probably died, so we give up
 *  on the transmission.
 */
void SMAC::handleCtsRecTimeoutTimerCallback() {
	if (!active) {
		// try again when we're next active
		numRetries = 0;
		setState(SMAC_STATE_SLEEP);
		goToSleep();
	}
	if (numRetries < maxRetries) {
		// retry
		sendBufferedDataPacket();
	} else {
		// failed
		deleteFrontOfBuffer();
		setState(SMAC_STATE_LISTEN_FOR_RTS);
	}
}

/**
 *  Handler method for SMAC_TIMER_DATA_TIMEOUT.
 *  The timer fires when we time out waiting for a data packet after having
 *  received an RTS and having replied with a CTS to signal that the node can
 *  actually send the DATA packet to us.
 *  If the LISTEN_TIMEOUT timer has already fired (meaning that we should be
 *  asleep, but extended our awake period to try and hear the data), then we
 *  turn off the radio and go to sleep. Otherwise, we go back to the
 *  LISTEN_FOR_RTS state and assume that the sending node will also time out if
 *  a collision or other failure occurred, so will try to send the data again.
 */
void SMAC::handleDataTimeoutCallback() {
	if (!active) {
		setState(SMAC_STATE_SLEEP);
		goToSleep();
	} else {
		setState(SMAC_STATE_LISTEN_FOR_RTS);
	}
}

/**
 *  Handler method for SMAC_TIMER_ACK_TIMEOUT.
 *
 */
void SMAC::handleAckTimeoutCallback() {
	if (!active) {
		// will be retried when we're next active
		numRetries = 0;
		setState(SMAC_STATE_SLEEP);
		goToSleep();
	} else {
		if (numRetries < maxRetries) {
			// retry
			numRetries++;
			setState(SMAC_STATE_WFACK);
			sendDataFromFrontOfBuffer();
		} else {
			// failed
			deleteFrontOfBuffer();
			setState(SMAC_STATE_LISTEN_FOR_RTS);
		}
	}
}

/**
 *  Iterates over the schedule table and returns the largest offset present.
 *  Useful for determining by how much longer than our default active time we
 *  need to stay awake in order that we might overlap with all of our
 *  neighbours' schedules.
 *
 *  @return The largest offset present in the schedule table.
 */
simtime_t SMAC::getMaxScheduleTableOffset() {
	simtime_t maxOffset = 0;
	for (long i=0; i<MAX_NODES; i++) {
		if (scheduleTable.at(i) > maxOffset) maxOffset = scheduleTable.at(i);
	}
	return maxOffset;
}

/**
 *  Iterates over the schedule table and returns the smallest offset present.
 *  Useful for ensuring that we don't wake up before we need to (and
 *  potentially transmit when none of our neighbours are listening, as well as
 *  being wasteful of energy).
 *
 *  @return The smallest offset present in the schedule table.
 */
simtime_t SMAC::getMinScheduleTableOffset() {
	simtime_t minOffset = getMaxScheduleTableOffset();
	for (long i=0; i<MAX_NODES; i++) {
		if (scheduleTable.at(i) < minOffset) minOffset = scheduleTable.at(i);
	}
	return minOffset;
}


/**
 *  Calculates and returns the value that the listen timeout should be set to
 *  upon wakeup (in milliseconds).
 *
 *  The maximum offset from the schedule table is added to the calculated
 *  current frame time to ensure that we don't go to sleep prematurely and
 *  potentially miss overlapping with the schedules of our neighbours.
 *  An adjustment of half the listen-sleep period is necessary since the frame
 *  time refers to awakenings, and here we are interested in when to sleep.
 *
 *  @return the value that the listen timeout should be set to upon wakeup,
 *          in milliseconds.
 */
double SMAC::getListenTimeoutValue() {

	simtime_t currentTime = getClock();
	simtime_t frameTime   = (numWakeups-1)*listenSleepPeriod + primarySchedule;
	simtime_t maxScheduleOffset = getMaxScheduleTableOffset();
	simtime_t listenTermination = frameTime+maxScheduleOffset+0.5*listenSleepPeriod;
	simtime_t listenTime        = listenTermination - currentTime;

	// SIMTIME_DBL is defined in simtime_t.h from OMNeT++
	double listenTimeoutValue = SIMTIME_DBL(listenTime)*1000.0;

	//trace() << "FRAME TIME IS " << frameTime;
	//trace() << "LISTENTIMEOUT should be returning " << listenTimeoutValue;

	return listenTimeoutValue;

}

/**
 *  Value that the wakeup timer should be set to upon sleeping (in ms), i.e.
 *  the next frame time minus the current time. There's no point in waking up
 *  before our neighbours do, so we add the minimum offset in the schedule
 *  table to the wakeup time.
 *
 *  @return the value that the wakeup timer should be set to upon sleeping,
 *          in milliseconds.
 */
double SMAC::getWakeupTimerValue() {

	simtime_t currentTime       = getClock();
	simtime_t frameTime         = numWakeups*listenSleepPeriod
			                                     + primarySchedule;
	simtime_t minScheduleOffset = getMinScheduleTableOffset();
	simtime_t wakeupValue       = frameTime+minScheduleOffset-currentTime;

	//trace() << "FRAME TIME IS " << frameTime;
	//trace() << "WAKEUPTIMERVALUE should be returning " << wakeupValue;

	double wakeupTimerValue = SIMTIME_DBL(wakeupValue)*1000.0;

	return wakeupTimerValue;
}

/**
 *  If debugging information is enabled, writes the given message to the
 *  simulator trace file, together with the protocol name and state.
 */
void SMAC::printNonFatalError(string description) {
	if (printDebuggingInfo) {
		trace() << PROTOCOL_NAME
				<< ": ERROR: STATE: "
				<< SmacStateNames[currentState]
				<< " DESCRIPTION: "
				<< description
				<< ".";
	}
}

/**
 *  If debugging information is enabled, writes the given message to the
 *  simulator trace file, together with the protocol name and state.
 *  Has the side effect of calling opp_error to stop the simulation.
 */
void SMAC::printFatalError(string description) {
	if (printDebuggingInfo) {
		trace() << PROTOCOL_NAME
				<< ": FATAL ERROR: STATE: "
				<< SmacStateNames[currentState]
				<< " DESCRIPTION: "
				<< description
				<< ".";
	}
	opp_error((char*)description.c_str());
}

/**
 *  If debugging information is enabled, writes the given message to the
 *  simulator trace file, together with the protocol name and state.
 *
 *  @param description The specific information that will be printed, alongside
 *                     the protocol name and state.
 */
void SMAC::printInfo(string description) {
	if (printDebuggingInfo) {
		trace() << PROTOCOL_NAME
				<< ": INFO STATE: "
				<< SmacStateNames[currentState]
			    << " DESCRIPTION: "
			    << description
			    << ".";
	}
}

/**
 *  If debugging information is enabled, writes the given message to the
 *  simulator trace file, together with the protocol name and state.
 *
 *  @param description Specific information about the type of packet sent (e.g.
 *                     RTS, CTS, etc), optionally including why it was sent.
 */
void SMAC::printPacketSent(string description) {
	if (printDebuggingInfo) {
		trace() << PROTOCOL_NAME
				<< ": INFO: SENT PACKET: "
				<< description
				<< ".";
	}
}

/**
 *  If debugging information is enabled, writes the given message to the
 *  simulator trace file, together with the protocol name and state.
 *
 *  @param description Specific information about the type of packet received
 *                     (e.g. RTS, CTS, etc), optionally including why/in what
 *                     circumstances it was received.
 */
void SMAC::printPacketReceived(string description) {
	if (printDebuggingInfo) {
		trace() << PROTOCOL_NAME
				<< ": INFO: RECEIVED PACKET: "
				<< description
				<< ".";
	}
}


/**
 *  Returns a random integer between min and max.
 *  Ensure scount is called in the startup() method.
 *  Useful for returning a random number of whole milliseconds between the
 *  specified minimum and maximum.
 *
 *  @return A random integer numerically between the two actual parameters.
 */
int SMAC::getRandom(int min, int max) {
	double range = double(max)-double(min);
	int rnd = int(double(range*double(rand()))/double(double(RAND_MAX) + 1.0));
	trace() << "Generated rnd: " << rnd;
	rnd += min;
	return rnd;
}

/**
 *  Returns a random integer between 0 and RAND_MAX.
 */
int SMAC::getRandom() {
	return getRandom(0, RAND_MAX);
}

/**
 *  Calculates a random integer between min and max, and then divides this
 *  by 1000, returning a double.
 *  This is useful if the parameters are specified in milliseconds, but we
 *  require a random time in seconds.
 *
 *  @param min Minimum random number to be returned (in milliseconds)
 *  @param max Maximum random number to be returned (in seconds)
 *
 *  @return A random time, between min and max, in seconds.
 */
double SMAC::getRandomSeconds(int min, int max) {
	return (double(getRandom(min, max))/1000.0);
}

/*
 *   parameters in s
 */
double SMAC::getRandomSeconds(double min, double max) {
	return (double(getRandom(int(min*1000), int(max*1000)))/1000.0);
}


/**
 *  Turns off the radio and (perhaps redundantly) sets a timer to generate
 *  an interrupt when the node is next due to wake up (i.e. at the next frame
 *  time). The handler for this timer wakes up the node and sets the state
 *  accordingly.
 *
 *  It is considered good practice to change to the sleep state before calling
 *  this method, although not essential since we do that anyway.
 */
void SMAC::goToSleep() {
	/* satisfy state transition */
	setState(SMAC_STATE_SLEEP);

	printInfo("Sending sleep command to radio layer");
	toRadioLayer(createRadioCommand(SET_STATE, SLEEP));

	active = false;
	overheardRts = false;
	overheardCts = false;

	setTimer(SMAC_TIMER_WAKEUP, double(getWakeupTimerValue())/1000.0);

}

/*
 * Note: does not change state! (This would not make sense, since there are
 * multiple transitions from the sleep state.)
 */
void SMAC::wakeUp() {
	printInfo("Waking up");
	toRadioLayer(createRadioCommand(SET_STATE, RX));
	active = true;
	numWakeups++;
}

/*
 *  initiates handshake and does all the sending data stuff, using the packet
 *  at the head of the buffer from the network layer.
 */
void SMAC::sendBufferedDataPacket() {
	setState(SMAC_STATE_WFCTS);

	// this doesn't need duplicating because we're not sending it to the radio
	// layer in this method
	SMacPacket* macPacket = check_and_cast <SMacPacket*>(macBuffer->peek());

	// (this is reset when a data packet is deleted from the buffer)
	numRetries++;

	sendRTS(macPacket->getDestination());
	setTimer(SMAC_TIMER_CTSREC_TIMEOUT, ctsTimeout);
}


/*
 *  This sends the data packet immediately. Note this is /not/ the same as
 *  sendBufferedDataPacket(), which is the method that goes through all the
 *  handshake business. This method does not handle any state transitions.
 *  This must be done by the caller.
 */
void SMAC::sendDataFromFrontOfBuffer() {
	printInfo("In sendDataFromFrontOfBuffer method");
	SMacPacket *dataPacket =
			check_and_cast < SMacPacket * >((macBuffer->peek())->dup());
	collectOutput("Number of DATA packets sent", SELF_MAC_ADDRESS);

	// all these parameters should already be set, but just check them
	dataPacket->setSource(SELF_MAC_ADDRESS);
	dataPacket->setType(SMAC_PACKET_DATA);
	dataPacket->setSequenceNumber(currentSequenceNumber);

	setState(SMAC_STATE_WFACK);
	setTimer(SMAC_TIMER_ACK_TIMEOUT, ackTimeout);

	// consider making these two lines an inline function or macro
	printInfo("Sending data packet to radio layer");
	toRadioLayer(dataPacket);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}

/*
 *  Broadcasts a SYNC packet
 */
void SMAC::broadcastSync() {
	SMacPacket *syncPacket =
			new SMacPacket("SMAC sync packet", MAC_LAYER_PACKET);

	collectOutput("Number of SYNC packets sent", SELF_MAC_ADDRESS);

	simtime_t syncValue = getScheduleValue();
	trace() << "Primary schedule: " << primarySchedule;
	trace() << "Broadcasting sync with schedule value " << syncValue;

	syncPacket->setSource(SELF_MAC_ADDRESS);
	syncPacket->setDestination(BROADCAST_MAC_ADDRESS);
	syncPacket->setType(SMAC_PACKET_SYNC);
	syncPacket->setSequenceNumber(0); // seq number doesn't make sense for SYNCs
	syncPacket->setSyncValue(syncValue);

	printInfo("Sending SYNC packet to radio layer");
	toRadioLayer(syncPacket);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}

/*
 *  primarySchedule stores the time of our first synchronised wakeup.
 *  The schedule table stores positive offsets from this (i.e. the time at which
 *  other nodes wake up).
 *
 *  Offsets of nodes that are not our neighbours, or that we have not yet heard
 *  from, are assumed to be 0.
 *
 *  Arguments: value is from the sync packet.
 */
void SMAC::addSchedule(int source, simtime_t value) {
	printInfo("Adding/updating schedule in schedule table");
	trace() << "  Source: " << source;

	/*  First, update the station's offset from our primarySchedule in the
	 *  schedule table. This is the extra amount by which we must stay awake
	 *  on each cycle. The 'value' argument indicates the time at which the
	 *  source node will next sleep, so we need to convert it to the correct
	 *  schedule table format.                                                */

	simtime_t nextSynchronisedSleep = value;
	//simtime_t nextSynchronisedAwakening = value;
	//trace() << "Other node's next sychronised awakening: "
	//		<< nextSynchronisedAwakening;

	//simtime_t otherNodePrimarySchedule = nextSynchronisedAwakening;
	simtime_t otherNodePrimarySchedule = nextSynchronisedSleep;
	while ((otherNodePrimarySchedule-listenSleepPeriod)>primarySchedule)
		otherNodePrimarySchedule -= listenSleepPeriod;
	otherNodePrimarySchedule -= 0.5*listenSleepPeriod;
	trace() << "Other node's primary schedule value: "
			<< otherNodePrimarySchedule;

	simtime_t newOffset = otherNodePrimarySchedule-primarySchedule;
	if (newOffset<0) newOffset=newOffset*(-1);  // equivalent to doing abs
	trace() << "New offset in schedule table: "
			<< newOffset;

	try {
		scheduleTable.at(source) = newOffset;
	} catch (exception& e) {
		printFatalError("Tried to add node MAC address that exceeds MAX_NODES "
				"to schedule");
	}

	/*  now, if the largest offset in the schedule table is greater than half of
	 *  the listen-sleep period, adjust primarySchedule accordingly.          */
	if (newOffset > (listenSleepPeriod*0.5)) {
		printInfo("New offset is too large. Adjusting schedule table "
				"accordingly");
		simtime_t reductionAmount = newOffset;
		primarySchedule = otherNodePrimarySchedule;
		for (long i=0; i<MAX_NODES; i++) {
			scheduleTable.at(i) = scheduleTable.at(i)-reductionAmount;
		}
	}

	printScheduleTable();
}

/**
 *  Prints the schedule table out to the simulator trace file for debugging
 *  purposes.
 */
void SMAC::printScheduleTable() {
	if (printDebuggingInfo) {
		printInfo("Outputting contents of schedule table");
		for (long i=0; i<MAX_NODES; i++) {
			trace() << "   " << scheduleTable.at(i);
		}
	}
}

/**
 *  Calculates and returns the time at which this node will next sleep, ready
 *  for broadcasting within a SYNC packet.
 *
 *  @return The next sleep time of this node.
 */
simtime_t SMAC::getScheduleValue() {
	printInfo("Getting schedule value");

	simtime_t nextWakeup = numWakeups*listenSleepPeriod + primarySchedule;
	simtime_t nextSleep = nextWakeup + 0.5*listenSleepPeriod;
	simtime_t broadcastedSleep = nextSleep - constOverhead;

	trace() << "  Next wakeup: "            << nextWakeup;
	trace() << "  Actual next sleep: "      << nextSleep;
	trace() << "  Constant overhead: "      << constOverhead;
	trace() << "  Broadcasted next sleep: " << broadcastedSleep;

	return broadcastedSleep;
}

/**
 *  Used initially, in the LISTEN_FOR_SCHEDULE state, when a schedule is
 *  received. Sets the value of the primarySchedule variable so that future
 *  wakeups will be syncrhonised with those of the node from which we received
 *  this initial schedule.
 *
 *  Note that there is no need to update the schedule table here, since all
 *  its entries are initialised to 0, and the whole point of adopting another
 *  node's schedule is that the offsets are 0.
 */
void SMAC::adoptSchedule(int source, simtime_t value) {
	printInfo("Adopting schedule");

	primarySchedule = value-0.5*listenSleepPeriod;

	if (printDebuggingInfo) {
		trace() << "  Primary schedule set to " << primarySchedule;
	}

}

/**
 *  Used initially, in the LISTEN_FOR_SCHEDULE state, if no schedule is
 *  received from a neighbour within the timeout. Calculates the first frame
 *  time and sets the primarySchedule variable accordingly.
 *
 *  As dictated by the state machine (dissertation Figure 3.6), the caller
 *  should go to sleep after using this method to create a schedule, and
 *  wakeup at the next frame time, calculated by this method. This avoids
 *  introducing error by receiving the rebroadcasting of the SYNC packet
 *  that we originally generated (which will likely have had very small errors
 *  introduced in transcription).
 */
void SMAC::createSchedule() {
	printInfo("Creating schedule");

	simtime_t currentTime = getClock();

	// next sleep time (after one full wakeup)
	simtime_t nextWakeup = currentTime + 0.5*listenSleepPeriod;
	primarySchedule = nextWakeup;

	if (printDebuggingInfo) {
		trace() << "  Primary schedule (first wakeup) set to: "
				<< primarySchedule;
	}

}

/**
 *  Resets the protocol by cancelling all timers, creating and broadcasting
 *  a new schedule, and transitioning to the sleep state. It should only be
 *  called if absolutely necessary (i.e. in the case of an illegal state or
 *  crash) since it may cause neighbouring nodes to adopt multiple schedules
 *  and is thus very wasteful of power. It will also cause any transmissions
 *  currently in progress to be aborted, which will also waste power since
 *  these will need to be retried, thus duplicating work that has already been
 *  performed.
 */
void SMAC::reset() {
	goToSleep();
	for (int i=0; i<SMAC_NUMBER_OF_TIMERS; i++)
		cancelTimer(i);
}

/**
 *  Used to perform a state transition from the current state to the one
 *  indicated by the supplied argument. This method must be used whenever
 *  performing a state transition since there is lots of state associated with
 *  doing so. The method also prints the state transition to the simulator trace
 *  file, in the format required by the state transition verification script.
 *
 *  @param newState The state to transition to, whose index is defined by the
 *                  enumerated type SmacStates.
 */
void SMAC::setState(int newState) {
	/* it is always valid to transition to one's own state */
	if (newState == currentState) {
		return;
	}

	/* print transition if desired */
	if (printDebuggingInfo) {
		trace() << "SMAC TRANSITION: " << SmacStateNames[currentState]
		        << " to " << SmacStateNames[newState] << ".";
	}

	/* make the transition */
	previousState = currentState;
	currentState = newState;
}

void SMAC::fromNetworkLayer(cPacket * netPacket, int destination) {
	trace() << "In network layer method";

	// oddity of the routing layer, not us
	destination = sinkMacAddress;

	if (sendDataEnabled) {
		printInfo("Received a packet from the network layer");

		/*encapsulate the packet from the network layer inside the mac headers*/
		SMacPacket* macPacket =
				new SMacPacket("SMAC data packet", MAC_LAYER_PACKET);
		encapsulatePacket(macPacket, netPacket);
		macPacket->setType(SMAC_PACKET_DATA);
		macPacket->setSource(SELF_MAC_ADDRESS);
		macPacket->setDestination(destination);

		/* buffer the packet. if we are idle and haven't been forced to go to
		 * sleep for some other reason, send it straight away                 */
		trace() << "About to buffer MAC packet. Potential problems!!!";
		macBuffer->insertPacket(macPacket);
		if ((currentState == SMAC_STATE_LISTEN_FOR_RTS)
				&& (macBuffer->numPackets() == 1)
				&& (!overheardRts)
				&& (!overheardCts)) {
			/* NB it's OK to send it straight away due to the random delay
			 * in when it was actually generated. 				 */
			sendBufferedDataPacket();
		} else {
			printInfo("Delaying transmission of new packet.");
		}

	} else {
		printInfo("Send disabled: ignoring packet from network layer");
	}
}

void SMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi) {
	int source, destination, seqNumber;
	SMacPacket* macPacket = dynamic_cast < SMacPacket* >(pkt);
	if (macPacket == NULL) {
		printNonFatalError("Cast of received packet failed.");
		return;
	}

	source       = macPacket->getSource();
	destination  = macPacket->getDestination();
	seqNumber    = macPacket->getSequenceNumber();

	bool forUs = (destination == SELF_MAC_ADDRESS) ||
			     (destination == BROADCAST_MAC_ADDRESS);

	trace() << "Packet received from radio layer! Type: "
			<< macPacket->getType() << ", forUs: " << forUs
			<< ", state: " << SmacStateNames[currentState];


	if (forUs && (currentState != SMAC_STATE_SLEEP)) {
		switch (macPacket->getType()) {
		case SMAC_PACKET_SYNC: {
			simtime_t syncValue = macPacket->getSyncValue();
			if (currentState == SMAC_STATE_LISTEN_FOR_SCHEDULE) {
				cancelTimer(SMAC_TIMER_BS);
				initialisationComplete = true;
				adoptSchedule(source, syncValue);
				broadcastSync();
				setTimer(SMAC_TIMER_WAKEUP,
						double(getWakeupTimerValue())/1000.0);
				/*  don't go to sleep immediately to give the schedule time to
				 *  rebroadcast before changing radio state                  */
				setTimer(SMAC_TIMER_WFBSTX, 0.0002);
			} else {
				updateSchedule(source, syncValue);
			}
			break;
		}
		case SMAC_PACKET_RTS : {
			if (currentState == SMAC_STATE_LISTEN_FOR_RTS) {
				cancelTimer(SMAC_TIMER_SEND);
				sendCTS(source, seqNumber);
				/* the following two lines are duplicated from sendCTS
				 * (redundant)                                               */
				setState(SMAC_STATE_WFDATA);
				setTimer(SMAC_TIMER_DATA_TIMEOUT, dataTimeout);
				/* end robust redundancy */
			} else {
				printNonFatalError("Received RTS in non-LISTENFORRTS state. "
						"Ignoring.");
			}
			break;
		}
		case SMAC_PACKET_CTS : {
			if (currentState == SMAC_STATE_WFCTS) {
				cancelTimer(SMAC_TIMER_CTSREC_TIMEOUT);
				sendDataFromFrontOfBuffer();
				setTimer(SMAC_TIMER_ACK_TIMEOUT, ackTimeout);
			} else {
				printNonFatalError("Received CTS in non-WFCTS state. Ignoring");
				return;
			}
			break;
		}
		case SMAC_PACKET_ACK: {
			if (currentState == SMAC_STATE_WFACK) {
				cancelTimer(SMAC_TIMER_ACK_TIMEOUT);
				deleteFrontOfBuffer();
				if (active) {
					setState(SMAC_STATE_LISTEN_FOR_RTS);
					if (!bufferIsEmpty()) {
						setTimer(SMAC_TIMER_SEND, getRandomSeconds(sendTimerMin,
								sendTimerMax));
						trace() << "Set SEND timer for "
								<< getTimer(SMAC_TIMER_SEND)
								<< "ms";
					}
				} else {
					goToSleep();
					setState(SMAC_STATE_SLEEP);
				}
			} else {
				printNonFatalError("Received ACK in non-WFACK state. Ignoring");
			}
			break;
		}
		case SMAC_PACKET_DATA : {
			if (currentState == SMAC_STATE_WFDATA) {
				cancelTimer(SMAC_TIMER_DATA_TIMEOUT);
				toNetworkLayer(decapsulatePacket(macPacket));
				sendAcknowledgement(source, seqNumber);
				if (active) {
					setState(SMAC_STATE_LISTEN_FOR_RTS);
					if (!bufferIsEmpty()) {
						setTimer(SMAC_TIMER_SEND, getRandomSeconds(sendTimerMin,
								sendTimerMax));
						trace() << "Set SEND timer for "
								<< getTimer(SMAC_TIMER_SEND)
								<< "ms";
					}
				} else {
					setState(SMAC_STATE_SLEEP);
					goToSleep();
				}
			} else {
				printNonFatalError("Received DATA in non-WFDATA state. "
						"Ignoring.");
			}
			break;
		}
		default : {
			printNonFatalError("Received unrecognised packet type. Ignoring.");
			return;
		}
		}

	} else if (!forUs) {
		switch (macPacket->getType()) {
		case SMAC_PACKET_RTS  : overheardRts = true;  break;
		case SMAC_PACKET_CTS  : overheardCts = true;  break;
		case SMAC_PACKET_DATA : overheardRts = false; break;
		case SMAC_PACKET_ACK  : overheardCts = false; break;
		default               : printInfo("Overheard packet");
		}
	} else {
		printNonFatalError("Packet received from radio layer in wrong state");
	}
}

/**
 *  Sends an RTS to the specified destination, generally as the first step in
 *  initiating a handshake when there is a sensor reading to be transmitted.
 *  The sequence number included in the RTS packet is that indicated by the
 *  class-local variable currentSequenceNumber, which is incremented upon
 *  deleting a packet from the buffer between the MAC and network layers (and is
 *  initialised to zero upon startup). The state is set (perhaps redundantly)
 *  to WFCTS and a timer is set after which we shall assume that the
 *  transmission has timed out. It is responsibility of the caller to handle
 *  the timeout appropriately.
 *
 *  @param destination MAC address of the station to which the RTS packet
 *                     should be sent.
 */
void SMAC::sendRTS(int destination) {
	trace() << "Sending an RTS to MAC address: " << destination;

	numRetries++;

	collectOutput("Number of RTS packets sent", SELF_MAC_ADDRESS);

	SMacPacket* rts = new SMacPacket("SMAC RTS packet", MAC_LAYER_PACKET);
	rts->setType(SMAC_PACKET_RTS);
	rts->setSource(SELF_MAC_ADDRESS);
	rts->setDestination(destination);
	rts->setSequenceNumber(currentSequenceNumber);

	setTimer(SMAC_TIMER_CTSREC_TIMEOUT, ctsTimeout);

	toRadioLayer(rts);
	toRadioLayer(createRadioCommand(SET_STATE, TX));

	setState(SMAC_STATE_WFCTS);

	trace() << "Finished sending RTS.";
}

/**
 *  Sends a CTS to the specified destination, called in response to receiving
 *  an RTS with the given sequence number. Sets the state (perhaps redundantly)
 *  to WFDATA and sets a timer after which we shall assume that the transmission
 *  has timed out.
 *
 *  @param destination MAC address of the station to which the CTS packet
 *                     should be sent.
 *  @param seqNumber   The sequence number contained within the RTS, to which
 *                     the CTS is a response.
 */
void SMAC::sendCTS(int destination, int seqNumber) {
	trace() << "Sending a CTS to MAC address: " << destination
			<< ". Sequence number: " << seqNumber;

	collectOutput("Number of CTS packets sent", SELF_MAC_ADDRESS);

	SMacPacket* cts = new SMacPacket("SMAC CTS packet", MAC_LAYER_PACKET);
	cts->setType(SMAC_PACKET_CTS);
	cts->setSource(SELF_MAC_ADDRESS);
	cts->setDestination(destination);
	cts->setSequenceNumber(seqNumber);

	setTimer(SMAC_TIMER_DATA_TIMEOUT, dataTimeout);

	toRadioLayer(cts);
	toRadioLayer(createRadioCommand(SET_STATE, TX));

	setState(SMAC_STATE_WFDATA);

	printInfo("Finished sending CTS");
}

/**
 *  Sends an ACK packet to the specified station in the source argument, to
 *  acknowledge receipt of a DATA packet.
 *  This method does not handle any state transitions, and does not modify
 *  the MAC buffer.
 *
 *  @param source    The source of the data packet that we are acknowledging.
 *  @param seqNumber The sequence number of the data packet that we are
 *                   acknowledging.
 */
void SMAC::sendAcknowledgement(int source, int seqNumber) {
	int destination = source;   // (otherwise it gets confusing: we want to
	                            // send the ack to the source of the data
	                            // packet)
	printInfo("Acknowledging data packet");
	collectOutput("Number of acks sent", SELF_MAC_ADDRESS);
	SMacPacket* ack = new SMacPacket("SMAC ACK packet", MAC_LAYER_PACKET);
	ack->setType(SMAC_PACKET_ACK);
	ack->setSource(SELF_MAC_ADDRESS);
	ack->setDestination(destination);
	printInfo("Sending acknowledgement to radio layer");
	toRadioLayer(ack);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}


int SMAC::handleRadioControlMessage(cMessage*) {
	// we don't use control messages, so perfectly safe to do nothing
	return 0;
}

/**
 *  Returns true if and only if we have no newly generated sensor readings in
 *  the buffer from the network layer above.
 *
 *  @return True if the buffer (between the MAC layer and network layer) is
 *          empty; false otherwise.
 */
inline bool SMAC::bufferIsEmpty() {
	return (macBuffer->numPackets() == 0);
}

/**
 *  Deletes the packet at the head of the queue of sensor readings from the
 *  network layer above.
 *
 *  Called either in response to having successfully transmitted this packet to
 *  a neighbour, or because we are forced to give up on transmitting the sensor
 *  reading due to the intended recipient having died.
 *
 *  The method has the side effect of resetting the retry count and
 *  incrementing our monotonically increasing sequence number.
 */
void SMAC::deleteFrontOfBuffer() {
	printInfo("Deleting buffered packet");
	cancelAndDelete(macBuffer->peek());
	macBuffer->removeFirst();
	numRetries=0;
	currentSequenceNumber++;
}



