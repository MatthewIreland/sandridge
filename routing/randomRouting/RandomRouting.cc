/*
 *  RandomRouting.cc
 *  Matthew Ireland, University of Cambridge
 *  2nd January MMXIV
 *
 *  Implements Random Walk Routing; that is, whenever a node receives a new
 *  packet, it picks a neighbour at random to which it should be forwarded.
 */


#include "RandomRouting.h"
#include "RandomRoutingPacket_m.h"
#include <ctime> 
#include <cstdlib>

Define_Module(RandomRouting);

void RandomRouting::startup() {
	startupComplete = false;

	/* Check that the Application module used has the boolean parameter "isSink"
	 * If we are the sink, then we pass data up to the application layer,
	 * otherwise we simply forward it (the sink node(s) do(es) not forward data).
	 */
  	cModule *parentMod =
  			getParentModule()->getParentModule()->getSubmodule("Application");
  	if (parentMod->hasPar("isSink")) {
  		isSink = parentMod->par("isSink");
  	} else {
  		opp_error("\nNeed isSink parameter");
  	}

  	printDebugInfo = par("printDebugInfo");

	/* after some random time, make sure our neighbours know about us */
  	// seed with a number unique to us, otherwise all nodes will have the
  	// same startup delay (converts const char* to int)
	int seed = atoi(SELF_NETWORK_ADDRESS);
	srand(seed);
	minStartupDelay = par("minStartupDelay");
	maxStartupDelay = par("maxStartupDelay");
	int startupDelay = minStartupDelay+(rand() %
			(maxStartupDelay-minStartupDelay));

	strength = par("strength");

	if (isSink)
		setTimer(RR_TIMER_STARTUP, 0.1);
	else
		setTimer(RR_TIMER_STARTUP, startupDelay);


  	if (printDebugInfo)
	  trace() << "Sending startup packet in " << startupDelay << " seconds.";

  	// simple output, only used at sink
  	declareOutput("Packets received per node");
    // simple output, used at all nodes
  	declareOutput("Data packets received per node before die time");
    // simple output, used at all nodes
	declareOutput("Data packets received per node after die time");
	// simple output, used at all nodes
  	declareOutput("Data packets sent to neighbour node before die time");
  	// simple output, used at all nodes
  	declareOutput("Data packets sent to neighbour node after die time");
    // simple output, used at every node
  	declareOutput("Packets originated");
  	// simple output, only used at sink
  	declareOutput("Packets received within n hops");

  	hasEnded = false;   // we are not allowed to originate
  	isAlive = true;     // we are not allowed to do anything after we have died

  	int endTime = par("endTime");
  	int dieTime = par("dieTime");
  	nodeShouldDie = par("nodeShouldDie");

  	neighbourOldAge = par("neighbourOldAge");
  	rediscoverTime = par("rediscoverTime");

  	setTimer(RR_TIMER_END, endTime);
  	setTimer(RR_TIMER_DIE, dieTime);
  	setTimer(RR_TIMER_START, maxStartupDelay);

	neighbourList = new RandomNeighbourList(this, par("neighbourTimeout"),
			SELF_MAC_ADDRESS);

}

void RandomRouting::timerFiredCallback(int timer) {
	switch(timer) {
	case RR_TIMER_STARTUP:    handleStartupTimerCallback(); break;
	case RR_TIMER_START:      handleStartTimerCallback();   break;
	case RR_TIMER_DIE:        handleDieTimerCallback();     break;
	case RR_TIMER_REDISCOVER: rediscoverNeighbours();       break;
	case RR_TIMER_END:        handleEndTimerCallback();     break;
	default: trace() << "ERROR: Unrecognised timer callback.";
	}
}

void RandomRouting::handleStartupTimerCallback() {
  	/* make sure our neighbours know about us */
  	if (printDebugInfo)
  		trace() << "Sending setup packet.";
  	RandomRoutingPacket* setupPacket =
  			new RandomRoutingPacket("random routing setup packet",
  					NETWORK_LAYER_PACKET);
  	setupPacket->setRandomRoutingPacketKind(SETUP_PACKET);
  	setupPacket->setSource(SELF_NETWORK_ADDRESS);
  	setupPacket->setDestination(BROADCAST_NETWORK_ADDRESS);
  	toMacLayer(setupPacket, BROADCAST_MAC_ADDRESS);

  	setTimer(RR_TIMER_REDISCOVER, rediscoverTime);

	trace() << "Initialisation complete.";
}

void RandomRouting::handleDieTimerCallback() { // consider making this inline
	isAlive = (!nodeShouldDie);
	dieTimeHasPassed = true;
	printInfo("Node has died :(");
}

void RandomRouting::handleStartTimerCallback() { // consider making this inline
	printInfo("All nodes have started up. Starting main body of simulation");
	startupComplete = true;

}

void RandomRouting::handleEndTimerCallback() { // consider making this inline
	//printInfo("Will not originate new data from this point");
	//hasEnded = true;
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
 *  Used to print the input string to the trace file.
 *
 *  @param info String to print to the simulator trace file.
 */
void RandomRouting::printInfo(string info) {
	trace() << info << ".";
}

void RandomRouting::rediscoverNeighbours() {
	if (startupComplete) {
		list<Neighbour*> oldNeighbours;
		neighbourList->getOldNeighbours(oldNeighbours, getClock(), neighbourOldAge);
		list<Neighbour*>::const_iterator it;
		for (it = oldNeighbours.begin(); it != oldNeighbours.end(); ++it) {
			trace() << "Sending setup packet to " << (*it)->getMacAddress();
			RandomRoutingPacket* setupPacket =
					new RandomRoutingPacket("random routing setup packet",
							NETWORK_LAYER_PACKET);
			setupPacket->setRandomRoutingPacketKind(SETUP_PACKET);
			setupPacket->setSource(SELF_NETWORK_ADDRESS);
			setupPacket->setDestination("tmpdst");  // TODO fixme
			toMacLayer(setupPacket, (*it)->getMacAddress());
		}
	}
	trace() << "resetting rediscover timer for " << rediscoverTime
			<< " seconds.";
	setTimer(RR_TIMER_REDISCOVER, rediscoverTime);
}

/**
 *  Called by the simulator at the end of the simulation.
 *  Deletes neighbour list and prints a message to show we've finished.
 */
void RandomRouting::finish() {
	delete neighbourList;
	trace() << "Random walk routing finished!";
}


void RandomRouting::fromMacLayer(cPacket* pkt, int srcMacAddress,
		double RSSI, double LQI) {
	if (!isAlive)
		return;
  // srcMacAddress is the last hop mac address
	/*
	 *  Firstly, add the mac address from which we received the packet to
	 *  our list of neighbours.
	 */
	if (printDebugInfo)
		trace() << "Adding " << srcMacAddress << " to neighbour list.";
	trace() << "here.";
	neighbourList->add(new Neighbour(srcMacAddress, this, getClock()),
			getClock());
	trace() << "here 2.";

	/*
	 *  Now, process the packet. If we've seen it before, ignore it. If not,
	 *  pass it up to the application layer if we are the sink. Otherwise,
	 *  forward it to all neighbours except the one it came from.
	 */
	int dstMacAddress;
	RandomRoutingPacket *netPacket = dynamic_cast <RandomRoutingPacket*>(pkt);

	trace() << "here 3.";

	if (!netPacket) {
		trace() << "Error casting received packet.";
		return;
	}

	int thisSeqNumber = netPacket->getSequenceNumber();
	const char* sourceNetAddress = netPacket->getSource();

	switch (netPacket->getRandomRoutingPacketKind()) {
	case RANDOM_DATA_PACKET : {
		trace() << "Got a data packet";

		if (dieTimeHasPassed) {
			collectOutput("Data packets received per node after die time",
					atoi(sourceNetAddress)); // simple output, used at all nodes
		} else {
			//collectOutput("Data packets received per node before die time", atoi(sourceNetAddress));   // simple output, used at all nodes
			collectOutput("Data packets received per node before die time", srcMacAddress);   // simple output, used at all nodes
		}

		/*
		 *  If we are the sink node, then we pass the data up to the next layer
		 *  and do not forward it.
		 */
		if (isSink) {
			collectOutput("Packets received per node", atoi(netPacket->getSource()));
			//collectOutput("Packets received within n hops", netPacket->getNumHops());

			//if (isNotDuplicatePacket(netPacket)) {
			trace() << "SINK received packet " << thisSeqNumber << " from " <<
										netPacket->getSource() << " in "
										<< netPacket->getNumHops() <<
										" hops.";
			if (isNotDuplicatePacket(netPacket))
			toApplicationLayer(netPacket->decapsulate()); //}
			return;
		} else {
			netPacket->setNumHops(netPacket->getNumHops()+1);

			for (int z=0; z<strength; z++) {
			// first, get list of current neighbours
			Neighbour* neighbour =
					neighbourList->pickRandomNeighbour(srcMacAddress, getClock());
			int dstMacAddress = neighbour->getMacAddress();
			trace() << "Forwarding packet to neighbour " << dstMacAddress
					<< ". It's had "
					<< netPacket->getNumHops() << " hops so far.";

			toMacLayer(netPacket->dup(), dstMacAddress);      // NB the packet is deleted in this method call (needs duplicating)!!!
			if (dieTimeHasPassed) {
				collectOutput("Data packets sent to neighbour node after die time", dstMacAddress);   // simple output, used at all nodes
			} else {
			  	collectOutput("Data packets sent to neighbour node before die time", dstMacAddress);   // simple output, used at all nodes
			}

			}
			return;
		}
		break;
	}
	case SETUP_PACKET : {
			trace() << "Got a random setup packet from " << srcMacAddress;

			if (isNotDuplicatePacket(netPacket)) {
				RandomRoutingPacket* setupPacket = new RandomRoutingPacket("random routing setup packet", NETWORK_LAYER_PACKET);
				setupPacket->setRandomRoutingPacketKind(SETUP_PACKET);
				setupPacket->setSource(SELF_NETWORK_ADDRESS);
				setupPacket->setDestination(sourceNetAddress);
				toMacLayer(setupPacket, srcMacAddress);
			}

		break;
	}
	}

}

/*
 * When we receive data from the application layer, send it to all neighbours.
 */
void RandomRouting::fromApplicationLayer(cPacket * pkt,
		const char *destination) {
	if (!isAlive)
		return;
	if (!startupComplete) {
		trace() << "Startup not complete. Ignoring packet from application layer.";
		return;
	}
	if (isSink) {
		trace() << "Sink node cannot originate data. Ignoring packet from application layer.";
		return;
	}

	if (hasEnded) {
		trace() << "Ignoring application packet. Ended.";
		return;
	} else {
		collectOutput("Packets originated", atoi(SELF_NETWORK_ADDRESS));
	}


	// encapsulate the packet...
	//trace() << "app method called";
	RandomRoutingPacket *netPacket = new RandomRoutingPacket
			("RandomRouting packet", NETWORK_LAYER_PACKET);
	encapsulatePacket(netPacket, pkt);
	netPacket->setSource(SELF_NETWORK_ADDRESS);
	netPacket->setRandomRoutingPacketKind(RANDOM_DATA_PACKET);
	//netPacket->setDestination(destination);
	netPacket->setDestination(SINK_NETWORK_ADDRESS);
	netPacket->setNumHops(1);    // first hop = us to next next node (alternative would be to initialise to 0 and increment after final hop at sink)

	// ...increment our sequence number...
	netPacket->setSequenceNumber(++mySeqNumber);
	//
	// ...and forward it to a random neighbour
	Neighbour* neighbour = neighbourList->pickRandomNeighbour(getClock());
	if (neighbour != NULL) {
		int dstMacAddress = neighbour->getMacAddress();
		if (printDebugInfo)
			trace() << "Sending new packet to neighbour " << dstMacAddress;
		toMacLayer(netPacket->dup(), dstMacAddress);   // TODO stop duplicating!!!

		if (dieTimeHasPassed) {
			collectOutput("Data packets sent to neighbour node after die time", dstMacAddress);   // simple output, used at all nodes
		} else {
		  	collectOutput("Data packets sent to neighbour node before die time", dstMacAddress);   // simple output, used at all nodes
		}
	} else {
		trace() << "We have no neighbours so can't forward our new packet!";
	}

}
