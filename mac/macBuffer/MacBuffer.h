/**
 *  MacBuffer.h
 *  Matthew Ireland, mti20, University of Cambridge
 *  20th November MMXIII
 *
 *  Stores data packets between layers-2 and -3 until they are ready for
 *  transmission.
 *
 */

#ifndef MACBUFFER_H_
#define MACBUFFER_H_

#include <queue>
#include "MockObjects.h"
#include "VirtualMac.h"
#include "MacBufferFullException.h"

using namespace std;

template <typename T>
class MacBuffer {
private:
	queue<T> buffer;
	CastaliaModule* castalia;
	int maxSize;
	bool printDebugInfo;
public:
	MacBuffer(CastaliaModule* castalia, int maxSize, bool printDebugInfo);
	void insertPacket(T packet);
	T peek();
	int numPackets();
	void removeFirst();
	inline bool isEmpty() { return (buffer.empty()); };
	virtual ~MacBuffer();
};

#endif /* MACBUFFER_H_ */
