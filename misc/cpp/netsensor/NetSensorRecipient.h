#ifndef NETSENSOR_NetSensorRecipient__INCLUDED
#define NETSENSOR_NetSensorRecipient__INCLUDED
/*
* NetSensorRecipient.h
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
*
* This class stores information about all the stats recipients
*/
#include"UDPDatagramSocket.h"
namespace IHMC_NETSENSOR
{
class NetSensorRecipient
{
public:
    NetSensorRecipient();
    ~NetSensorRecipient();
    NetSensorRecipient(const char *addr, uint32 port);
    NetSensorRecipient(const NetSensorRecipient& temp);

//<--------------------------------------------------------------------------->
public:
    NOMADSUtil::UDPDatagramSocket *pNotifierSocket;
    NOMADSUtil::InetAddr *notifyAddr;
};

inline NetSensorRecipient::~NetSensorRecipient()
{
    pNotifierSocket->close();
    delete pNotifierSocket;
    notifyAddr->clear();
    delete notifyAddr;
}

inline NetSensorRecipient::NetSensorRecipient() 
{
    pNotifierSocket = new NOMADSUtil::UDPDatagramSocket();
    pNotifierSocket->init();
    notifyAddr = new NOMADSUtil::InetAddr();

}

inline NetSensorRecipient::NetSensorRecipient(const char *addr, uint32 port) 
{
    pNotifierSocket = new NOMADSUtil::UDPDatagramSocket();
    pNotifierSocket->init();
    notifyAddr = new NOMADSUtil::InetAddr(addr, port);
}  

inline NetSensorRecipient::NetSensorRecipient(const NetSensorRecipient& temp)
{         
    pNotifierSocket = new NOMADSUtil::UDPDatagramSocket();
    pNotifierSocket->init();
    notifyAddr = new NOMADSUtil::InetAddr(temp.notifyAddr->getIPAsString(), 
        temp.notifyAddr->getPort());
}
}
#endif