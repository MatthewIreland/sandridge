//
//  MacawPacket.msg
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

enum MacawPacketType {
    MACAW_PACKET_DATA = 1;
	MACAW_PACKET_RTS = 2;
	MACAW_PACKET_CTS = 3;
	MACAW_PACKET_DS = 4;
	MACAW_PACKET_ACK = 5;
	MACAW_PACKET_RRTS = 6;
};

packet MacawPacket extends MacPacket {
	int type enum (MacawPacketType);  // 1 byte
	int localBackoff;
	int remoteBackoff;
	int retryCount;
}
 