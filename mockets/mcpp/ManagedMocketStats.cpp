#include "ManagedMocketStats.h"

using namespace us::ihmc::mockets;

ManagedMocketStats::ManagedMocketStats (MocketStats *pRealStats)
{
    _pRealStats = pRealStats;
}

ManagedMocketStats::!ManagedMocketStats()
{
}

ManagedMocketStats::~ManagedMocketStats()
{
    ManagedMocketStats::!ManagedMocketStats();
}

uint32 ManagedMocketStats::getRetransmitCount (void)
{
    return _pRealStats->getRetransmitCount();
}

uint32 ManagedMocketStats::getSentPacketCount (void)
{
    return _pRealStats->getSentPacketCount();
}

uint32 ManagedMocketStats::getSentByteCount (void)
{
    return _pRealStats->getSentByteCount();
}

uint32 ManagedMocketStats::getReceivedPacketCount (void)
{
    return _pRealStats->getReceivedPacketCount();
}

uint32 ManagedMocketStats::getReceivedByteCount (void)
{
    return _pRealStats->getReceivedByteCount();
}

uint32 ManagedMocketStats::getDuplicatedDiscardedPacketCount (void)
{
    return _pRealStats->getDuplicatedDiscardedPacketCount();
}

uint32 ManagedMocketStats::getNoRoomDiscardedPacketCount (void)
{
    return _pRealStats->getNoRoomDiscardedPacketCount();
}
