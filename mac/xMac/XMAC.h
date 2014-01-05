/*
 * Xmac.h
 *
 *  Created on: Jan 5, 2014
 *      Author: mti20
 */

#ifndef XMAC_H_
#define XMAC_H_

#include "VirtualMac.h"
#include "XMacPacket_m.h"
#include "../macBuffer/MacBuffer.h"
#include <assert.h>
#include <string>
#include "../../CastaliaIncludes.h"

using namespace std;

// TODO tidy up all the defines into a column layout

// TODO change this to an inline function
//#ifndef TX_TIME(x)
#define TX_TIME(x)		(phyLayerOverhead + x)*1/(1000*phyDataRate/8.0)	//x are in BYTES

/**** defining these two macros was silly (should be in ned file). these parameters are
 * now in fact in the ned file, so delete these macros as soon as possible.          */
// time we ordinarily listen for
#define XMAC_WAKEUP_TIME     listenPeriod
// time we ordinarily listen for
#define XMAC_CHECK_PERIOD    checkPeriod

// number of retries
#define XMAC_NUM_RETRIES 5

#define NO_REMOTE_STATION            -1

#define PROTOCOL_NAME "XMAC"

#define XMAC_NUMBER_OF_STATES 7
#define XMAC_NUMBER_OF_TIMERS 10

/*
 *  TODO: description
 *  Note: When adding a new state, update the macro XMAC_NUMBER_OF_STATES to
 *  reflect the new number of states, and also give it a name in the array
 *  XmacStateNames, which is initialised at the bottom of this header file. */
enum XmacStates {
	XMAC_STATE_SLEEP,
	XMAC_STATE_CCA,
	XMAC_STATE_LISTEN,
	XMAC_STATE_WFDATA,
	XMAC_STATE_WFACK,
	XMAC_STATE_PREAMBLE_SEND,
	XMAC_STATE_DATASENDWAIT
};

/*
 *  TODO: description
 *  Note: When adding a new timer, update the macro XMAC_NUMBER_OF_TIMERS to
 *  reflect the new number of timers, and also give it a name in the array
 *  XmacTimerNames, which is initialised at the bottom of this header file. */
enum XmacTimers {
	XMAC_TIMER_CHECKPERIOD,
	XMAC_TIMER_ACKTIMEOUT,
	XMAC_TIMER_LISTENTIMEOUT,
	XMAC_TIMER_WFRADIO_CCA,
	XMAC_TIMER_WFDATATIMEOUT,
	XMAC_TIMER_SENDDATAWAIT,
	XMAC_TIMER_PREAMBLESTROBE,
	XMAC_TIMER_RETRY,
	XMAC_TIMER_RETRYWAKEUP,
	XMAC_TIMER_WAITFORACKTX
};

class XMAC : public VirtualMac {
private:
	/* begin parameters from .ned file */
	bool printDebuggingInfo;
	double phyDelayForValidCS;
	double phyDataRate;
	int phyFrameOverhead;
	double listenPeriod;
	double checkPeriod;
	double interAckPeriod;
	double interPreamblePeriod;
	double maxDataDelay;            // the amount of time we are willing to wait for a data packet after acknowledging a preamble
	bool sendDataEnabled;
	int maxDataSendDelay;           // this differs from maxDataDelay:
	double maxBroadcastDataDelay;
	int numberOfPreambles;
	double retryPeriod;
	int sinkMacAddress;
	double maxAckDelay;
	double waitForAckTxTime;
	/* end parameters from .ned file */

	/* begin state machine control */
	void setState(int newState);
	void setState(int, int);   // legacy from MACAW (can probably be deleted)
	int currentState;
	int previousState;
	const static string XmacStateNames [XMAC_NUMBER_OF_STATES];
	/* end state machine control */


	/* begin timer callback functions */
	const static string XmacTimerNames [XMAC_NUMBER_OF_TIMERS];
	void handleCheckPeriodTimerCallback();
	void handleWfAckTimerCallback();
	void handleListenTimeoutCallback();
	void handleCheckPeriodTimerCallback_postwakeup();
	void handleWfDataTimeout();
	void handleSendDataWaitTimerCallback();
	void handlePreambleTimerCallback();
	void handleRetryTimerCallback();
	void handleRetryWakeupTimerCallback();
	void handleWaitForAckTxTimerCallback();
	/* end timer callback functions */

	/* begin cca functions */
	double wakeupDelay;
	bool channelIsClear();
	bool canSendData;
	void doCCA();
	/* end cca functions */

	/* begin sending data functions and state */
	void sendBufferedDataPacket();
	void sendDataFromFrontOfBuffer();
	void resendData();
	int numRetries;
	/* end sending data functions and state */

	/* begin preamble manipulation functions */
	void sendPreambleAck(int source, int seqNumber);
	void sendPreamble(int destination, int sequenceNumber);
	/* end preamble manipulation functions */

	/* begin debug functions */
	void printNonFatalError(string);
	void printFatalError(string);
	void printInfo(string);
	void printPacketSent(string);
	void printPacketReceived(string);
	/* end debug functions */

	/* begin power control functions */
	void goToSleep();
	void goToSleep(bool canCheckTxBuffer);
	void wakeUp();
	/* end power control functions */

	/* buffer management */
	void deleteFrontOfBuffer();
	/* end buffer management */

	/* packet manipulation functions */
	XMacPacket* createDataPacket(cPacket*, int);
	void sendDataAcknowledgement(int source, int seqNumber);
	/* end packet manipulation functions */

	int currentSequenceNumber;
	int numberOfPreambleRetries;
	bool preambleHasBeenAcked;

	int remoteStation;

	MacBuffer<XMacPacket*>* macBuffer;


protected:
	void startup();
	void reset();
	void timerFiredCallback(int);
	void fromNetworkLayer(cPacket *, int);
	void fromRadioLayer(cPacket *, double, double);
	int handleRadioControlMessage(cMessage *);
};

const string XMAC::XmacStateNames [XMAC_NUMBER_OF_STATES] = { "XMAC_STATE_SLEEP", "XMAC_STATE_CCA", "XMAC_STATE_LISTEN", "XMAC_STATE_WFDATA", "XMAC_STATE_WFACK", "XMAC_STATE_PREAMBLESEND", "XMAC_STATE_DATASENDWAIT" };
const string XMAC::XmacTimerNames [XMAC_NUMBER_OF_TIMERS] = { "XMAC_TIMER_CHECKPERIOD",	"XMAC_TIMER_ACKTIMEOUT", "XMAC_TIMER_LISTENTIMEOUT", "XMAC_TIMER_WFRADIO_CCA", "XMAC_TIMER_WFDATATIMEOUT", "XMAC_TIMER_SENDDATAWAIT", "XMAC_TIMER_PREAMBLESTROBE", "XMAC_TIMER_RETRY", "XMAC_TIMER_RETRYWAKEUP", "XMAC_TIMER_WAITFORACKTX" };


#endif /* def XMAC_H_ */
