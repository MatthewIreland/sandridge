//
//  BMacPacket.msg
//  Matthew Ireland, University of Cambridge
//  29th December MMXIII
//
//  Extends the generic MacPacket class.
//
//  MacPacket contains the following fields:
//     MacRadioInfoExchange_type macRadioInfoExchange;    // bytes = ? 
//     int source;
//     int destination;
//     unsigned int sequenceNumber;
//
 
cplusplus {{
 #include "MacPacket_m.h"
}}
 
class MacPacket;

enum BMacPacketType {
    BMAC_PACKET_DATA = 1;
    BMAC_PACKET_PREAMBLE = 2;
    BMAC_PACKET_ACK = 3;
};

packet BMacPacket extends MacPacket {
	int type enum (BMacPacketType);  // 1 byte
}
 