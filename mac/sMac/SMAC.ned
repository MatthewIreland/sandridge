//
//  SMAC.ned
//  Matthew Ireland, University of Cambridge
//  14th November MMXIII
//
//  Default parameters and values in the S-MAC protocol.
// 
 
 package node.communication.mac.sMac;
 
 simple SMAC like node.communication.mac.iMac {
  parameters:
    // compulsary parameters (from documentation)
   	bool collectTraceInfo = default(true);
	int macMaxPacketSize = default(0);	    // in bytes
	int macBufferSize = default(16);		// in number of messages
	int macPacketOverhead = default(20);

  	// debug parameters
  	bool printDebugInfo = default(true);
  	bool sendDataEnabled = default(true);   // set to false to stop this node
  	                                        // from ever accepting data from
  	                                        // the network layer or above
	
	// mac address of the sink node (optional for S-MAC)
	int sinkMacAddress = default(1);
	
	// time spent listening and sleeping (ms)
	int syncListenPeriod = default(100);
	int rtsListenPeriod = default(500);
	int listenSleepPeriod = default(1200);

	// range of number of wakeups before sending a sync (ms)
    // these will usually be overridden in omnetpp.ini, so will actually
    // be much larger
	int minScount = default(3);
	int maxScount = default(6);
	
	// send timer (ms)
	int sendTimerMin = default(50);
	int sendTimerMax = default(300);

	// range of sync broadcast timer (ms)
	int syncBroadcastTimeMin = default(2);
	int syncBroadcastTimeMax = default(75);
	
	// measured empirically
	double constOverhead = default(0.0002000);
	
	// time to initially wait for a schedule (ms)
	int minInitialScheduleWaitTime = default(1000);
	int maxInitialScheduleWaitTime = default(5000);

	// timeouts (ms)
	int dataTimeout = default(300);
	int ctsTimeout = default(100);
	int ackTimeout = default(200);
	int listenTimeout = default(250);

 gates:
	output toNetworkModule;
	output toRadioModule;
	input fromNetworkModule;
	input fromRadioModule;
	input fromCommModuleResourceMgr;
 
 }