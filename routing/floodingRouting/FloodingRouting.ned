// ******************************************************
// * FloodingRouting.ned                                *
// * Matthew Ireland, University of Cambridge           *
// * 27th December MMXIII                               *
// *                                                    *
// * Default parameters and values in the Flood Routing *
// * protocol.                                          *
// *                                                    *
// *****************************************************/
 
 package node.communication.routing.floodingRouting;
 
 simple FloodingRouting like node.communication.routing.iRouting {
  parameters:
	bool collectTraceInfo = default (true);
	int maxNetFrameSize = default (0);  // no limit
	int netBufferSize = default (32);
	int netDataFrameOverhead = default(14);	
	
 	bool printDebugInfo = default (true); 
	// both of the following in seconds
	int  minStartupDelay = default(10);      // random startup delay will be some
	int  maxStartupDelay = default(30);      // number between min and max
 		
	int sinkMacAddress = default(40);
 		
  gates:
	output toCommunicationModule;
	output toMacModule;
	input fromCommunicationModule;
	input fromMacModule;
	input fromCommModuleResourceMgr;
 }