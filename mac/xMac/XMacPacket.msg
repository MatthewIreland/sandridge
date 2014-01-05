//
//  XmacPacket.msg
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

enum XmacPacketType {
    XMAC_PACKET_DATA = 1;
    XMAC_PACKET_PREAMBLE = 2;
    XMAC_PACKET_PREAMBLE_ACK = 3;
	XMAC_PACKET_RTS = 4;
	XMAC_PACKET_CTS = 5;
	XMAC_PACKET_ACK = 6;
};

packet XMacPacket extends MacPacket {
	int type enum (XmacPacketType);  // 1 byte
}
 