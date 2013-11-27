
/**
 *  MACAW.h
 *  Matthew Ireland, mti20, University of Cambridge
 *  4th November MMXIII
 *
 *  State, timer, and method declarations for the MACAW protocol implementation.
 *
 */

#ifndef MACAW_H_
#define MACAW_H_

#include "VirtualMac.h"
#include "../macBuffer/MacBuffer.h"
#include "MacawPacket_m.h"
#include "RemoteStationList.h"
#include "RemoteStation.h"
#include "RemoteStationNotFoundException.h"
#include <assert.h>
#include <string>
#include "../../CastaliaIncludes.h"

using namespace std;

#define MACAW_NUMBER_OF_STATES  8
#define MACAW_NUMBER_OF_TIMERS  8

#define NO_REMOTE_STATION -1

#define REMOTE_BACKOFF_UNKNOWN -1
#define LOCAL_BACKOFF_UNKNOWN  -1

#define PROTOCOL_NAME "MACAW"


/* collecting output macros */
#define COLLECT_DATA_SENT_STRING "Number of DATA packets sent"
#define COLLECT_DATA_RECEIVED_STRING "Number of DATA packets received"
#define COLLECT_DS_SENT_STRING "Number of DS packets sent"
#define COLLECT_RTS_RECEIVED_STRING "Number of RTS packets received"
#define COLLECT_CTS_RECEIVED_STRING "Number of CTS packets received"
#define COLLECT_DS_RECEIVED_STRING "Number of DS packets received"
#define COLLECT_CTS_SENT_STRING "Number of CTS packets sent"
#define COLLECT_ACK_SENT_STRING "Number of ACK packets sent"
#define COLLECT_ACK_RECEIVED_STRING "Number of ACK packets received"
#define COLLECT_RRTS_RECEIVED_STRING "Number of RRTS packets received"
#define COLLECT_RRTS_SENT_STRING "Number of RRTS packets sent"
#define COLLECT_RTS_SENT_STRING "Number of CTS packets sent"
#define COLLECT_DATA_SENT  collectOutput(COLLECT_DATA_SENT_STRING, SELF_MAC_ADDRESS);
#define COLLECT_DATA_RECEIVED  collectOutput(COLLECT_DATA_RECEIVED_STRING, SELF_MAC_ADDRESS);
#define COLLECT_DS_SENT    collectOutput(COLLECT_DS_SENT_STRING, SELF_MAC_ADDRESS);
#define COLLECT_DS_RECEIVED collectOutput(COLLECT_DS_RECEIVED_STRING, SELF_MAC_ADDRESS);
#define COLLECT_RTS_RECEIVED collectOutput(COLLECT_RTS_RECEIVED_STRING, SELF_MAC_ADDRESS);
#define COLLECT_RTS_SENT collectOutput(COLLECT_RTS_SENT_STRING, SELF_MAC_ADDRESS);
#define COLLECT_RRTS_SENT collectOutput(COLLECT_RRTS_SENT_STRING, SELF_MAC_ADDRESS);
#define COLLECT_RRTS_RECEIVED collectOutput(COLLECT_RRTS_RECEIVED_STRING, SELF_MAC_ADDRESS);
#define COLLECT_CTS_SENT collectOutput(COLLECT_CTS_SENT_STRING, SELF_MAC_ADDRESS);
#define COLLECT_ACK_SENT collectOutput(COLLECT_ACK_SENT_STRING, SELF_MAC_ADDRESS);
#define COLLECT_ACK_RECEIVED collectOutput(COLLECT_ACK_RECEIVED_STRING, SELF_MAC_ADDRESS);
#define COLLECT_CTS_RECEIVED collectOutput(COLLECT_CTS_RECEIVED_STRING, SELF_MAC_ADDRESS);
/* end collecting output macros




/**
 *  States in the state machine.
 *
 *  Note: When adding a new state, update the macro MACAW_NUMBER_OF_STATES to
 *  reflect the new number of states, and also give it a name in the array
 *  MacawStateNames, which is initialised at the bottom of this header file. */
enum MacawStates {
	MACAW_STATE_IDLE,
	MACAW_STATE_CONTEND,
	MACAW_STATE_WFCTS,
	MACAW_STATE_WFCONTEND,
	MACAW_STATE_WFDATA,
	MACAW_STATE_WFDS,
	MACAW_STATE_WFACK,
	MACAW_STATE_QUIET
};

/**
 *  Timer definitions.
 *
 *  Note: When adding a new timer, update the macro MACAW_NUMBER_OF_TIMERS to
 *  reflect the new number of timers, and also give it a name in the array
 *  MacawTimerNames, which is initialised at the bottom of this header file. */
enum MacawTimers {
	MACAW_TIMER_CONTEND,
	MACAW_TIMER_WFDS_TIMEOUT,
	MACAW_TIMER_CTS_TIMEOUT,
	MACAW_TIMER_ACK_TIMEOUT,
	MACAW_TIMER_WFDATA_TIMEOUT,
	MACAW_TIMER_QUIET,
	MACAW_TIMER_SEND_PAUSE,
	MACAW_TIMER_DSDELAY
};

class MACAW : public VirtualMac {
private:
	/* begin parameters from .ned file */
	bool printDebuggingInfo;
	double phyDelayForValidCS;
	double phyDataRate;
	int phyFrameOverhead;
	double maxWfdsTimeout;
	double maxAckTimeout;
	double maxDataTimeout;
	double maxCtsTimeout;
	double overheardRtsTime;
	double overheardDsTime;
	double overheardRtsCtsExchange;
	/* end parameters from .ned file */

	/* begin state machine control */
	void setState(int newState);
	void setState(int, int);   // legacy from MACAW (can probably be deleted)
	int currentState;
	int previousState;
	const static string MacawStateNames [MACAW_NUMBER_OF_STATES];
	/* end state machine control */

	/* begin timer control functions */
	int defaultMinRndTimerValue;  // min value for a random timer (e.g. IDLE->CONTEND), in ms
	int defaultMaxRndTimerValue;  // max value for a random timer (e.g. IDLE->CONTEND), in ms
	inline double getRandomTimerValue(const int min, const int max) const;  // args in ms, returns in seconds
	inline double getRandomTimerValue() const;
	/* end timer control functions */

	/* begin timer callback functions */
	const static string MacawTimerNames [MACAW_NUMBER_OF_TIMERS];
	void handleContendTimerCallback();
	void handleWfdsTimeoutTimerCallback();
	void handleQuietTimerCallback();
	void handleDsTimerCallback();
	void handleCtsTimeoutTimerCallback();
	void handleGenericTimeoutCallback();
	void handleSendPauseTimerCallback();
	/* end timer callback functions */

	/* begin sending data functions and state */
	void sendRTS(int destination, int seqNumber);
	void sendCTS(int destination, int seqNumber);
	void sendAck(int destination, int seqNumber);
	void sendDataFromFrontOfBuffer();
	void sendBufferedDataPacket();
	bool alreadyAcked(int, int);   // consider making this inline
	int numRetries;
	int maxRetries;
	int currentSequenceNumber;
	/* end sending data functions and state */

	/* begin backoff state and functions */
	RemoteStationList* remoteStationList;
	int myBackoff;    // my backoff value in ms
	int minBackoff;   // minimum backoff in ms
	int maxBackoff;   // minimum backoff in ms
	int backoffDecrement;   // constant factor by which to decrement backoff
	int backoffResetValue;  // value with which to reset the backoff to, in ms
	int tmpLocalBackoff;
	int localBackoff;  // for use in the current communication (see "backoff and copying rules")
	double interSendPause;
	double getMyBackoff();
	void setMyBackoff(int);
	double getTimerValueFromBackoff();
	int getMyBackoffInMs() const;
	int getBackoffInMs() const;
	double increaseBackoff();
	double decreaseBackoff();
	double resetBackoff();
	/* end backoff state and functions */

	/* begin functions for managing fromRadioLayer */

	/* end functions for managing fromRadioLayer */

	/* begin debug functions */
	void printNonFatalError(string);
	void printFatalError(string);
	void printInfo(string);
	void printPacketSent(string);
	void printPacketReceived(string);
	/* end debug functions */

	/* buffer management */
	void deleteFrontOfBuffer();
	/* end buffer management */

	int remoteStation;
	int sinkMacAddress;

	bool sendDataEnabled;

	MacBuffer<MacawPacket*>* macBuffer;


protected:
	void startup();
	void reset();
	void timerFiredCallback(int);
	void fromNetworkLayer(cPacket *, int);
	void fromRadioLayer(cPacket *, double, double);
	int handleRadioControlMessage(cMessage *);
};

const string MACAW::MacawStateNames [MACAW_NUMBER_OF_STATES] = { "MACAW_STATE_IDLE", "MACAW_STATE_CONTEND",	"MACAW_STATE_WFCTS", "MACAW_STATE_WFCONTEND", "MACAW_STATE_WFDATA", "MACAW_STATE_WFDS", "MACAW_STATE_WFACK", "MACAW_STATE_QUIET" };
const string MACAW::MacawTimerNames [MACAW_NUMBER_OF_TIMERS] = { "MACAW_TIMER_CONTEND", "MACAW_TIMER_WFDS_TIMEOUT", "MACAW_TIMER_CTS_TIMEOUT", "MACAW_TIMER_ACK_TIMEOUT", "MACAW_TIMER_WFDATA_TIMEOUT", "MACAW_TIMER_QUIET", "MACAW_TIMER_SEND_PAUSE", "MACAW_TIMER_DSDELAY" };


#endif /* def MACAW_H_ */
