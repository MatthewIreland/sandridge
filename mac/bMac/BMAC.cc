/**
 *  SMAC.cc
 *  Matthew Ireland, mti20, University of Cambridge
 *  27th November MMXIII
 *
 *  State, timer, and method definitions for the B-MAC protocol implementation.
 *
 */

#include "BMAC.h"
#include "../macBuffer/MacBuffer.h"
#include "../../CastaliaIncludes.h"
#include <cstdlib>

/**
 *  Register the protocol with Castalia.
 */
Define_Module(BMAC);

/**
 *  Entry point to the protocol from the Castalia simulator.
 *  Imports parameters from the .ned file and initialises timers, the state
 *  machine, and internal state.
 */
void BMAC::startup() {
	trace() << "BMAC: starting up!";
	setState(BMAC_STATE_STARTUP);

	// seed random generator with mac address for later use
	srand(SELF_MAC_ADDRESS);

	// initialise parameters from .ned file
	printDebuggingInfo = par("printDebugInfo");
	sendDataEnabled    = par("sendDataEnabled");

	phyDelayForValidCS = par("phyDelayForValidCS");
	phyDataRate        = par("phyDataRate");
	phyFrameOverhead   = par("phyFrameOverhead");

	wakeupDelay        = par("wakeupDelay");

	/* note: when setting parameters, listenPeriod must be at least twice as
	 * large as maxAckDelay, so that transmission can be retried while the
	 * remote node is still awake if no ACK was received
	 */

	listenPeriod          = par("listenPeriod");
	interAckPeriod        = par("interAckPeriod");
	interPreamblePeriod   = interAckPeriod;   // the above was just a silly name for this

	maxDataDelay          = par("maxDataDelay");
	maxDataSendDelay      = par("maxDataSendDelay");
	maxBroadcastDataDelay = par("maxBroadcastDataDelay");

	useComplexIncrementMethod = par("useComplexIncrementMethod");

	sinkMacAddress        = par("sinkMacAddress");

	myBackoff = 0;

	waitForAckTxTime = par("waitForAckTxTime");
	maxAckDelay = par("maxAckDelay");

	preamblePacketLength = par("preamblePacketLength");

	int phyFrameOverhead = par("phyFrameOverhead");
	gapBetweenPreambleAndData = par("dataGap");
	preambleTransmissionTime = (phyFrameOverhead+preamblePacketLength)*1/(1000*phyDataRate/8.0);
	checkPeriod = preambleTransmissionTime - wakeupDelay;
	sendDataTime = preambleTransmissionTime + gapBetweenPreambleAndData;

	trace() << "preamble transmission time: " << preambleTransmissionTime << " seconds.";
	trace() << "Check period: " << checkPeriod << " seconds.";
	trace() << "send data time: " << sendDataTime << " seconds.";

	/* all these outputs refer to packets /successfully/ received, i.e. if we're
	 * in the wrong state when they come in, we won't count them! Overheard
	 * packets don't count. It may be in the future that we want to break them
	 * up, into packets received from individual senders.                     */
	declareOutput("Number of preamble packets sent");
	declareOutput("Number of DATA packets sent");
	declareOutput("Number of acks sent");
	declareOutput("Number of preamble packets received");  // this is purely for debugging and does not make sense in context of the protocol implementation
	declareOutput("Number of DATA packets received");      // number that we've received and /are/ addressed to us
	declareOutput("Number of acks received");

	// initialise internal state
	currentSequenceNumber = 0;
	numRetries = 0;

	trace() << "My MAC address: " << SELF_MAC_ADDRESS;

	macBuffer = new MacBuffer<BMacPacket*>(this, 25, true);

	// ready, set, go!
	setState(BMAC_STATE_SLEEP);
	goToSleep();
	setTimer(BMAC_TIMER_CHECKPERIOD, BMAC_CHECK_PERIOD);

	printInfo("startup complete");
}

/**
 *  Called by the simulator to signal a timer interrupt.
 *  Dispatches flow of control to an appropriate handler method for the timer
 *  that has fired.
 *
 *  @param timer Index of the timer that fired (corresponding to definition
 *               in the enumerated type BmacTimers). Passed in by the simulator.
 */
void BMAC::timerFiredCallback(int timer) {
	trace() << "Timer fired: " << BmacTimerNames[timer];
	switch(timer) {
	case BMAC_TIMER_CHECKPERIOD:   handleCheckPeriodTimerCallback();  break;
	case BMAC_TIMER_WFRADIO_CCA:   handleCheckPeriodTimerCallback_postwakeup();            break;
	case BMAC_TIMER_ACKTIMEOUT:    handleWfAckTimerCallback();        break;
	case BMAC_TIMER_LISTENTIMEOUT: handleListenTimeoutCallback();     break;
	case BMAC_TIMER_RETRY: handleRetryTimerCallback();    break;
	case BMAC_TIMER_RETRYWAKEUP: handleRetryWakeupTimerCallback();    break;
	case BMAC_TIMER_WAITFORPREAMBLE: handleWaitForPreambleTimerCallback(); break;
	default: printNonFatalError("Unrecognised timer callback");       break;
	}
}

void BMAC::handleCheckPeriodTimerCallback() {
	printInfo("check period timer fired");
	if (currentState == BMAC_STATE_SLEEP) {
		wakeUp();
		setState(BMAC_STATE_WFRADIORSSI);
		setTimer(BMAC_TIMER_WFRADIO_CCA, wakeupDelay);
		trace() << "Set timer to do cca in " << wakeupDelay << " seconds.";
	} else {
		trace() << "CHECK PERIOD timer called in non-SLEEP state. Resetting it without side effects.";
		setTimer(BMAC_TIMER_CHECKPERIOD, BMAC_CHECK_PERIOD);
	}
}

void BMAC::handleCheckPeriodTimerCallback_postwakeup() { // consider making inline
	setState(BMAC_STATE_RSSISAMPLE);
	doCCA();
}

void BMAC::doCCA() {
	// should have already been set, but make sure for robustness
	// (if there is an error, it will be picked up by the state transitions
	// test script)
	if (currentState != BMAC_STATE_RSSISAMPLE)
		setState(BMAC_STATE_RSSISAMPLE);

	printInfo("Woken up and radio alive: doing clear channel assessment");

	if (currentState == BMAC_STATE_RSSISAMPLE) {
		// in future we might want to check twice, in case we hit a gap between
		// preamble packets, but this has shown to be sufficient.
		if (isChannelClear(radioModule->readRSSI())) {
			printInfo("Channel is clear. Going back to sleep");
			setTimer(BMAC_TIMER_CHECKPERIOD, BMAC_CHECK_PERIOD);
			setState(BMAC_STATE_SLEEP);
			goToSleep();
		} else {
			trace() << "Channel is not clear: must be a preamble!!!";
			setState(BMAC_STATE_LISTEN);
			setTimer(BMAC_TIMER_LISTENTIMEOUT, listenPeriod);
		}
	} else {
		printNonFatalError("Attempted RSSI in non-CCA state");
	}
}

bool BMAC::isChannelClear(int rssiSample) {
	if (printDebuggingInfo)
		trace() << "Checking channel: " << radioModule->isChannelClear();

	double staticThreshold = -95.0;  // dBm
	return (radioModule->readRSSI() > staticThreshold);

	/*  old reference version:
	CCA_result ccaResult = radioModule->isChannelClear();

	if ((ccaResult!=CLEAR) && (ccaResult!=BUSY)) {
		printFatalError("Not waiting long enough after wakeup for CCA. "
				"Check wakeupDelay parameter");
	}

	return (ccaResult == CLEAR);
	*/
}

void BMAC::handleRetryTimerCallback() {
	setState(BMAC_STATE_PREAMBLE_SEND);
	wakeUp();
	setTimer(BMAC_TIMER_RETRYWAKEUP, wakeupDelay);
}

void BMAC::handleRetryWakeupTimerCallback() {
	sendBufferedDataPacket();
}

/*
 *  Note, this method isn't called if we're doing a broadcast (timer isn't set),
 *  so no need to worry about checking for that case.
 */
void BMAC::handleWfAckTimerCallback() {
	trace() << "in wfack callback";
	//assert(!currentPacketIsBroadcast); // method precondition test
	if (!currentPacketIsBroadcast)		return;
	if (numRetries < BMAC_NUM_RETRIES) {
		/* we still have retries left: try resending it */
		printInfo("Increasing backoff ready for retry");
		setTimer(BMAC_TIMER_RETRY, increaseBackoff());
		goToSleep(false);
	} else {
		/* we've tried the maximum number of times & it hasn't worked :( */
		printNonFatalError("No ACK from neighbour. Data possibly not received.");
		goToSleep();
	}
	return;
}

void BMAC::handleListenTimeoutCallback() {
	/* we set the listen timeout timer because we thought we heard a preamble.
	 * However, since the timer was not cancelled, this was not followed by a
	 * data packet, so we shall simply go back to sleep.
	 */

	printInfo("Listen timed out: no data packets received");
	setTimer(BMAC_TIMER_CHECKPERIOD, checkPeriod);
	setState(BMAC_STATE_SLEEP);
	goToSleep();
	return;
}

/*
 *  unused.
 */
void BMAC::resendData() {
	return;
}

void BMAC::printNonFatalError(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": ERROR: STATE: " << BmacStateNames[currentState] << " DESCRIPTION: " << description << ".";
}


/*
 *  Has the side effect of calling opp_error to stop the simulation.
 */
void BMAC::printFatalError(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": FATAL ERROR: STATE: " << BmacStateNames[currentState] << " DESCRIPTION: " << description << ".";
	opp_error((char*)description.c_str());
}

void BMAC::printInfo(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": INFO STATE: " << BmacStateNames[currentState] << " DESCRIPTION: " << description << ".";
}

void BMAC::printPacketSent(string description) {
	if (printDebuggingInfo)
		trace() << PROTOCOL_NAME << ": INFO: SENT PACKET: " << description << ".";
}

void BMAC::printPacketReceived(string description) {
	trace() << PROTOCOL_NAME << ": INFO: RECEIVED PACKET: " << description << ".";
}

/*
 *  Unimplemented, since noise floor in network is roughly constant, so
 *  software-automatic gain control is unnecessary.
 *  Must be called when the node is awake and the channel is assumed to be
 *  clear!!!
 */
void BMAC::takeRSSIsample() {
	takeRSSIsample(true);
}

/*
 *  Unimplemented, since noise floor in network is roughly constant, so
 *  software-automatic gain control is unnecessary.
 *  Must be called when the node is awake!!!
 */
void BMAC::takeRSSIsample(bool goToSleepAfterSample) {
	/*
	 *  This is where software-automatic gain control would be implemented.
	 */

	if (goToSleepAfterSample)
		goToSleep();
}

/*
 *  Changes to the sleep state, and checks the txbuffer and initiates
 *  transmission if it is non-empty, otherwise turns off the radio.
 *  It is good practice to change to the sleep state before calling this method.
 */
void BMAC::goToSleep() {
	goToSleep(true);
}

void BMAC::goToSleep(bool canCheckTxBuffer) {
	if (canCheckTxBuffer)
		printInfo("Checking Tx buffer before potentially sleeping.");
	else {
		setTimer(BMAC_TIMER_CHECKPERIOD, checkPeriod);
		printInfo("Going to sleep immediately.");
	}

	/* satisfy state transition */
	setState(BMAC_STATE_SLEEP);

	/* if we are sleeping for a reason other than we've overheard something,
	 * check the the tx buffer */
	if (canCheckTxBuffer) {
		trace() << "checking tx buffer";
		if (macBuffer->numPackets() > 0) {
			trace() << "sending buffered data packet";
			sendBufferedDataPacket();
		} else {
			printInfo("Sending sleep command to radio layer");
			toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
			setTimer(BMAC_TIMER_CHECKPERIOD, checkPeriod);
		}
	} else {
		// control flow should never reach here, since the canCheckTxBuffer
		// variable is not changed after call: included for robustness
		printInfo("Sending sleep command to radio layer");
		toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
	}
}

/*
 *  does all the preamble and sending data stuff, using the packet at the head
 *  of the buffer from the network layer.
 */
void BMAC::sendBufferedDataPacket() {
	setState(BMAC_STATE_PREAMBLE_SEND);

	// (this is reset when a data packet is deleted from the buffer)
	numRetries++;
	canSendData = true;

	/* first, ensure we're awake */
	trace() << "in sendBufferedDataPacket method: waking up";
	toRadioLayer(createRadioCommand(SET_STATE, RX));

	// this doesn't need duplicating because we're not sending it to the radio
	// layer in this method
	BMacPacket* macPacket = check_and_cast <BMacPacket*>(macBuffer->peek());
	currentPacketIsBroadcast =
			(macPacket->getDestination() == BROADCAST_MAC_ADDRESS);

	// this might cause problems, and isn't strictly necessary
	// (just slightly cleaner), so can be removed if necessary
	cancelTimer(BMAC_TIMER_CHECKPERIOD);

	/* send preamble to required destination */
	// NOTE: preamble is sent to broadcast address, to emulate a pseudorandom
	// bit stream
	sendPreamble();
	setTimer(BMAC_TIMER_WAITFORPREAMBLE, sendDataTime);
}

/*
 *  The timer is set immediately before the preamble is sent; the calling of
 *  this method signifies that the preamble has been successfully transmitted.
 */
void BMAC::handleWaitForPreambleTimerCallback() {
	printInfo("Preamble sent. Sending data");
	sendDataFromFrontOfBuffer();
	setState(BMAC_STATE_WFACK);
	if (!currentPacketIsBroadcast) {
		/* if the intended receiver was a single node, we need to await an ack
		 * from the destination to signify successful receipt, otherwise resend */
		setTimer(BMAC_TIMER_ACKTIMEOUT, maxAckDelay);
	} else {
		setTimer(BMAC_TIMER_CHECKPERIOD, checkPeriod);
		goToSleep();
	}
}

/*
 *  Note: This does /not/ increment the number of preamble retries.
 *  This must be done in the calling method.
 *  This method does not handle any state transitions. Again, this must be done
 *  by the caller.
 */
void BMAC::sendPreamble() {
	BMacPacket* preamblePacket = new BMacPacket("BMAC preamble packet", MAC_LAYER_PACKET);
	collectOutput("Number of preamble packets sent", SELF_MAC_ADDRESS);
	preamblePacket->setSource(SELF_MAC_ADDRESS);
	preamblePacket->setDestination(BROADCAST_MAC_ADDRESS);
	preamblePacket->setType(BMAC_PACKET_PREAMBLE);
	preamblePacket->setSequenceNumber(currentSequenceNumber);
	preamblePacket->setByteLength(preamblePacketLength);
	printInfo("Sending preamble packet to radio layer");
	toRadioLayer(preamblePacket);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}

/*
 *  This sends the data packet immediately. Note this is /not/ the same as
 *  sendBufferedDataPacket(), which is the method that goes through all the
 *  preamble business. This method does not handle any state transitions.
 *  Again, this must be done by the caller.
 */
void BMAC::sendDataFromFrontOfBuffer() {
	trace() << "in send data from front of buffer method";
	BMacPacket *dataPacket = check_and_cast < BMacPacket * >((macBuffer->peek())->dup());
	collectOutput("Number of DATA packets sent", SELF_MAC_ADDRESS);

	// all these parameters should already be set, but just check them
	dataPacket->setSource(SELF_MAC_ADDRESS);
	dataPacket->setType(BMAC_PACKET_DATA);
	dataPacket->setSequenceNumber(currentSequenceNumber);

	// consider making these two lines an inline function or macro
	printInfo("Sending data packet to radio layer");
	toRadioLayer(dataPacket);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}


/*
 * Note: does not change state! (This would not make sense, since there are
 * multiple transitions from the sleep state.)
 */
void BMAC::wakeUp() {
	printInfo("waking up");
	toRadioLayer(createRadioCommand(SET_STATE, RX));
}

/**
 *  Should only be called if necessary (illegal state or crash), since might
 *  cause us to miss a preamble from neighbour whilst resetting.
 */
void BMAC::reset() {
	goToSleep(false);
	for (int i=0; i<BMAC_NUMBER_OF_TIMERS; i++)
		cancelTimer(i);
}


void BMAC::setState(int newState) {
	/* it is always valid to transition to one's own state */
	if (newState == currentState) {
		return;
	}

	/* print transition if desired */
	if (printDebuggingInfo) {
		trace() << "BMAC TRANSITION: " << BmacStateNames[currentState] << " to " <<
				BmacStateNames[newState] << ".";
	}

	/* make the transition */
	previousState = currentState;
	currentState = newState;
	remoteStation = NO_REMOTE_STATION;
}

void BMAC::setState(int newState, int remoteStation) {
	setState(newState);
	this->remoteStation = remoteStation;
}

/*
 * If we're currently idle and no packets are buffered, go into the sendData
 * routing straight away. Otherwise, simply buffer the packet for later.
 */
void BMAC::fromNetworkLayer(cPacket * netPacket, int destination) {
	trace() << "In network layer method";

	// oddity of the routing layer, not us
	destination = sinkMacAddress;

	if (sendDataEnabled) {
		printInfo("Received a packet from the network layer");

		/* encapsulate the packet from the network layer inside the mac headers */
		BMacPacket* macPacket = new BMacPacket("BMAC data packet", MAC_LAYER_PACKET);
		encapsulatePacket(macPacket, netPacket);
		macPacket->setType(BMAC_PACKET_DATA);
		macPacket->setSource(SELF_MAC_ADDRESS);
		macPacket->setDestination(destination);

		/* buffer the packet. if we are idle and haven't been forced to go to
		 * sleep for some other reason, send it straight away                 */
		trace() << "About to buffer MAC packet. Potential problems!!!";
		macBuffer->insertPacket(macPacket);
		if ((currentState == BMAC_STATE_SLEEP) && macBuffer->numPackets()==1) {
			sendBufferedDataPacket();
		}

	} else {
		printInfo("send disabled: ignoring packet from network layer");
	}
}

/*
 * note we can't actually pay any attention to preamble packets due to the
 * original protocol specification, but we can collect the output for debugging
 * purposes
 */
void BMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi) {
	int source, destination, seqNumber;
	BMacPacket* macPacket = dynamic_cast < BMacPacket* >(pkt);
	if (macPacket == NULL) {
		printNonFatalError("Cast of received packet failed.");
		return;
	}

	source = macPacket->getSource();
	destination = macPacket->getDestination();
	seqNumber = macPacket->getSequenceNumber();

	bool forUs = (destination == SELF_MAC_ADDRESS) ||
			(destination == BROADCAST_MAC_ADDRESS);

	trace() << "Packet received from radio layer! Type: " <<
			macPacket->getType() << ", forUs: " << forUs;

	if (macPacket->getType() == BMAC_PACKET_PREAMBLE) {
		// simply log, as it is probably an error
		collectOutput("Number of preamble packets received", SELF_MAC_ADDRESS);
		return;
	}

	if (currentState == BMAC_STATE_LISTEN) {
		cancelTimer(BMAC_TIMER_LISTENTIMEOUT);
		if (macPacket->getType() == BMAC_PACKET_DATA) {
			if (forUs) {
				printInfo("Received a data packet intended for us.");
				collectOutput("Number of DATA packets received", SELF_MAC_ADDRESS);
				toNetworkLayer(decapsulatePacket(macPacket));
				if (destination == BROADCAST_MAC_ADDRESS) {
					// don't send an ack!
					setTimer(BMAC_TIMER_CHECKPERIOD, BMAC_CHECK_PERIOD);
					// this includes a small delay before we try sending data
					// again :)
					goToSleep(false);
					return;
				} else {
					// this method takes care of going to sleep after sending
					// the ack
					sendAcknowledgement(source, seqNumber);
					return;
				}
			} else {
				printInfo("Overheard a data packet. Going to sleep immediately");
				// this should give them plenty of time for us not to interfere
				// with their ack
				setTimer(BMAC_TIMER_CHECKPERIOD, BMAC_CHECK_PERIOD);
				goToSleep(false);
			}
		} else if (macPacket->getType() == BMAC_PACKET_PREAMBLE) {
			/* we possibly missed the data and the remote station has resent
			 * the preamble. stay awake long enough to hear the data.	 */
			setTimer(BMAC_TIMER_LISTENTIMEOUT, listenPeriod);
		}
		else {
			printNonFatalError("Received entire non-data packet in listen state. Ignoring");
		}
		return;
	} else if (currentState == BMAC_STATE_WFACK) {
		if (macPacket->getType() == BMAC_PACKET_ACK) {
			if (forUs) {
				cancelTimer(BMAC_TIMER_ACKTIMEOUT);
				trace() << "GOT ACK!!! deleting sent packet from buffer.";
				collectOutput("Number of acks received", SELF_MAC_ADDRESS);
				deleteFrontOfBuffer();
				goToSleep(true);

			} else {
				printInfo("Overheard an ACK. Ignoring");
			}
		} else {
			printNonFatalError("Received entire non-ack packet in wfack state. Ignoring");
		}
		return;
	} else {
		printNonFatalError("Packet received from radio layer in wrong state");
	}
}

/*
 *  The method to send an ACK packet, to acknowledge receipt of a DATA packet.
 */
void BMAC::sendAcknowledgement(int source, int seqNumber) {
	int destination = source;   // (otherwise it gets confusing: we want to send the ack to the source of the data packet)
	trace() << "Acknowledging data packet";
	collectOutput("Number of acks sent", SELF_MAC_ADDRESS);
	BMacPacket* ack = new BMacPacket("BMAC ACK packet", MAC_LAYER_PACKET);
	ack->setType(BMAC_PACKET_ACK);
	ack->setSource(SELF_MAC_ADDRESS);
	ack->setDestination(destination);
	trace() << "Sending acknowledgement to radio layer";
	toRadioLayer(ack);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
	setTimer(BMAC_TIMER_WAITFORACKTX, waitForAckTxTime);   // we need to wait a small amount of time before going to sleep after sending the packet, otherwise the radio layer will not be able to complete the transmission
}

void BMAC::handleWaitForAckTxTimerCallback() {
	trace() << "Given radio layer enough time to send ack. Going to sleep.";
	setTimer(BMAC_TIMER_CHECKPERIOD, BMAC_CHECK_PERIOD);
	goToSleep();
}

int BMAC::handleRadioControlMessage(cMessage*) {
	// we don't use control messages, so perfectly safe to do nothing
	return 0;
}

void BMAC::deleteFrontOfBuffer() {
	trace() << "Deleting buffered packet";
	cancelAndDelete(macBuffer->peek());
	macBuffer->removeFirst();
	numRetries=0;
	currentSequenceNumber++;
	resetBackoff();
}

/*
 *  Backoff is the time that we wait for after a failed transmission, before we
 *  try again. It varies between 0 and maxBackoff.
 *  Result is returned in seconds, ready to be be passed into the setTimer(..)
 *  function directly.
 */
inline double BMAC::getCurrentBackoff() const {
	// The internal myBackoff variable is stored in ms, to make manipulating it
	// easier. We return it in seconds, since we need it in seconds for any
	// practical use (e.g. setting a timer).
	return ((double)bmacBackoffs[myBackoff])/1000;
}

/*
 *  Increases the backoff by some random number of increments (between 1 and 5),
 *  as long as it remains less than the maximum backoff specified in the .ned
 *  file, and returns the new backoff.
 */
double BMAC::increaseBackoff() {
	int maxBackoffArrayPosition = BMAC_NUMBER_OF_BACKOFF_INCREMENTS-1;
	trace() << "MAX BACKOFF ARRAY POSITION: " << maxBackoffArrayPosition;
	int rangeToIncreaseBackoff = maxBackoffArrayPosition - myBackoff;
	trace() << "BACKOFF RANGE: " << rangeToIncreaseBackoff;
	if (rangeToIncreaseBackoff == 0)
		return getCurrentBackoff();

	bool useComplexIncrementMethod = false;

	if (useComplexIncrementMethod) {
		int newBackoffArrayPosition = myBackoff+
				int(rangeToIncreaseBackoff*rand()/(RAND_MAX + 1.0));
	} else {
		int newBackoffArrayPosition = myBackoff+1;
		myBackoff = newBackoffArrayPosition;
	}

	trace() << "NEW BACKOFF ARRAY POSITION: " << myBackoff;
	trace() << "BACKOFF VALUE: " << (getCurrentBackoff()*1000) << "ms.";
	return getCurrentBackoff();
}

double BMAC::decreaseBackoff() {
	myBackoff = (myBackoff == 0) ? myBackoff : myBackoff-1;
	return getCurrentBackoff();
}

void BMAC::resetBackoff() {
	myBackoff = 0;
}

