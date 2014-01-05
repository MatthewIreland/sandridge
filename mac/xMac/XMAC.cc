/*
 * XMAC.cc
 *
 *  Created on: Jan 5, 2014
 *      Author: mti20
 */

#include "XMAC.h"
#include "../../CastaliaIncludes.h"
#include "../macBuffer/MacBuffer.h"
#include <cstdlib>

Define_Module(XMAC);

void XMAC::startup() {
	trace() << "XMAC: starting up!";

	printNonFatalError("testing the print non fatal error method");

	// seed random generator with mac address for later use
	srand(SELF_MAC_ADDRESS);

	// initialise parameters from .ned file
	printDebuggingInfo    = par("printDebugInfo");
	sendDataEnabled       = par("sendDataEnabled");

	phyDelayForValidCS    = par("phyDelayForValidCS");
	phyDataRate           = par("phyDataRate");
	phyFrameOverhead      = par("phyFrameOverhead");

	wakeupDelay           = par("wakeupDelay");

	listenPeriod          = par("listenPeriod");
	interAckPeriod        = par("interAckPeriod");
	interPreamblePeriod   = interAckPeriod;   // the above was just a silly name for this
	checkPeriod           = par("checkPeriod");
	maxDataDelay          = par("maxDataDelay");
	maxDataSendDelay      = par("maxDataSendDelay");
	maxBroadcastDataDelay = par("maxBroadcastDataDelay");

	sinkMacAddress        = par("sinkMacAddress");

	numberOfPreambles     = par("numberOfPreambles");
	retryPeriod           = par("retryPeriod");

	waitForAckTxTime      = par("waitForAckTxTime");
	maxAckDelay           = par("maxAckDelay");

	trace() << "maxAckDelay: " << maxAckDelay;

	/* all these outputs refer to packets /successfully/ received, i.e. if we're
	 * in the wrong state when they come in, we won't count them! Overheard
	 * packets don't count. It may be in the future that we want to break them
	 * up, into packets received from individual senders.                     */
	declareOutput("Number of preamble packets sent");
	declareOutput("Number of preamble acks sent");
	declareOutput("Number of DATA packets sent");
	declareOutput("Number of acks sent");
	declareOutput("Number of preamble packets received");
	declareOutput("Number of preamble acks received");
	declareOutput("Number of DATA packets received");
	declareOutput("Number of acks received");

	// initialise internal state
	currentSequenceNumber = 0;
	numRetries = 0;

	trace() << "My MAC address: " << SELF_MAC_ADDRESS;

	macBuffer = new MacBuffer<XMacPacket*>(this, 25, true);

	// ready, set, go!
	goToSleep();
	setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);

	printInfo("startup complete");
}

void XMAC::timerFiredCallback(int timer) {
	trace() << "Timer fired: " << XmacTimerNames[timer];
	switch(timer) {
	case XMAC_TIMER_CHECKPERIOD:   handleCheckPeriodTimerCallback();  break;
	case XMAC_TIMER_WFRADIO_CCA:   handleCheckPeriodTimerCallback_postwakeup();            break;
	case XMAC_TIMER_ACKTIMEOUT:    handleWfAckTimerCallback();        break;
	case XMAC_TIMER_LISTENTIMEOUT: handleListenTimeoutCallback();     break;
	case XMAC_TIMER_WFDATATIMEOUT: handleWfDataTimeout();             break;
	case XMAC_TIMER_SENDDATAWAIT:  handleSendDataWaitTimerCallback(); break;
	case XMAC_TIMER_PREAMBLESTROBE: handlePreambleTimerCallback();    break;
	case XMAC_TIMER_RETRY: handleRetryTimerCallback();    break;
	case XMAC_TIMER_RETRYWAKEUP: handleRetryWakeupTimerCallback();    break;
	case XMAC_TIMER_WAITFORACKTX: goToSleep(); break;
	default: printNonFatalError("Unrecognised timer callback");       break;
	}
}

void XMAC::handleCheckPeriodTimerCallback() {
	printInfo("check period timer fired");
	if (currentState == XMAC_STATE_SLEEP) {
		wakeUp();
		doCCA();
	} else {
		trace() << "CHECK PERIOD timer called in non-SLEEP state. Resetting it without side effects.";
		setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
	}
}

void XMAC::handleCheckPeriodTimerCallback_postwakeup() {
	return;
	setState(XMAC_STATE_CCA);
	trace() << "Woken up: doing CCA";
	doCCA();
}

void XMAC::handleRetryTimerCallback() {
	wakeUp();
	setTimer(XMAC_TIMER_RETRYWAKEUP, wakeupDelay);
}

void XMAC::handleRetryWakeupTimerCallback() {
	/* the semantics of what we're doing here may be somewhat confusing. it is
	 * guaranteed that there is a packet in the buffer (otherwise we wouldn't
	 * be here), and the goToSleep method with parameter true has the side
	 * effect of sending whatever is in the buffer, before putting the node to
	 * sleep. */
	sendBufferedDataPacket(); // better clarity by doing this, actually
}

void XMAC::handleWfDataTimeout() {
/*	printInfo("Timed out waiting for data after acknowledging preamble.");
	setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
	goToSleep();*/
}

/* largely to keep same structure as bmac */
void XMAC::doCCA() {
	if (currentState != XMAC_STATE_CCA)  // will have already set, if we use the handleCheckPeriodTimerCallback method
		setState(XMAC_STATE_CCA);
	trace() << "In CCA method";
	if (currentState == XMAC_STATE_CCA) {
		trace() << "About to set state to listen";
		setState(XMAC_STATE_LISTEN);
		setTimer(XMAC_TIMER_LISTENTIMEOUT, listenPeriod);
		trace() << "set timer. waiting for it to fire.";
	} else {
		printNonFatalError("Attempted CCA in non-CCA state");
	}
}

/*
 *  Note, this method isn't called if we're doing a broadcast (timer isn't set),
 *  so no need to worry about checking for that case.
 */
void XMAC::handleWfAckTimerCallback() {
	trace() << "in wfack callback";
	if (numRetries < XMAC_NUM_RETRIES) {
		/* we still have retries left: try resending it */
		sendBufferedDataPacket();
	} else {
		/* we've tried the maximum number of times & it hasn't worked :( */
		printNonFatalError("No ACK from neighbour. Data possibly not received.");
		goToSleep();
	}
	return;
}

void XMAC::handleListenTimeoutCallback() {
	/* idea: need to stay awake long enough to hear at least 2 preamble packets.
	 * if no preamble packet is heard in the time that we're awake, we can be
	 * fairly sure noone is trying to contact us */
	printInfo("Listen timed out: no preambles received");
	trace() << "listen timer: " << getTimer(XMAC_TIMER_LISTENTIMEOUT);
	trace() << "check period timer: " << getTimer(XMAC_TIMER_CHECKPERIOD);

	trace() << "setting check period timer";
	setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
	trace() << "check period timer: " << getTimer(XMAC_TIMER_CHECKPERIOD);

	trace() << "Going to sleep";
	goToSleep();
	trace() << "Returning.";
	return;
}

/*
 * Not used.
 */
bool XMAC::channelIsClear() {
	return false;
}

/*
 *  Not used.
 */
XMacPacket* XMAC::createDataPacket(cPacket* netPacket, int destination) {
	return NULL;
}



/*
 *  Not used.
 */
void XMAC::resendData() {
	return;
}

void XMAC::printNonFatalError(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": ERROR: STATE: " << XmacStateNames[currentState] << " DESCRIPTION: " << description << ".";
}

void XMAC::printFatalError(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": FATAL ERROR: STATE: " << XmacStateNames[currentState] << " DESCRIPTION: " << description << ".";
	//exit(-1);
}

void XMAC::printInfo(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": INFO STATE: " << XmacStateNames[currentState] << " DESCRIPTION: " << description << ".";
}

void XMAC::printPacketSent(string description) {
	if (printDebuggingInfo)
		trace() << PROTOCOL_NAME << ": INFO: SENT PACKET: " << description << ".";
}

void XMAC::printPacketReceived(string description) {
	trace() << PROTOCOL_NAME << ": INFO: RECEIVED PACKET: " << description << ".";
}

/*
 * Note: does change state! (Since the only state we're going to sleep in is
 * the sleep state!)
 */
void XMAC::goToSleep() {
	goToSleep(true);
}

void XMAC::goToSleep(bool canCheckTxBuffer) {
	if (canCheckTxBuffer)
		printInfo("Checking Tx buffer before potentially sleeping.");
	else {
		setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
		printInfo("Going to sleep immediately.");
	}

	/* satisfy state transition */
	setState(XMAC_STATE_SLEEP);

	/* if we are sleeping for a reason other than we've overheard something, check the the tx buffer */
	if (canCheckTxBuffer) {
		trace() << "checking tx buffer";
		if (macBuffer->numPackets() > 0) {
			trace() << "sending buffered data packet";
			sendBufferedDataPacket();
		} else {
			printInfo("Sending sleep command to radio layer");
			toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
			setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
		}
	} else {
		printInfo("Sending sleep command to radio layer");
		toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
	}
}

/*
 *  does all the preamble and sending data stuff, using the packet at the head of the buffer from the network layer.
 */
void XMAC::sendBufferedDataPacket() {
	setState(XMAC_STATE_DATASENDWAIT);
	numRetries++;       // (this is reset when a data packet is deleted from the buffer)
	canSendData = true;
	preambleHasBeenAcked = false;

	/* first, ensure we're awake */
	trace() << "in sendBufferedDataPacket method: waking up";
	toRadioLayer(createRadioCommand(SET_STATE, RX));

	cancelTimer(XMAC_TIMER_CHECKPERIOD);     // this might cause problems, and isn't necessary (just slightly cleaner), so can be removed if necessary

	/* put in a small pause to (primarily) allow radio to wake up and also
	 * (less importantly) randomise slightly                                 */
	double pause = (rand() % maxDataSendDelay) / 1000;
	setTimer(XMAC_TIMER_SENDDATAWAIT, wakeupDelay+pause);
	trace() << "set random timer for " << (wakeupDelay+pause) << " seconds";

	/* see handleSendDataWaitTimerCallback() method for the continuation of this method */
}

void XMAC::handleSendDataWaitTimerCallback() {
	trace() << "in handleSendDataWaitTimerCallback method";

	/* get packet from head of buffer */
	setState(XMAC_STATE_PREAMBLE_SEND);
	// this doesn't need duplicating because we're not sending it to the radio layer in this method
	XMacPacket* macPacket = check_and_cast <XMacPacket*>(macBuffer->peek());
	int destination = macPacket->getDestination();

	/* increment sequence number & send preamble to required destination */
	trace() << "About to send preamble sequence!!! Exciting!!!";
	sendPreamble(destination, ++currentSequenceNumber);
	numberOfPreambleRetries = 1;
	setTimer(XMAC_TIMER_PREAMBLESTROBE, interPreamblePeriod);
}

/* note this method will /not/ be called if the preamble has been acked!! */
void XMAC::handlePreambleTimerCallback() {
	//trace() << "Preamble strobe method called (i.e. number of preambles is currently 2 or greater, and another is about to be sent.";
	XMacPacket* macPacket = check_and_cast <XMacPacket*>(macBuffer->peek());
	int destination = macPacket->getDestination();
	if ((numberOfPreambleRetries++) < numberOfPreambles) {
		trace() << "Sending preamble number " << numberOfPreambleRetries;
		/* send next strobe in the preamble sequence */
		sendPreamble(destination, currentSequenceNumber);
		setTimer(XMAC_TIMER_PREAMBLESTROBE, interPreamblePeriod);
	} else {
		/* if it's a broadcast packet, we weren't expecting a reply to the preamble */
		if (destination == BROADCAST_MAC_ADDRESS) {
			sendDataFromFrontOfBuffer();
			deleteFrontOfBuffer();
			goToSleep();
			return;
		}
		/* preamble sequence finished & no reply received: if we have exceeded our maximum number of retries, give up; otherwise, wait some amount of time and try again */
		printInfo("No response to preamble sequence. Perhaps destination node has died or is intentionally ignoring us.");
		if (numRetries >= XMAC_NUM_RETRIES) {
			trace() << "Exceeded maximum number of retries, giving up.";
			deleteFrontOfBuffer();
			goToSleep();
		} else {
			/* keep quiet before we retry, in case we're causing interference with another node */
			trace() << "Retry number: " << numRetries;
			setTimer(XMAC_TIMER_RETRY, retryPeriod+(rand() % 200)/1000);
			toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
		}
	}
}

/*
 *  Note: This does /not/ increment the number of preamble retries. This must be done in the calling method.
 *  This method does not handle any state transitions. Again, this must be done by the caller.
 */
void XMAC::sendPreamble(int destination, int sequenceNumber) {
	XMacPacket* preamblePacket = new XMacPacket("XMAC preamble packet", MAC_LAYER_PACKET);
	collectOutput("Number of preamble packets sent", SELF_MAC_ADDRESS);
	preamblePacket->setSource(SELF_MAC_ADDRESS);
	preamblePacket->setDestination(destination);
	preamblePacket->setType(XMAC_PACKET_PREAMBLE);
	preamblePacket->setSequenceNumber(currentSequenceNumber);
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
void XMAC::sendDataFromFrontOfBuffer() {
	trace() << "in send data from front of buffer method";
	XMacPacket *dataPacket = check_and_cast < XMacPacket * >((macBuffer->peek())->dup());
	collectOutput("Number of DATA packets sent", SELF_MAC_ADDRESS);
	// all these parameters should already be set, but just check them
	dataPacket->setSource(SELF_MAC_ADDRESS);
	dataPacket->setType(XMAC_PACKET_DATA);
	dataPacket->setSequenceNumber(currentSequenceNumber);
	trace() << "heree";
	printInfo("Sending data packet to radio layer");
	toRadioLayer(dataPacket);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
	trace() << "returning from sendDataFromFrontOfBuffer()";
}


/*
 * Note: does not change state! (This would not make sense, since there are
 * multiple transitions from the sleep state.)
 */
void XMAC::wakeUp() {
	printInfo("waking up");
	toRadioLayer(createRadioCommand(SET_STATE, RX));
}

/*
 *  Should only be called if necessary (illegal state or crash), since might
 *  cause us to miss a preamble from neighbour whilst resetting.
 */
void XMAC::reset() {
	goToSleep(false);
	for (int i=0; i<XMAC_NUMBER_OF_TIMERS; i++)
		cancelTimer(i);
}


void XMAC::setState(int newState) {
	/* it is always valid to transition to one's own state */
	if (newState == currentState) {
		return;
	}

	/* print transition if desired */
	if (printDebuggingInfo) {
		trace() << "XMAC TRANSITION: " << XmacStateNames[currentState] << " to " <<
				XmacStateNames[newState] << ".";
	}

	/* make the transition */
	previousState = currentState;
	currentState = newState;
	remoteStation = NO_REMOTE_STATION;
}

void XMAC::setState(int newState, int remoteStation) {
	setState(newState);
	this->remoteStation = remoteStation;
}

/*
 * If we're currently idle and no packets are buffered, go into the sendData
 * routing straight away. Otherwise, simply buffer the packet for later.
 */
void XMAC::fromNetworkLayer(cPacket * netPacket, int destination) {
	trace() << "In network layer method";
	if (sendDataEnabled) {
		printInfo("Received a packet from the network layer");

		/* encapsulate the packet from the network layer inside the mac headers */
		XMacPacket* macPacket = new XMacPacket("XMAC data packet", MAC_LAYER_PACKET);
		encapsulatePacket(macPacket, netPacket);
		macPacket->setType(XMAC_PACKET_DATA);
		macPacket->setSource(SELF_MAC_ADDRESS);
		//macPacket->setDestination(destination);
		macPacket->setDestination(sinkMacAddress);     // an error/oddity of the routing layer, not us

		/* buffer the packet. if we are idle and haven't been forced to go to
		 * sleep for some other reason, send it straight away                 */
		trace() << "About to buffer MAC packet. Potential problems!!!";
		macBuffer->insertPacket(macPacket);
		if ((currentState == XMAC_STATE_SLEEP) && macBuffer->numPackets()==1) {
			sendBufferedDataPacket();
		}

	} else {
		printInfo("send disabled: ignoring packet from network layer");
	}
}

void XMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi) {
	int source, destination, seqNumber;
	XMacPacket* macPacket = dynamic_cast < XMacPacket* >(pkt);
	if (macPacket == NULL) {
		printNonFatalError("Cast of received packet failed.");
		return;
	}

	source = macPacket->getSource();
	destination = macPacket->getDestination();
	seqNumber = macPacket->getSequenceNumber();

	bool forUs = (destination == SELF_MAC_ADDRESS) || (destination == BROADCAST_MAC_ADDRESS);

	trace() << "Packet received from radio layer! Type: " << macPacket->getType() << ", forUs: " << forUs;

	if (currentState == XMAC_STATE_LISTEN || currentState == XMAC_STATE_WFDATA) {
		/* attempt to decode a preamble packet */
		if (macPacket->getType() == XMAC_PACKET_PREAMBLE) {
			cancelTimer(XMAC_TIMER_LISTENTIMEOUT);
			collectOutput("Number of preamble packets received", SELF_MAC_ADDRESS);
			canSendData = false;
			if (forUs) {
				if (destination != BROADCAST_MAC_ADDRESS) {
					trace() << "Received a preamble addressed to us! Acknowledging.";
					trace() << "Preamble source: " << source;
					trace() << "Preamble destination: " << destination;
					sendPreambleAck(source, seqNumber);
					return;
				} else {
					trace() << "Heard broadcast preamble. Moving to WFDATATIMEOUT state, but not acknowledging.";
					setTimer(XMAC_TIMER_WFDATATIMEOUT, maxBroadcastDataDelay);
					return;
				}
				setState(XMAC_STATE_WFDATA);
			} else {  /* overheard */
				trace() << "Overheard a preamble. Going to sleep.";
				setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
				goToSleep(false);
				return;
			}
		} else {
			if (currentState == XMAC_STATE_LISTEN) {
				printNonFatalError("Attempted to decode non-preamble packet in XMAC_STATE_LISTEN");
				return;
			}
		}
	}
	if (currentState == XMAC_STATE_WFDATA) {
		if (forUs && (macPacket->getType() == XMAC_PACKET_DATA)) {
			collectOutput("Number of DATA packets received", SELF_MAC_ADDRESS);
			toNetworkLayer(decapsulatePacket(macPacket));
			if (destination != BROADCAST_MAC_ADDRESS) {
				sendDataAcknowledgement(source, seqNumber);
			}
			return;
		}
	}
	if (currentState == XMAC_STATE_PREAMBLE_SEND) {
		trace() << "Received a packet in the XMAC_STATE_PREAMBLE_SEND state";

		cancelTimer(XMAC_TIMER_PREAMBLESTROBE);
		trace() << "Cancelled strobe timer";

		if (forUs && (macPacket->getType() == XMAC_PACKET_PREAMBLE_ACK)) {
trace() << "The preamble ack was intended for us";

			// could check sequence numbers match here, but not a lot of point
			collectOutput("Number of preamble acks received", SELF_MAC_ADDRESS);
			numberOfPreambleRetries = 0;   // yes, this is intentionally duplicated below, for now
			sendDataFromFrontOfBuffer();

			trace() << "acktimeout timer: " << getTimer(XMAC_TIMER_ACKTIMEOUT);
			setTimer(XMAC_TIMER_ACKTIMEOUT, maxAckDelay);

			trace() << "acktimeout timer: " << getTimer(XMAC_TIMER_ACKTIMEOUT);
			trace() << "setting state to wfack";
			setState(XMAC_STATE_WFACK);
			return;
		} else {
			/* we overheard it, go to sleep to avoid causing interference, and try sending the packet again later */
			//printInfo("Overheard preamble acknowledgement");
			trace() << "Overheard preamble acknowledgement. destination: " << destination;
			numberOfPreambleRetries = 0;    // perhaps also reset numRetries, see how it goes :)
			setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
			goToSleep(false);
			return;
		}
	}
	if (currentState == XMAC_STATE_WFACK){
		if (forUs && (macPacket->getType() == XMAC_PACKET_ACK)) {
			trace() << "GOT ACK!!! deleting sent packet from buffer.";
			collectOutput("Number of acks received", SELF_MAC_ADDRESS);
			cancelTimer(XMAC_TIMER_ACKTIMEOUT);
			deleteFrontOfBuffer();
			goToSleep(true);
			return;
		}
	}
}

/*
 *  The method to interrupt a strobed preamble, and request the transmission of
 *  a DATA packet to us.
 */
void XMAC::sendPreambleAck(int source, int seqNumber) {
	int destination = source;   // (otherwise it gets confusing: we want to send the ack to the source of the preamble)
	trace() << "Acknowledging preamble";
	trace() << "Sending preamble ack to MAC address: " << destination;
	collectOutput("Number of preamble acks sent", SELF_MAC_ADDRESS);
	setTimer(XMAC_TIMER_WFDATATIMEOUT, maxDataDelay);
	setState(XMAC_STATE_WFDATA);
	XMacPacket* preambleAck = new XMacPacket("XMAC preamble ack packet", MAC_LAYER_PACKET);
	preambleAck->setType(XMAC_PACKET_PREAMBLE_ACK);
	preambleAck->setSource(SELF_MAC_ADDRESS);
	preambleAck->setDestination(destination);
	preambleAck->setSequenceNumber(seqNumber);
	toRadioLayer(preambleAck);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}

/*
 *  The method to send an ACK packet, to acknowledge receipt of a DATA packet.
 */
void XMAC::sendDataAcknowledgement(int source, int seqNumber) {
	int destination = source;   // (otherwise it gets confusing: we want to send the ack to the source of the data packet)
	cancelTimer(XMAC_TIMER_WFDATATIMEOUT);
	trace() << "Acknowledging data packet";
	collectOutput("Number of acks sent", SELF_MAC_ADDRESS);
	XMacPacket* ack = new XMacPacket("XMAC ACK packet", MAC_LAYER_PACKET);
	ack->setType(XMAC_PACKET_ACK);
	ack->setSource(SELF_MAC_ADDRESS);
	ack->setDestination(destination);
	trace() << "Sending acknowledgement to radio layer";
	toRadioLayer(ack);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
	setTimer(XMAC_TIMER_WAITFORACKTX, waitForAckTxTime);   // we need to wait a small amount of time before going to sleep after sending the packet, otherwise the radio layer will not be able to complete the transmission
}

void XMAC::handleWaitForAckTxTimerCallback() {
	trace() << "Given radio layer enough time to send ack. Going to sleep.";
	setTimer(XMAC_TIMER_CHECKPERIOD, XMAC_CHECK_PERIOD);
	goToSleep();
}

int XMAC::handleRadioControlMessage(cMessage*) {
	// control messages are not used.
	return 0;
}

void XMAC::deleteFrontOfBuffer() {
	trace() << "Deleting buffered packet";
	cancelAndDelete(macBuffer->peek());
	macBuffer->removeFirst();
	numRetries=0;
}

