/**
 *  BMAC.h
 *  Matthew Ireland, mti20, University of Cambridge
 *  27th November MMXIII
 *
 *  State, timer, and method declarations for the B-MAC protocol implementation.
 *
 */

#ifndef BMAC_H_
#define BMAC_H_

#include "VirtualMac.h"
#include "../macBuffer/MacBuffer.h"
#include "BMacPacket_m.h"
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
#define BMAC_CHECK_PERIOD    checkPeriod

// number of retries
#define BMAC_NUM_RETRIES 5

#define NO_REMOTE_STATION            -1

#define PROTOCOL_NAME "BMAC"

// TODO use the last of each of the enum's to signify these
#define BMAC_NUMBER_OF_STATES 9
#define BMAC_NUMBER_OF_TIMERS 8
#define BMAC_NUMBER_OF_BACKOFF_INCREMENTS 6

/*
 *  TODO: description
 *  Note: When adding a new state, update the macro BMAC_NUMBER_OF_STATES to
 *  reflect the new number of states, and also give it a name in the array
 *  BmacStateNames, which is initialised at the bottom of this header file. */
enum BmacStates {
	BMAC_STATE_SLEEP,
	BMAC_STATE_RSSISAMPLE,
	BMAC_STATE_WFRADIORSSI,
	BMAC_STATE_LISTEN,
	BMAC_STATE_WFDATA,
	BMAC_STATE_WFACK,
	BMAC_STATE_PRESENDCCA,
	BMAC_STATE_PREAMBLE_SEND,
	BMAC_STATE_STARTUP
};

/*
 *  TODO: description
 *  Note: When adding a new timer, update the macro BMAC_NUMBER_OF_TIMERS to
 *  reflect the new number of timers, and also give it a name in the array
 *  BmacTimerNames, which is initialised at the bottom of this header file. */
enum BmacTimers {
	BMAC_TIMER_CHECKPERIOD,
	BMAC_TIMER_ACKTIMEOUT,
	BMAC_TIMER_LISTENTIMEOUT,
	BMAC_TIMER_WAITFORPREAMBLE,
	BMAC_TIMER_WFRADIO_CCA,
	BMAC_TIMER_RETRY,
	BMAC_TIMER_RETRYWAKEUP,
	BMAC_TIMER_WAITFORACKTX
};

class BMAC : public VirtualMac {
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
	int preamblePacketLength;          // since we must send packets, we saturate the radio layer with a stream of preamble packets to emulate a long bit stream
	double preambleTransmissionTime;
	double gapBetweenPreambleAndData;
	double sendDataTime;
	double retryPeriod;
	int sinkMacAddress;
	double maxAckDelay;
	double waitForAckTxTime;
	/* end parameters from .ned file */

	bool currentPacketIsBroadcast;

	/* begin state machine control */
	void setState(int newState);
	void setState(int, int);   // legacy from MACAW (can probably be deleted)
	int currentState;
	int previousState;
	const static string BmacStateNames [BMAC_NUMBER_OF_STATES];
	/* end state machine control */


	/* begin timer callback functions */
	const static string BmacTimerNames [BMAC_NUMBER_OF_TIMERS];
	void handleCheckPeriodTimerCallback();
	void handleWfAckTimerCallback();
	void handleListenTimeoutCallback();
	void handleCheckPeriodTimerCallback_postwakeup();
	void handleWfDataTimeout();
	void handleRetryTimerCallback();
	void handleRetryWakeupTimerCallback();
	void handleWaitForAckTxTimerCallback();
	void handleWaitForPreambleTimerCallback();
	/* end timer callback functions */

	/* begin cca & rssi functions */
	double wakeupDelay;
	bool isChannelClear(int rssiSample);
	bool canSendData;
	void doCCA();
	void takeRSSIsample();
	void takeRSSIsample(bool);  // allows the option of not calling the goToSleep() method
	/* end cca functions */

	/* begin sending data functions and state */
	void sendBufferedDataPacket();
	void sendDataFromFrontOfBuffer();
	void resendData();
	int numRetries;
	/* end sending data functions and state */

	/* begin preamble manipulation functions and state */
	void sendPreamble();
	bool useComplexIncrementMethod;
	/* end preamble manipulation functions and state*/

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

	/* begin backoff state and functions */
	int myBackoff;        // current position in backoff array
	int maxBackoff;
	const static int bmacBackoffs [BMAC_NUMBER_OF_BACKOFF_INCREMENTS];   // backoff units, in ms
	inline double getCurrentBackoff() const;
	double increaseBackoff();
	double decreaseBackoff();
	void resetBackoff();
	/* end backoff state and functions */

	/* buffer management */
	void deleteFrontOfBuffer();
	/* end buffer management */

	/* packet manipulation functions */
	void sendAcknowledgement(int source, int seqNumber);
	/* end packet manipulation functions */

	int currentSequenceNumber;

	int remoteStation;      // legacy (from MACAW). can probably be deleted.

	MacBuffer<BMacPacket*>* macBuffer;


protected:
	void startup();
	void reset();
	void timerFiredCallback(int);
	void fromNetworkLayer(cPacket *, int);
	void fromRadioLayer(cPacket *, double, double);
	int handleRadioControlMessage(cMessage *);
};

const string BMAC::BmacStateNames [BMAC_NUMBER_OF_STATES] = { "BMAC_STATE_SLEEP", "BMAC_STATE_RSSISAMPLE", "BMAC_STATE_WFRADIORSSI", "BMAC_STATE_LISTEN",	"BMAC_STATE_WFDATA", "BMAC_STATE_WFACK", "BMAC_STATE_PRESENDCCA", "BMAC_STATE_PREAMBLE_SEND", "BMAC_STATE_STARTUP" };
const string BMAC::BmacTimerNames [BMAC_NUMBER_OF_TIMERS] = { "BMAC_TIMER_CHECKPERIOD", "BMAC_TIMER_ACKTIMEOUT", "BMAC_TIMER_LISTENTIMEOUT", "BMAC_TIMER_WAITFORPREAMBLE", "BMAC_TIMER_WFRADIO_CCA", "BMAC_TIMER_RETRY", "BMAC_TIMER_RETRYWAKEUP", "BMAC_TIMER_WAITFORACKTX" };
const int    BMAC::bmacBackoffs   [BMAC_NUMBER_OF_BACKOFF_INCREMENTS] = {32, 64, 96, 128, 256, 512};   // backoff units, in ms. empirical.


#endif /* def BMAC_H_ */
