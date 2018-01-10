#ifndef NETSENSOR_NetSensorPacketQueue__INCLUDED
#define NETSENSOR_NetSensorPacketQueue__INCLUDED
/*
* NetSensorPacketQueue.h
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 3 (GPLv3) as published by the Free Software Foundation.
*
* U.S. Government agencies and organizations may redistribute
* and/or modify this program under terms equivalent to
* "Government Purpose Rights" as defined by DFARS
* 252.227-7014(a)(12) (February 2014).
*
* Alternative licenses that allow for use within commercial products may be
* available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
*/
#include "NetSensorPacketQueue.h"
#include "NetSensorPacket.h"
#include "Queue.h"
#include "Mutex.h"
#include "NLFLib.h"

namespace IHMC_NETSENSOR
{
class NetSensorPacketQueue
{
public:
    void dequeue(NetSensorPacket & p);
    bool mutexTest(void);
    bool enqueue(const NetSensorPacket & packet);
    NetSensorPacketQueue(uint32 queueMaxSize = 500);
    ~NetSensorPacketQueue(void);
//<--------------------------------------------------------------------------->
private:
    uint32 _uint32Size;
    uint32 _uint32QueueMaxSize;
    NOMADSUtil::Queue<NetSensorPacket> _pPQueue;
    NOMADSUtil::Mutex _pQMutex;
};

inline NetSensorPacketQueue::NetSensorPacketQueue(uint32 queueMaxSize)
{
    _uint32QueueMaxSize = queueMaxSize;
}

inline NetSensorPacketQueue::~NetSensorPacketQueue(void) { }

inline bool NetSensorPacketQueue::enqueue(const NetSensorPacket & p)
{
    if (_pQMutex.lock() == NOMADSUtil::Mutex::RC_Ok) {
        if ((_uint32Size = _pPQueue.size()) < _uint32QueueMaxSize) {
            _pPQueue.enqueue(p);
            _pQMutex.unlock();
            return true;
        }
        else {
            _pQMutex.unlock();
            return false;
        }
    }
    return false;
}

inline void NetSensorPacketQueue::dequeue(NetSensorPacket & p)
{
    if (_pQMutex.lock() == NOMADSUtil::Mutex::RC_Ok) {
        if (_pPQueue.dequeue(p) != 0) {
            p = EMPTY_PACKET;
        }
        _pQMutex.unlock();
    }
}

inline bool NetSensorPacketQueue::mutexTest(void)
{
    if (_pQMutex.lock() == NOMADSUtil::Mutex::RC_Ok) {
        _pQMutex.unlock();
        return true;
    }
    else {
        return false;
    }
}
}
#endif
