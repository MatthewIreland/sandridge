//
//  XMAC.ned
//  Matthew Ireland, University of Cambridge
//  22nd November MMXIII
//
//  Default parameters and values in the S-MAC protocol.
// 
 
 package node.communication.mac.xMac;
 
 simple XMAC like node.communication.mac.iMac {
  parameters:
    // compulsary parameters (from documentation)
   	bool collectTraceInfo = default(true);
	int macMaxPacketSize = default(0);	// in bytes
	int macBufferSize = default(16);		// in number of messages
	int macPacketOverhead = default(11);
	
	double wakeupDelay = default(0.0005);   // set experimentally (seconds)
	double listenPeriod = default(0.1);     // must be at least twice inter-preamble period 
	double checkPeriod = default(0.25);       // = (inter-preamble period) * (number of preambles) minus a fiddle-factor
	double interAckPeriod = default(0.03);  // silly name for inter-preamble period
	int numberOfPreambles = default(10);    // number of preamble packets to send before each data (together with interAckPeriod determines how often a node must wake up)
	double maxDataDelay = default(0.1);     // maximum of time we will wait for data after acknowledging a preamble (seconds)
	double maxAckDelay = default(0.1);      // maximum time we'll wait for an ack after sending data, before retrying (seconds)
	int maxDataSendDelay = default(20);     // maximum random delay before sending out data packet, in ms
	double maxBroadcastDataDelay = default(0.1);  // maximum of time we'll spend waiting for a broadcast packet (ms)
	double waitForAckTxTime = default(0.0003);
  
  	// debug parameters
  	bool printDebugInfo = default(true);
  	bool sendDataEnabled = default(true);   // set to false to stop this node from ever accepting data from the network layer or above
  	
  	// physical layer parameters (defaults copied from Castalia documentation)
	double phyDelayForValidCS = default (0.128);
	double phyDataRate = default (250);
	int phyFrameOverhead = default (6);
	
	int sinkMacAddress = default(1);
  	
  	double retryPeriod = 0.25;  // seconds, before retrying a failed transmission
  	
 gates:
	output toNetworkModule;
	output toRadioModule;
	input fromNetworkModule;
	input fromRadioModule;
	input fromCommModuleResourceMgr;
 
 }