//
//  MACAW.ned
//  Matthew Ireland, University of Cambridge
//  4th November MMXIII
//
//  Default parameters and values in the MACAW protocol.
// 

 package node.communication.mac.macaw;
 
 simple MACAW like node.communication.mac.iMac {
  parameters:
    // compulsary parameters (from documentation)
   	bool collectTraceInfo = default(true);
	int macMaxPacketSize = default(0);	// in bytes
	int macBufferSize = default(16);		// in number of messages
	int macPacketOverhead = default(11);
	
  	// physical layer parameters (defaults copied from Castalia documentation)
	double phyDelayForValidCS = default (0.128);
	double phyDataRate = default (250);
	int phyFrameOverhead = default (6);
	
	// my stuff
	int defaultMinRndTimerValue = default(10);  // min value for a random timer (e.g. IDLE->CONTEND), in ms
	int defaultMaxRndTimerValue = default(100); // max value for a random timer (e.g. IDLE->CONTEND), in ms
	bool sendDataEnabled = default(true);
	
	double interSendPause = default(0.1);       // in seconds
	
	bool printDebugInfo = default(true);
	
	// timeouts
	int maxWfdsTimeout = default(500);   // max number of ms we'll wait for a DS, after sending a CTS
	int maxAckTimeout = default(500);    // ms
	int maxDataTimeout = default(500);   // ms
	int maxCtsTimeout = default(500);    // ms
	int overheardRtsTime = default(500);   // ms
	int overheardDsTime = default(350);   // ms
	int overheardRtsCtsExchange = default(400); // ms
	int maxRetries = default(5);
	
	// backoff
	int minBackoff = default(20);  // minimum backoff for this station in ms
	int maxBackoff = default(1024);   // maximum backoff for this station in ms
	int initialBackoff = default(20);
	int backoffDecrement = default(10);   // constant factor by which to decrement the backoff (ms)
	
	int sinkMacAddress = default(1);
  	
 gates:
	output toNetworkModule;
	output toRadioModule;
	input fromNetworkModule;
	input fromRadioModule;
	input fromCommModuleResourceMgr;
 
 }