/*
 *  FloodingRouting.cc
 *  Matthew Ireland, University of Cambridge
 *  27th December MMXIII
 *
 *  Implements Flood Routing; that is, whenever a node receives a new packet,
 *  it forwards it to all its neighbours.
 */


#include "FloodingRouting.h"
#include "FloodingRoutingPacket_m.h"
#include <ctime> 
#include <cstdlib>

Define_Module(FloodingRouting);

/**
 *  Entry point to the protocol from the Castalia simulator.
 *  Imports parameters from the .ned file and initialises timers, the state
 *  machine, and internal state.
 */
void FloodingRouting::startup() {
  startupComplete = false;

	/* Check that the Application module used has the boolean parameter "isSink"
	 * If we are the sink, then we pass data up to the application layer,
	 * otherwise we simply forward it (the sink node(s) do(es) not forward
	 * data).
	 */
  	cModule *parentMod =
  			getParentModule()->getParentModule()->getSubmodule("Application");
  	if (parentMod->hasPar("isSink")) {
  		isSink = parentMod->par("isSink");
  		if (isSink)
  			trace() << "Node is sink.";
  		else
  			trace() << "Node is a source node.";
  	} else {
  		opp_error("\nNo isSink parameter");
  	}

  	printDebugInfo = par("printDebugInfo");

	/* after some random time, make sure our neighbours know about us */
	int seed = *SELF_NETWORK_ADDRESS;    // seed with a number unique to us, otherwise all nodes will have the same startup delay (converts const char* to int)
	srand(seed);
	minStartupDelay = par("minStartupDelay");
	maxStartupDelay = par("maxStartupDelay");
	int startupDelay = minStartupDelay+(rand() % (maxStartupDelay-minStartupDelay));
	setTimer(FR_TIMER_STARTUP, startupDelay);
  	if (printDebugInfo)
	  trace() << "Sending startup packet in " << startupDelay << " seconds.";

  	declareOutput("Packets received per node");   // simple output, only used at sink
  	declareOutput("Packets originated");          // simple output, used at every node
  	declareOutput("Data packets received per node before die time");   // simple output, used at all nodes
  	declareOutput("Data packets received per node after die time");   // simple output, used at all nodes
  	declareOutput("Data packets sent to neighbour node before die time");   // simple output, used at all nodes
  	declareOutput("Data packets sent to neighbour node after die time");   // simple output, used at all nodes
  	declareOutput("Packets received within n hops");   // simple output, only used at sink

  	trace() << "here.";

	neighbourList = new FloodingNeighbourList(this);

	setTimer(FR_TIMER_MAXSTARTUP, maxStartupDelay);

	// the sink must periodically rebroadcast discovery packets so it doesn't
	// time out
	//if (isSink)
		setTimer(FR_TIMER_SINKDISCOVERY, 30);

}

/**
 *  Called by the simulator to signal a timer interrupt.
 *  Dispatches flow of control to an appropriate handler method for the timer
 *  that has fired (or sets startup to true in the case of the MAXSTARTUP
 *  timer).
 *
 *  @param timer Index of the timer that fired (corresponding to definition
 *               in the enumerated type SmacTimers). Passed in by the simulator.
 */
void FloodingRouting::timerFiredCallback(int timer) {
	switch(timer) {
	case FR_TIMER_SINKDISCOVERY:
	case FR_TIMER_STARTUP:   handleStartupTimerCallback(); break;
	case FR_TIMER_MAXSTARTUP:   startupComplete=true; break;
	default: trace() << "ERROR: Unrecognised timer callback.";
	}
}

/**
 *  Handler method for FLOODING_TIMER_STARTUP.
 *  Creates a setup packet and broadcasts it on the medium.
 *  If the current node is the sink, the method sets the SINKDISCOVERY
 *  timer to periodically rebroadcast the setup packet.
 */
void FloodingRouting::handleStartupTimerCallback() {
  	/* make sure our neighbours know about us */
  	if (printDebugInfo)
  		trace() << "Sending setup packet.";
  	FloodingRoutingPacket* setupPacket =
  			new FloodingRoutingPacket("flooding routing setup packet",
  					                  NETWORK_LAYER_PACKET);
  	setupPacket->setFloodingRoutingPacketKind(SETUP_PACKET);
  	setupPacket->setSource(SELF_NETWORK_ADDRESS);
  	setupPacket->setDestination(BROADCAST_NETWORK_ADDRESS);
  	toMacLayer(setupPacket, BROADCAST_MAC_ADDRESS);

  	if (!isSink) {
  		trace() << "Initialisation complete.";
  	} else {
  		setTimer(FR_TIMER_SINKDISCOVERY, 30);
  	}

}


void FloodingRouting::fromMacLayer(cPacket* pkt, int srcMacAddress,
		double RSSI, double LQI) {
  // srcMacAddress is the last hop mac address
	/*
	 *  Firstly, add the mac address from which we received the packet to
	 *  our list of neighbours.
	 */
	if (printDebugInfo)
		trace() << "Adding " << srcMacAddress << " to neighbour list.";
	neighbourList->add(new Neighbour(srcMacAddress, this));

	/*
	 *  Now, process the packet. If we've seen it before, ignore it. If not,
	 *  pass it up to the application layer if we are the sink. Otherwise,
	 *  forward it to all neighbours except the one it came from.
	 */
	int dstMacAddress;
	FloodingRoutingPacket *netPacket =
			dynamic_cast <FloodingRoutingPacket*>(pkt);

	if (!netPacket) {
		trace() << "Error casting received packet.";
		return;
	}

	int thisSeqNumber = netPacket->getSequenceNumber();
	const char* sourceNetAddress = netPacket->getSource();

	if (isSink &&
			netPacket->getFloodingRoutingPacketKind() == FLOODING_DATA_PACKET) {
		/*trace() << "SINK received packet " << thisSeqNumber << " from " <<
				netPacket->getSource() << " in " << netPacket->getNumHops() <<
				" hops.";*/
	}


	if (!isNotDuplicatePacket(netPacket)) {
		if (printDebugInfo)
			trace() << "Already seen this packet, ignoring";
		return;
	} else {
		if (printDebugInfo)
			trace() << "Packet received at routing layer was unique.";
	}

	switch (netPacket->getFloodingRoutingPacketKind()) {
	case FLOODING_DATA_PACKET : {
		trace() << "Got a flooding data packet";
		//collectOutput("Data packets received per node before die time", atoi(sourceNetAddress));
		collectOutput("Data packets received per node before die time", srcMacAddress);
		/*
		 *  If we are the sink node, then we pass the data up to the next layer
		 *  and do not forward it.
		 */
		if (isSink) {

			trace() << "SINK received packet " << thisSeqNumber << " from " <<
							netPacket->getSource() << " in " << netPacket->getNumHops() <<
							" hops.";

			collectOutput("Packets received per node", atoi(netPacket->getSource()));
			collectOutput("Packets received within n hops", netPacket->getNumHops());
			toApplicationLayer(netPacket->decapsulate());
		} else {
			netPacket->setNumHops(netPacket->getNumHops()+1);

			// first, get list of current neighbours
			list<Neighbour*> neighbours = neighbourList->pickAllNeighbours();
			list<Neighbour*>::const_iterator it;

			// now forward the packet to all neighbours
			// (except the one it came from which it came)
			for (it = neighbours.begin(); it != neighbours.end(); ++it) {
				if ((dstMacAddress = (*it)->getMacAddress()) != srcMacAddress) {
					trace() << "Forwarding packet to " << dstMacAddress;

					// NB the packet is deleted in this method call
					// (needs duplicating)!!!
					toMacLayer(netPacket->dup(), dstMacAddress);
					trace() << "done";
				}
			}

		}
		break;
	}
	case SETUP_PACKET : {
		trace() << "Got a flooding setup packet from " << srcMacAddress;

		// return it (duplicates have already been discarded)
		FloodingRoutingPacket* setupPacket =
		  			new FloodingRoutingPacket("flooding routing setup packet",
		  					                  NETWORK_LAYER_PACKET);
		setupPacket->setFloodingRoutingPacketKind(SETUP_PACKET);
		setupPacket->setSource(SELF_NETWORK_ADDRESS);
		setupPacket->setDestination(netPacket->getSource());
		toMacLayer(setupPacket, srcMacAddress);

		trace() << "returned setup packet.";

		break;
	}
	default : {
		trace() << "Unrecognised packet received.";
		return;
	}
	}
}

/**
 *  Called by the simulator at the end of the simulation.
 *  Deletes neighbour list and prints a message to show we've finished.
 */
void FloodingRouting::finish() {
	delete neighbourList;
	trace() << "Flooding routing finished!";
}

/**
 * When we receive data from the application layer, send it to all neighbours.
 */
void FloodingRouting::fromApplicationLayer(cPacket * pkt,
		const char *destination) {
	if (!startupComplete) {
		trace() << "Startup not complete. " <<
				"Ignoring packet from application layer.";
		return;
	}
	if (isSink) {
		trace() << "Sink node cannot originate data. " <<
				"Ignoring packet from application layer.";
		return;
	}

	collectOutput("Packets originated", atoi(SELF_NETWORK_ADDRESS));

	// encapsulate the packet...
	//trace() << "app method called";
	FloodingRoutingPacket *netPacket = new FloodingRoutingPacket
			("FloodingRouting packet", NETWORK_LAYER_PACKET);
	encapsulatePacket(netPacket, pkt);
	netPacket->setSource(SELF_NETWORK_ADDRESS);
	netPacket->setFloodingRoutingPacketKind(FLOODING_DATA_PACKET);
	//netPacket->setDestination(destination);
	netPacket->setDestination(SINK_NETWORK_ADDRESS);

	// increment our sequence number and set it
	netPacket->setSequenceNumber(++mySeqNumber);

	// initialise hop count
	netPacket->setNumHops(1);

	// ...and forward it to all neighbours
	list<Neighbour*> neighbours = neighbourList->pickAllNeighbours();
	list<Neighbour*>::const_iterator it;
	for (it = neighbours.begin(); it != neighbours.end(); ++it) {
		if (printDebugInfo)
			trace() << "Sending new packet to neighbour " << (*it)->getMacAddress();
		toMacLayer(netPacket->dup(), (*it)->getMacAddress());
	}

}
