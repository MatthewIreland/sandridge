/**
 *  MacBuffer.cc
 *  Matthew Ireland, mti20, University of Cambridge
 *  20th November MMXIII
 *
 *  Stores data packets between layers-2 and -3 until they are ready for
 *  transmission.
 *
 */

#include "MacBuffer.h"
#include "../macaw/MacawPacket_m.h"
#include "../sMac/SMacPacket_m.h"
#include "../bMac/BMacPacket_m.h"
#include "../xMac/XMacPacket_m.h"


template <typename T>
MacBuffer<T>::MacBuffer(CastaliaModule* castalia,
		int maxSize,
		bool printDebugInfo) : castalia (castalia),
		                       maxSize (maxSize),
		                       printDebugInfo (printDebugInfo) {};

template <typename T>
void MacBuffer<T>::insertPacket(T packet) {
	if (buffer.size() >= maxSize)
		throw MacBufferFullException();
	buffer.push(packet);
	if (printDebugInfo) {
		//castalia->trace() << "Added a packet to the MAC buffer. "
		//		          << "Number of buffered packets: "
		//		          << buffer.size();
	}
}

template <typename T>
T MacBuffer<T>::peek() {
	return buffer.front();
}

template <typename T>
int MacBuffer<T>::numPackets() {
	return buffer.size();
}

template <typename T>
void MacBuffer<T>::removeFirst() {
	buffer.pop();
}

template <typename T>
MacBuffer<T>::~MacBuffer() {
	// nothing to do here
}

template class MacBuffer<int>;
template class MacBuffer<MacawPacket*>;
template class MacBuffer<SMacPacket*>;
template class MacBuffer<BMacPacket*>;
template class MacBuffer<XMacPacket*>;


