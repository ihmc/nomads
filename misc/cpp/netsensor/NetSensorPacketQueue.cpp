/*
* NetSensorPacketQueue.cpp
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
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
NetSensorPacketQueue::NetSensorPacketQueue (const uint32 queueMaxSize)
{
    _uint32QueueMaxSize = queueMaxSize;
}

bool NetSensorPacketQueue::enqueue (const NetSensorPacket& p)
{
    if (_mutex.lock() == Mutex::RC_Ok) {
        if ((_uint32Size = _pPQueue.size()) < _uint32QueueMaxSize) {
            _pPQueue.enqueue (p);
            _mutex.unlock();
            return true;
        }
        else {
            _mutex.unlock();
            return false;
        }
    }
    return false;
}

void NetSensorPacketQueue::dequeue (NetSensorPacket& p)
{
    bool bRcvdSomething = false;
    while (!bRcvdSomething) {
        if (_mutex.lock() == Mutex::RC_Ok) {
            bRcvdSomething = _pPQueue.size() > 0;
            _mutex.unlock();
        }
        if (!bRcvdSomething) { 
            sleepForMilliseconds (50); 
        }  
    }

    if (_mutex.lock() == Mutex::RC_Ok) {
        if (_pPQueue.dequeue (p) != 0) { 
            p = EMPTY_PACKET;
        }
        _mutex.unlock();
    }
}

bool NetSensorPacketQueue::mutexTest(void)
{
    if (_mutex.lock() == Mutex::RC_Ok) {
        _mutex.unlock();
        return true;
    }
    else {
        return false;
    }
}
}