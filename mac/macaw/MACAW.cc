/**
 *  MACAW.cc
 *  Matthew Ireland, mti20, University of Cambridge
 *  4th November MMXIII
 *
 *  State, timer, and method definitions for the MACAW protocol implementation.
 *
 */

#include "MACAW.h"
#include "../macBuffer/MacBuffer.h"
#include "../../CastaliaIncludes.h"
#include <cstdlib>

/**
 *  Register the protocol with Castalia.
 */
Define_Module(MACAW);

/**
 *  Entry point to the protocol from the Castalia simulator.
 *  Imports parameters from the .ned file and initialises timers, the state
 *  machine, and internal state.
 */
void MACAW::startup() {
	trace() << "MACAW: starting up!";

	// seed random generator with mac address for later use
	srand(SELF_MAC_ADDRESS*int(SIMTIME_DBL(getClock())*1000.0+1.0));

	// initialise parameters from .ned file
	printDebuggingInfo = par("printDebugInfo");
	sendDataEnabled    = par("sendDataEnabled");

	phyDelayForValidCS = par("phyDelayForValidCS");
	phyDataRate        = par("phyDataRate");
	phyFrameOverhead   = par("phyFrameOverhead");

	sinkMacAddress     = par("sinkMacAddress");
	sendDataEnabled    = par("sendDataEnabled");

	// min value for a random timer (e.g. IDLE->CONTEND), in ms
	defaultMinRndTimerValue = par("defaultMinRndTimerValue");
	// max value for a random timer (e.g. IDLE->CONTEND), in ms
	defaultMaxRndTimerValue = par("defaultMaxRndTimerValue");

	myBackoff         = par("initialBackoff");
	backoffResetValue = myBackoff;
	backoffDecrement  = par("backoffDecrement");

	int msWfdsTimeout             = par("maxWfdsTimeout");
	maxWfdsTimeout                = ((double)msWfdsTimeout)/1000;
	int msAckTimeout              = par("maxAckTimeout");
	maxAckTimeout                 = ((double)msAckTimeout)/1000;
	int msDataTimeout             = par("maxDataTimeout");
	maxDataTimeout                = ((double)msDataTimeout)/1000;
	int msCtsTimeout              = par("maxCtsTimeout");
	maxCtsTimeout                 = ((double)msCtsTimeout)/1000;
	int msOverheardRts            = par("overheardRtsTime");
	overheardRtsTime              = ((double)msOverheardRts)/1000;
	int msOverheardDs             = par("overheardDsTime");
	overheardDsTime               = ((double)msOverheardDs)/1000;
	int msOverheardRtsCtsExchange = par("overheardRtsCtsExchange");
	overheardRtsCtsExchange       = ((double)msOverheardRtsCtsExchange)/1000;

	// throws an error if these are uninitialised
	trace() << maxWfdsTimeout
			<< maxAckTimeout
			<< maxDataTimeout
			<< maxCtsTimeout
			<< overheardRtsTime
			<< overheardDsTime
			<< overheardRtsCtsExchange;

	/* all these outputs refer to packets /successfully/ received, i.e. if we're
	 * in the wrong state when they come in, we won't count them! Overheard
	 * packets don't count. It may be in the future that we want to break them
	 * up, into packets received from individual senders.                     */
	declareOutput(COLLECT_RTS_SENT_STRING);
	declareOutput(COLLECT_CTS_SENT_STRING);
	declareOutput(COLLECT_DS_SENT_STRING);
	declareOutput(COLLECT_DATA_SENT_STRING);
	declareOutput(COLLECT_ACK_SENT_STRING);
	declareOutput(COLLECT_RRTS_SENT_STRING);
	declareOutput(COLLECT_RTS_RECEIVED_STRING);
	declareOutput(COLLECT_CTS_RECEIVED_STRING);
	declareOutput(COLLECT_DS_RECEIVED_STRING);
	declareOutput(COLLECT_DATA_RECEIVED_STRING);
	declareOutput(COLLECT_ACK_RECEIVED_STRING);
	declareOutput(COLLECT_RRTS_RECEIVED_STRING);

	// initialise internal state
	currentSequenceNumber = 0;
	numRetries = 0;

	interSendPause = par("interSendPause");
	maxRetries = par("maxRetries");

	trace() << "My MAC address: " << SELF_MAC_ADDRESS;

	// ready, set, go!
	setState(MACAW_STATE_IDLE);

	printInfo("startup complete");


	trace() << "backoff: " << getMyBackoff();
	trace() << "in ms: " << getBackoffInMs();
	trace() << "increase: " << increaseBackoff();
	trace() << "decrease: " << decreaseBackoff();
	trace() << "reset: " << resetBackoff();


	remoteStationList = new RemoteStationList(this);
	macBuffer         = new MacBuffer<MacawPacket*>(this, 25, true);
}

/**
 *  Called by the simulator to signal a timer interrupt.
 *  Dispatches flow of control to an appropriate handler method for the timer
 *  that has fired.
 *
 *  @param timer Index of the timer that fired (corresponding to definition
 *               in the enumerated type MacawTimers). Passed in by the
 *               simulator.
 */
void MACAW::timerFiredCallback(int timer) {
	trace() << "Timer fired: " << MacawTimerNames[timer];
	switch(timer) {
	case MACAW_TIMER_CONTEND : handleContendTimerCallback(); break;
	case MACAW_TIMER_WFDS_TIMEOUT : handleWfdsTimeoutTimerCallback(); break;
	case MACAW_TIMER_QUIET : handleQuietTimerCallback(); break;
	case MACAW_TIMER_SEND_PAUSE: handleSendPauseTimerCallback(); break;
	case MACAW_TIMER_DSDELAY: 	handleDsTimerCallback(); break;
	case MACAW_TIMER_CTS_TIMEOUT :
	case MACAW_TIMER_WFDATA_TIMEOUT:
	case MACAW_TIMER_ACK_TIMEOUT : handleGenericTimeoutCallback(); break;
	default: printNonFatalError("Unrecognised timer callback");       break;
	}
}

void MACAW::handleSendPauseTimerCallback() {
	if (currentState == MACAW_STATE_IDLE)
		sendBufferedDataPacket();
}

void MACAW::handleContendTimerCallback() {
	sendRTS(remoteStation, ++currentSequenceNumber);
	setTimer(MACAW_TIMER_CTS_TIMEOUT, maxCtsTimeout);
}

void MACAW::handleDsTimerCallback() {
	trace() << "in ds method...";

	MacawPacket* dataPacket =
			check_and_cast <MacawPacket*>((macBuffer->peek())->dup());

	int destination = dataPacket->getDestination();
	trace() << "destination: " << destination;

	// construct data packet
	dataPacket->setDestination(destination);
	dataPacket->setSource(SELF_MAC_ADDRESS);
	dataPacket->setType(MACAW_PACKET_DATA);
	dataPacket->setLocalBackoff(localBackoff);
	dataPacket->setRemoteBackoff(remoteStationList->getRemoteBackoff(destination));
	dataPacket->setRetryCount(remoteStationList->getRetryCount(destination));
	dataPacket->setSequenceNumber(currentSequenceNumber);

	COLLECT_DATA_SENT
	toRadioLayer(dataPacket);   // sending it down here avoid accidental manipulation of fields below
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}

void MACAW::handleWfdsTimeoutTimerCallback() {
	// legacy
}

void MACAW::handleQuietTimerCallback() {
	if (currentState == MACAW_STATE_WFCONTEND) {
		// timeout rule 1
		setTimer(MACAW_TIMER_CONTEND, getRandomTimerValue());
		setState(MACAW_STATE_CONTEND, remoteStation);
	} else {
		// timeout rule 2
		if (currentState == MACAW_STATE_CONTEND) {
			/* this is where RRTS send would be implemented */
		}
		// timeout rule 3
		setState(MACAW_STATE_IDLE);
	}
}

void MACAW::handleGenericTimeoutCallback() {
	/* increase Q's backoff */
	int trialBackoff = (int)(1.5*(double)localBackoff);
	trialBackoff = (trialBackoff>maxBackoff) ? maxBackoff : trialBackoff;
	remoteStationList->updateRemoteBackoff(remoteStation,trialBackoff);

	/* deal with case where we've exceeded the maximum number of retries */
	if (remoteStationList->getRetryCount(remoteStation) > maxRetries) {
		localBackoff = maxBackoff;
		remoteStationList->updateRemoteBackoff(remoteStation, REMOTE_BACKOFF_UNKNOWN);
		deleteFrontOfBuffer();  // we failed, perhaps the node has died
	}

	/* go to idle state without removing failed packet from buffer */
	setState(MACAW_STATE_IDLE);
}

void MACAW::printNonFatalError(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": ERROR: STATE: " << MacawStateNames[currentState]
	        << " DESCRIPTION: " << description << ".";
}

void MACAW::printFatalError(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": FATAL ERROR: STATE: "
	        << MacawStateNames[currentState] << " DESCRIPTION: " << description
	        << ".";
	//exit(-1);
}

void MACAW::printInfo(string description) {
	if (printDebuggingInfo)
	trace() << PROTOCOL_NAME << ": INFO STATE: "
	        << MacawStateNames[currentState] << " DESCRIPTION: "
	        << description << ".";
}

void MACAW::printPacketSent(string description) {
	if (printDebuggingInfo)
		trace() << PROTOCOL_NAME << ": INFO: SENT PACKET: "
		        << description << ".";
}

void MACAW::printPacketReceived(string description) {
	trace() << PROTOCOL_NAME << ": INFO: RECEIVED PACKET: "
			<< description << ".";
}

/*
 *  See message exchange/control rule 1. Sets a random timer and goes to the
 *  CONTEND state.
 */
void MACAW::sendBufferedDataPacket() {
	trace() << "In sendBufferedDataPacket";

	if (numRetries == 0) {
		currentSequenceNumber++;
	}

	// get front of buffer so we can extract the destination
	MacawPacket* bufferFront = check_and_cast <MacawPacket*>(macBuffer->peek());  // this doesn't need duplicating because we're not sending it to the radio layer in this method

	setTimer(MACAW_TIMER_CONTEND, getRandomTimerValue());
	setState(MACAW_STATE_CONTEND, bufferFront->getDestination());
}

// args in ms, returns in seconds
inline double MACAW::getRandomTimerValue(const int min, const int max) const {
	return ((double)((rand() % (max-min)) + min))/1000;
}

// returns timer value in seconds
inline double MACAW::getRandomTimerValue() const {
	return getRandomTimerValue(defaultMinRndTimerValue, defaultMaxRndTimerValue);
}


void MACAW::reset() {
	setState(MACAW_STATE_IDLE);
	for (int i=0; i<MACAW_NUMBER_OF_TIMERS; i++)
		cancelTimer(i);
}

/*
 *  Method called to perform a state transition.
 *  If we are transitioning to the IDLE state, we will check the transmission
 *  buffer and try to initiate a transmission if it is non-empty.
 */
void MACAW::setState(int newState) {
	/* it is always valid to transition to one's own state */
	if (newState == currentState) {
		return;
	}

	/* print transition if desired */
	if (printDebuggingInfo) {
		trace() << "MACAW TRANSITION: " << MacawStateNames[currentState] << " to " <<
				MacawStateNames[newState] << ".";
	}

	/* make the transition */
	previousState = currentState;
	currentState = newState;
	remoteStation = NO_REMOTE_STATION;

	if (currentState == MACAW_STATE_IDLE) {
		if (macBuffer->numPackets() > 0) {
			/* put in a small pause to avoid possible domination */
			setTimer(MACAW_TIMER_SEND_PAUSE, interSendPause);
			/* if we are still in the IDLE state after the timer expires,
			 * sendBufferedDataPacket() is called                             */
		}
	}
}

void MACAW::setState(int newState, int remoteStation) {
	setState(newState);
	this->remoteStation = remoteStation;
}

void MACAW::sendRTS(int destination, int seqNumber) {
	trace() << "Sending an RTS to MAC address: " << destination;

	localBackoff = myBackoff;
	COLLECT_RTS_SENT

	MacawPacket* rts = new MacawPacket("MACAW RTS packet", MAC_LAYER_PACKET);
	rts->setType(MACAW_PACKET_RTS);
	rts->setSource(SELF_MAC_ADDRESS);
	rts->setDestination(destination);
	rts->setRemoteBackoff(remoteStationList->getRemoteBackoff(destination));
	rts->setLocalBackoff(localBackoff);
	rts->setSequenceNumber(currentSequenceNumber);
	rts->setRetryCount(remoteStationList->getRetryCount(destination));

	toRadioLayer(rts);
	toRadioLayer(createRadioCommand(SET_STATE, TX));

	setState(MACAW_STATE_WFCTS, destination);

	trace() << "Finished sending RTS.";
}

void MACAW::sendCTS(int destination, int seqNumber) {
	trace() << "Sending a CTS to MAC address: " << destination;
	COLLECT_CTS_SENT
	MacawPacket* cts = new MacawPacket("MACAW CTS packet", MAC_LAYER_PACKET);
	cts->setType(MACAW_PACKET_CTS);
	cts->setSource(SELF_MAC_ADDRESS);
	cts->setDestination(destination);
	cts->setRemoteBackoff(remoteStationList->getRemoteBackoff(destination));
	cts->setLocalBackoff(localBackoff);
	cts->setSequenceNumber(currentSequenceNumber);
	cts->setRetryCount(remoteStationList->getRetryCount(destination));
	toRadioLayer(cts);    // doesn't need duplicating, since we'll reconstruct if it needs resending
	toRadioLayer(createRadioCommand(SET_STATE, TX));

	trace() << "Finished sending CTS";
}

void MACAW::sendAck(int destination, int seqNumber) {
	trace() << "Sending ACK to MAC address: " << destination;
	COLLECT_ACK_SENT
	MacawPacket* ack = new MacawPacket("MACAW ACK packet", MAC_LAYER_PACKET);
	ack->setType(MACAW_PACKET_ACK);
	ack->setSource(SELF_MAC_ADDRESS);
	ack->setDestination(destination);
	ack->setRemoteBackoff(remoteStationList->getRemoteBackoff(destination));
	ack->setLocalBackoff(localBackoff);
	ack->setSequenceNumber(currentSequenceNumber);
	ack->setRetryCount(remoteStationList->getRetryCount(destination));

	toRadioLayer(ack);
	toRadioLayer(createRadioCommand(SET_STATE, TX));

	trace() << "Finished sending ACK";
}

/*
 *  Sends back-to-back DS and DATA packets.
 *
 *  Implementation: data is constructed before the ds (even though the send order
 *  is the other way around), so that we can get the destination/seq number from
 */
void MACAW::sendDataFromFrontOfBuffer() {
	MacawPacket* dataPacket = check_and_cast <MacawPacket*>(macBuffer->peek());
	int destination = dataPacket->getDestination();

	// construct data send
	MacawPacket* ds = new MacawPacket("MACAW DS packet", MAC_LAYER_PACKET);
	ds->setSource(SELF_MAC_ADDRESS);
	ds->setDestination(destination);
	ds->setType(MACAW_PACKET_DS);
	ds->setLocalBackoff(localBackoff);
	ds->setRemoteBackoff(remoteStationList->getRemoteBackoff(destination));
	ds->setSequenceNumber(currentSequenceNumber);

	COLLECT_DS_SENT

	toRadioLayer(ds);
	toRadioLayer(createRadioCommand(SET_STATE, TX));

	setTimer(MACAW_TIMER_DSDELAY, 0.25);
	trace() << "SET THE TIMER";
}

/*
 * If we're currently idle and no packets are buffered, go into the sendData
 * routing straight away. Otherwise, simply buffer the packet for later.
 */
void MACAW::fromNetworkLayer(cPacket * netPacket, int destination) {
	if (sendDataEnabled) {
		printInfo("Received a packet from the network layer");
		destination = sinkMacAddress;   // it's an error/oddity of the routing layer, not us
		/* if the destination station isn't in our list, add it with some default
		 * values                                                                 */
		if (!remoteStationList->isInList(destination)) {
			trace() << "Destination " << destination << " was not in list. Adding it.";
			remoteStationList->add(destination, LOCAL_BACKOFF_UNKNOWN, REMOTE_BACKOFF_UNKNOWN, 0, 0);
		}
		remoteStationList->clearRetryCount(destination);

		/* encapsulate the packet from the network layer inside the mac headers */
		MacawPacket* macPacket = new MacawPacket("MACAW data packet", MAC_LAYER_PACKET);
		encapsulatePacket(macPacket, netPacket);
		macPacket->setType(MACAW_PACKET_DATA);
		macPacket->setSequenceNumber(currentSequenceNumber);
		macPacket->setSource(SELF_MAC_ADDRESS);
		macPacket->setDestination(destination);

		/* buffer the packet. if we are idle and haven't been forced to go to
		 * sleep for some other reason, send it straight away                 */
		trace() << "About to buffer MAC packet. Potential problems!!!";
		macBuffer->insertPacket(macPacket);
		if ((currentState == MACAW_STATE_IDLE) && macBuffer->numPackets()==1) {
			sendBufferedDataPacket();
		}

	} else {
		printInfo("send disabled: ignoring packet from network layer");
	}
}

void MACAW::fromRadioLayer(cPacket * pkt, double rssi, double lqi) {
	trace() << "In radio layer method";

	int source, destination, seqNumber;
	int localBackoff, remoteBackoff, retryCount;

	MacawPacket* macPacket = dynamic_cast < MacawPacket* >(pkt);
	if (macPacket == NULL) {
		printNonFatalError("Cast of received packet failed.");
		return;
	}

	source = macPacket->getSource();
	destination = macPacket->getDestination();
	seqNumber = macPacket->getSequenceNumber();
	localBackoff = macPacket->getLocalBackoff();
	remoteBackoff = macPacket->getRemoteBackoff();
	retryCount = macPacket->getRetryCount();

	/* if the station isn't in our remote station list, add it. we'll update
	 * the parameters depending on whether or not the packet was for us later */
	if (!remoteStationList->isInList(source)) {
		trace() << "Destination " << source << " was not in list. Adding it.";
		remoteStationList->add(source, LOCAL_BACKOFF_UNKNOWN, REMOTE_BACKOFF_UNKNOWN, seqNumber, 0);
	}

	bool forUs = (destination == SELF_MAC_ADDRESS) || (destination == BROADCAST_MAC_ADDRESS);

	trace() << "Packet received from radio layer! Type: " << macPacket->getType() << ", forUs: " << forUs;

	/* begin "backoff and copying rules" */
	if (!forUs) {
		if (macPacket->getType() != MACAW_PACKET_RTS) {
			remoteStationList->updateRemoteBackoff(source, localBackoff);
			if (remoteBackoff != REMOTE_BACKOFF_UNKNOWN) {
				remoteStationList->updateRemoteBackoff(destination, remoteBackoff);
			}
			myBackoff = localBackoff;
		}
	} else {      /* forUs */
		if (seqNumber > remoteStationList->getESN(source)) {
			remoteStationList->updateRemoteBackoff(source, localBackoff);
			if (remoteBackoff != REMOTE_BACKOFF_UNKNOWN) {
				tmpLocalBackoff = remoteBackoff;
				myBackoff = remoteBackoff;
			} else {
				tmpLocalBackoff = myBackoff;
			}
			remoteStationList->updateESN(source, seqNumber);
			remoteStationList->clearRetryCount(source);  // sets to 0
			remoteStationList->incrementRetryCount(source);  // now it's 1
		} else {   /* packet is a retransmission */
			int trialBackoff = (int)(1.5*(double)localBackoff);
			trialBackoff = (trialBackoff>maxBackoff) ? maxBackoff : trialBackoff;
			remoteStationList->updateRemoteBackoff(source, trialBackoff);
			if (remoteBackoff != REMOTE_BACKOFF_UNKNOWN) {
				tmpLocalBackoff = localBackoff+remoteBackoff-remoteStationList->getRemoteBackoff(source);
			} else {
				tmpLocalBackoff = myBackoff;
			}
			remoteStationList->incrementRetryCount(source);
		}
	}
	/* end "backoff and copying rules" */

	switch(macPacket->getType()) {
	case MACAW_PACKET_RTS : {
		if (!forUs) {
			// defer
			printInfo("Overheard an RTS");
			setTimer(MACAW_TIMER_QUIET, overheardRtsTime);
			setState(MACAW_STATE_QUIET, source);
			return;
		}
		if (currentState == MACAW_STATE_IDLE) {
			trace() << "Received an RTS from " << source << "!";
			COLLECT_RTS_RECEIVED
			if (alreadyAcked(source, seqNumber)) {
				sendAck(source, seqNumber);
			} else {
				remoteStationList->clearRetryCount(source);
				remoteStationList->updateSequenceNumber(source,seqNumber);
				sendCTS(source, seqNumber);
				setTimer(MACAW_TIMER_WFDS_TIMEOUT, maxWfdsTimeout);
				setState(MACAW_STATE_WFDS, source);
			}
			return;
		} else if (currentState == MACAW_STATE_CONTEND) {
			sendCTS(source, seqNumber);
			setTimer(MACAW_TIMER_WFDS_TIMEOUT, maxWfdsTimeout);
			setState(MACAW_STATE_WFDS, source);
			return;
		} else if (currentState == MACAW_STATE_QUIET) {
			// no need to set a timer here, since the MACAW_TIMER_QUIET timer
			// has already been set and will expire
			setState(MACAW_STATE_WFCONTEND);
			return;
		} else {
			trace() << "received rts in non-idle/non-contend/non-quiet state. ignoring.";
			return;
		}
		break;
	}
	case MACAW_PACKET_CTS : {
		if (!forUs) {
			// defer
			printInfo("Overhead a CTS");
			setTimer(MACAW_TIMER_QUIET, overheardRtsCtsExchange);
			setState(MACAW_STATE_QUIET);
			return;
		}
		COLLECT_CTS_RECEIVED
		if (currentState == MACAW_STATE_WFCTS) {
			cancelTimer(MACAW_TIMER_CTS_TIMEOUT);
			sendDataFromFrontOfBuffer();
			setState(MACAW_STATE_WFACK, source);
			setTimer(MACAW_TIMER_ACK_TIMEOUT, maxAckTimeout);
			return;
		} else {
			trace() << "received a cts in non-wfcts state. Ignoring.";
			return;
		}
		break;
	}
	case MACAW_PACKET_DS : {
		if (!forUs) {
			// defer
			printInfo("Overheard a DS");
			setTimer(MACAW_TIMER_QUIET, overheardDsTime);
			setState(MACAW_STATE_QUIET);
			return;
		}
		COLLECT_DS_RECEIVED
		if (currentState == MACAW_STATE_WFDS) {
			cancelTimer(MACAW_TIMER_WFDS_TIMEOUT);
			setState(MACAW_STATE_WFDATA, source);
			setTimer(MACAW_TIMER_WFDATA_TIMEOUT, maxDataTimeout);
		} else {
			printNonFatalError("received a ds in non-wfds state. Ignoring");
			return;
		}
		break;
	}
	case MACAW_PACKET_DATA : {
		if (!forUs) {
			printInfo("Overheard a DATA");
			return;
		}
		COLLECT_DATA_RECEIVED
		if (currentState == MACAW_STATE_WFDATA) {
			cancelTimer(MACAW_TIMER_WFDATA_TIMEOUT);
			trace() << "SENDING TO ROUTING LAYER";
			toNetworkLayer(decapsulatePacket(macPacket));
			sendAck(source, seqNumber);
			setState(MACAW_STATE_IDLE);
		} else {
			printNonFatalError("received a data in non-wfdata state. Ignoring");
			return;
		}
		break;
	}
	case MACAW_PACKET_ACK : {
		if (!forUs) {
			printInfo("Overhead an ACK");
			return;
		}
		COLLECT_ACK_RECEIVED
		if (currentState == MACAW_STATE_WFACK) {
			cancelTimer(MACAW_TIMER_ACK_TIMEOUT);
			deleteFrontOfBuffer();
			setState(MACAW_STATE_IDLE);
		} else {
			printNonFatalError("received an ack in non-wfack state. Ignoring");
		}
		break;
	}
	case MACAW_PACKET_RRTS : {
		if (!forUs){
			// defer
			printInfo("Overheard an RRTS");
			setTimer(MACAW_TIMER_QUIET, overheardRtsCtsExchange);
			setState(MACAW_STATE_QUIET, source);
			return;
		}
		COLLECT_RRTS_RECEIVED
		sendRTS(source, seqNumber);
		setTimer(MACAW_TIMER_CTS_TIMEOUT, maxCtsTimeout);
		setState(MACAW_STATE_WFCTS);
		break;
	}
	}
}


/* sets this node's backoff to the supplied value */
void MACAW::setMyBackoff(int backoff) {
	myBackoff = backoff;
}

/* returns a random timer value caclulated from the current backoff */
double MACAW::getTimerValueFromBackoff() {
	int tmpBackoff = (myBackoff>minBackoff) ? myBackoff : minBackoff;
	return getRandomTimerValue(minBackoff, tmpBackoff);
}

/* returns the current backoff value in seconds, ready for use with setting a timer */
double MACAW::getMyBackoff() {
	return ((double)myBackoff)/1000;
}

/* returns the current backoff in a more useful unit for sending in a packet */
int MACAW::getBackoffInMs() const {
	return myBackoff;
}

/* exactly the same as the above, but with a clearer semantic name */
int MACAW::getMyBackoffInMs() const {
	return getBackoffInMs();
}

/* increases backoff and returns the new backoff */
double MACAW::increaseBackoff() {
	int trialBackoff = (int)(1.5*(double)myBackoff);
	myBackoff = (trialBackoff>maxBackoff) ? maxBackoff : trialBackoff;
	return getMyBackoff();
}

/* decreases backoff and returns the new backoff */
double MACAW::decreaseBackoff() {
	int trialBackoff = myBackoff - backoffDecrement;
	myBackoff = (trialBackoff<minBackoff) ? minBackoff : trialBackoff;
	return getMyBackoff();
}

/* returns the reset value of the backoff, in seconds */
double MACAW::resetBackoff() {
	myBackoff = backoffResetValue;
	return getMyBackoff();
}


int MACAW::handleRadioControlMessage(cMessage*) {
	// we don't use control messages, so perfectly safe to do nothing
	return 0;
}

inline bool MACAW::alreadyAcked(int source, int seqNum) {
	return (remoteStationList->getESN(source) > seqNum);
}

void MACAW::deleteFrontOfBuffer() {
	trace() << "Deleting buffered packet";
	cancelAndDelete(macBuffer->peek());
	macBuffer->removeFirst();
	numRetries=0;
}

