#ifndef INCL_PACKET_STRUCTURE
#define INCL_PACKET_STRUCTURE

#include "FTypes.h"

namespace NET_SENSOR
{
	class  PacketStructure {
	public:
		PacketStructure();
		~PacketStructure();
	
	
		uint8 ui8Buf[9038U];
		//uint8 *ui8Buf;
		int64 receivedTimeStamp;
		int classification;
		int received;
	};
}

#endif