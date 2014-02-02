//
//  RandomRoutingPacket.msg
//  Matthew Ireland, University of Cambridge
//  2nd January MMXIV
//
//  Extends the RoutingPacket class with a type field (DATA or SETUP), and a
//  field containing the number of hops that the packet has currently
//  undertaken since its inception (to be updated by each intermediate node). 
//
 
cplusplus {{
#include "RoutingPacket_m.h"
}}

class RoutingPacket;

// Routing packet contains:
//  string source;
//  string destination;
//  unsigned int sequenceNumber;
// 

enum randomRoutingPacketDef {
	RANDOM_DATA_PACKET = 1;
	SETUP_PACKET = 2;
} 

packet RandomRoutingPacket extends RoutingPacket {
	int randomRoutingPacketKind enum (randomRoutingPacketDef);
	int numHops;
}