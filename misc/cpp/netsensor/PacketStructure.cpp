#include "PacketStructure.h"

using namespace NET_SENSOR;

PacketStructure::PacketStructure()
	: receivedTimeStamp(0), classification(0), received(0)

{
}

PacketStructure::~PacketStructure()
{
	//if (ui8Buf != 0) {
	//	delete[](ui8Buf);
	//	ui8Buf = 0;
	//}
}