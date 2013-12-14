/**
 *  SMAC.h
 *  Matthew Ireland, mti20, University of Cambridge
 *  14th November MMXIII
 *
 *  State, timer, and method declarations for the S-MAC protocol implementation.
 *
 */

#ifndef SMAC_H_
#define SMAC_H_

#include "VirtualMac.h"
#include "../macBuffer/MacBuffer.h"
#include "SMacPacket_m.h"
#include <assert.h>
#include <string>
#include <vector>
#include "../../CastaliaIncludes.h"

using namespace std;

#define PROTOCOL_NAME "SMAC"

/* Initial size of the schedule table    */
#define MAX_NODES     32

/* Used for sizing arrays of state and timer names */
#define SMAC_NUMBER_OF_STATES 9
#define SMAC_NUMBER_OF_TIMERS 10

/**
 *  State names corresponding to the S-MAC state machine (Dissertation
 *  Figure 3.6).
 *  Note: When adding a new state, update the macro SMAC_NUMBER_OF_STATES to
 *  reflect the new number of states, and also give it a name in the array
 *  SmacStateNames, which is initialised at the bottom of this header file. */
enum SmacStates {
	SMAC_STATE_SLEEP,
	SMAC_STATE_STARTUP,
	SMAC_STATE_LISTEN_FOR_SCHEDULE,
	SMAC_STATE_SYNC_BCAST_WAIT,
	SMAC_STATE_LISTEN_FOR_SYNC,
	SMAC_STATE_LISTEN_FOR_RTS,
	SMAC_STATE_WFDATA,
	SMAC_STATE_WFCTS,
	SMAC_STATE_WFACK
};

/**
 *  Timer index definitions.
 *  Note: When adding a new timer, update the macro SMAC_NUMBER_OF_TIMERS to
 *  reflect the new number of timers, and also give it a name in the array
 *  SmacTimerNames, which is initialised at the bottom of this header file. */
enum SmacTimers {
	SMAC_TIMER_WAKEUP,
	SMAC_TIMER_SYNC_BROADCAST,
	SMAC_TIMER_RTS_LISTEN,
	SMAC_TIMER_SEND,
	SMAC_TIMER_LISTEN_TIMEOUT,
	SMAC_TIMER_BS,
	SMAC_TIMER_CTSREC_TIMEOUT,
	SMAC_TIMER_DATA_TIMEOUT,
	SMAC_TIMER_ACK_TIMEOUT,
	SMAC_TIMER_WFBSTX
};

/**
 *  Main class implementing the S-MAC protocol.
 */
class SMAC : public VirtualMac {
private:
	/* begin parameters from .ned file */
	bool printDebuggingInfo;
	double phyDelayForValidCS;
	double phyDataRate;
	int phyFrameOverhead;

	bool sendDataEnabled;

	double syncListenPeriod;
	double rtsListenPeriod;
	double listenSleepPeriod;

	double minInitialScheduleWaitTime;
	double maxInitialScheduleWaitTime;

	int sendTimerMin;
	int sendTimerMax;
	/* end parameters from .ned file */

	/* begin state machine control */
	void setState(int newState);
	int currentState;
	int previousState;
	const static string SmacStateNames [SMAC_NUMBER_OF_STATES];
	/* end state machine control */

	/* begin timer callback functions */
	const static string SmacTimerNames [SMAC_NUMBER_OF_TIMERS];
	void handleWakeupTimerCallback();
	void handleSyncBroadcastTimerCallback();
	void handleRtsListenTimerCallback();
	void handleSendTimerCallback();
	void handleListenTimeoutCallback();
	void handleBsTimerCallback();
	void handleCtsRecTimeoutTimerCallback();
	void handleDataTimeoutCallback();
	void handleAckTimeoutCallback();
	void handleWfBsTxTimerCallback();
	/* end timer callback functions */

	/* random generation */
	int getRandom();
	int getRandom(int min, int max);
	double getRandomSeconds(int min, int max);  // parameters in ms
	double getRandomSeconds(double min, double max);  // parameters in s
	/* end random generation */

	/* begin sending data functions and state */
	void sendBufferedDataPacket();     // initiates handshake
	void sendDataFromFrontOfBuffer();  // actually sends the thing
	int numRetries;
	/* end sending data functions and state */

	/* packet manipulation functions */
	void broadcastSync();
	void sendRTS(int destination);
	void sendCTS(int destination, int seqNumber);
	void sendAcknowledgement(int source, int seqNumber);
	/* end packet manipulation functions */

	/* timeout state */
	double dataTimeout;
	double ctsTimeout;
	double ackTimeout;
	double listenTimeout;
	/* end timeout state */

	/* begin debug functions */
	void printNonFatalError(string);
	void printFatalError(string);
	void printInfo(string);
	void printPacketSent(string);
	void printPacketReceived(string);
	/* end debug functions */

	/* begin power control functions */
	void goToSleep();
	void wakeUp();
	/* end power control functions */

	int syncBroadcastTimeMin;
	int syncBroadcastTimeMax;

	bool overheardRts, overheardCts;

	/* buffer management */
	inline bool bufferIsEmpty();
	void deleteFrontOfBuffer();
	/* end buffer management */

	/* schedule management */
	bool active;
	simtime_t primarySchedule;
	simtime_t constOverhead;
	std::vector<simtime_t> scheduleTable;  // indexed by MAC address
	void addSchedule(int source, simtime_t value);   // value is from the sync packet
	inline void updateSchedule(int source, simtime_t value) { addSchedule(source, value); };
	simtime_t getScheduleValue();   // (for broadcasting)
	simtime_t getMaxScheduleTableOffset();
	simtime_t getMinScheduleTableOffset();
	inline simtime_t getLargestOffset() {return getMaxScheduleTableOffset();};
	inline simtime_t getSmallestOffset() {return getMinScheduleTableOffset();};
	void printScheduleTable();
	double getListenTimeoutValue();
	double getWakeupTimerValue();
	void adoptSchedule(int source, simtime_t value);  // used initially
	void createSchedule();   // used if no schedule to be adopted
	/* end schedule management */

	int currentSequenceNumber;
	int scount;
	int numWakeups;
	int sinkMacAddress;
	bool isSink;
	bool initialisationComplete;
	const static int maxRetries = 5;

	MacBuffer<SMacPacket*>* macBuffer;


protected:
	void startup();
	void reset();
	void timerFiredCallback(int);
	void fromNetworkLayer(cPacket *, int);
	void fromRadioLayer(cPacket *, double, double);
	int handleRadioControlMessage(cMessage *);
};

/**
 *  Names of states in the S-MAC state machine.
 */
const string SMAC::SmacStateNames [SMAC_NUMBER_OF_STATES] = {
		"SMAC_STATE_SLEEP",
		"SMAC_STATE_STARTUP",
		"SMAC_STATE_LISTEN_FOR_SCHEDULE",
		"SMAC_STATE_SYNC_BCAST_WAIT",
		"SMAC_STATE_LISTEN_FOR_SYNC",
		"SMAC_STATE_LISTEN_FOR_RTS",
		"SMAC_STATE_WFDATA",
		"SMAC_STATE_WFCTS",
		"SMAC_STATE_WFACK" };

/**
 *  Human-friendly names of the timers used in the S-MAC protocol.
 */
const string SMAC::SmacTimerNames [SMAC_NUMBER_OF_TIMERS] = {
		"SMAC_TIMER_WAKEUP",
		"SMAC_TIMER_SYNC_BROADCAST",
		"SMAC_TIMER_RTS_LISTEN",
		"SMAC_TIMER_SEND",
		"SMAC_TIMER_LISTEN_TIMEOUT",
		"SMAC_TIMER_BS",
		"SMAC_TIMER_CTSREC_TIMEOUT",
		"SMAC_TIMER_DATA_TIMEOUT",
		"SMAC_TIMER_ACK_TIMEOUT",
		"SMAC_TIMER_WFBSTX" };

#endif /* def SMAC_H_ */
