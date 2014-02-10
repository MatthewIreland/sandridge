/**
 *  CastaliaIncludes.h
 *  Matthew Ireland, mti20, University of Cambridge
 *  14th November MMXIII
 *
 *  Mock object/class/method/type definitions for those defined by
 *  Castalia/OMNeT++, to avoid having to include their paths in the IDE.
 *  Keeps Eclipse happy without lots of overhead.
 *
 *  The inclusion of this file is removed from source code during the build
 *  process.
 */

#ifndef CASTALIA_INCLUDES_H_
#define CASTALIA_INCLUDES_H_

class cPacket;
class BmacDataPacket;
class SMacPacket;
class BmacPreamblePacket;
class VirtualApplication;
class VirtualRouting;
class SandridgeApplicationPacket;
void setTimer(int, int);
#define Define_Module
#define CollectOutput
#define SET_STATE
#define SLEEP
#define declareOutput
cPacket* createRadioCommand(int, int);
void toRadioLayer(cPacket*);
void toNetworkLayer(cPacket*);
bool par(string);
int par(string);
string par(string);
#define trace() cout
#define MAC_LAYER_PACKET 0
class CCA_result;
class XMacPacket;
class BMacPacket;
typedef int simtime_t ;
class MacawPacket;
#define CLEAR 0
#define BUSY 1
#define BROADCAST_MAC_ADDRESS 0
#define SELF_MAC_ADDRESS 0
void opp_error(string);
cPacket* decapsulatePacket(BMacPacket*);
cPacket* decapsulatePacket(SMacPacket*);
cPacket* decapsulatePacket(XMacPacket*);
void toNetworkLayer(cPacket*);
void toRadioLayer(MacawPacket*);
void toRadioLayer(BMacPacket*);
void toRadioLayer(SMacPacket*);
void toRadioLayer(XMacPacket*);
void toRadioLayer(cPacket*);
void collectOutput(string, int);
void cancelTimer(int);
void getTimer(int);
#define BMAC_PACKET_ACK 0
#define BMAC_PACKET_PREAMBLE 0
#define BMAC_PACKET_DATA 0
#define MACAW_PACKET_DATA 0
#define MACAW_PACKET_RTS 0
#define MACAW_PACKET_CTS 0
#define MACAW_PACKET_DS 0
#define MACAW_PACKET_ACK 0
#define MACAW_PACKET_RRTS 0
#define SMAC_PACKET_RTS 0
#define SMAC_PACKET_CTS 0
#define SMAC_PACKET_ACK 0
#define SMAC_PACKET_DATA 0
#define SMAC_PACKET_SYNC 0

#endif   /* CASTALIA_INCLUDES_H_ */

