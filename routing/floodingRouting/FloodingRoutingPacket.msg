//
//  FloodingRoutingPacket.msg
//  Matthew Ireland, University of Cambridge
//  27th December MMXIII
//
//  Extends the RoutingPacket class with a type field (SETUP or DATA), and a
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

enum floodingRoutingPacketDef {
	FLOODING_DATA_PACKET = 1;
	SETUP_PACKET = 2;
} 

packet FloodingRoutingPacket extends RoutingPacket {
	int floodingRoutingPacketKind enum (floodingRoutingPacketDef);
	int numHops;
}