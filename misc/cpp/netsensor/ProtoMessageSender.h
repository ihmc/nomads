#ifndef NETSENSOR_ProtoMessageSender__INCLUDED
#define NETSENSOR_ProtoMessageSender__INCLUDED
/*
* ProtoMessageSender.h
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
* This class handle the fragmentation and sending of NetSensor Statistics
*
*
*/

#include"container.pb.h"
#include"InterfaceInfo.h"
#include"InterfaceMonitor.h"
#include"PtrLList.h"
#include"Logger.h"
#include"NetSensorConstants.h"
#include"NetSensorRecipient.h"
#include"NetSensorUtilities.h"
#include"StringHashtable.h"
#include"TopologyInfo.h"
#include"topology.pb.h"
#include"UDPDatagramSocket.h"
#include"ZlibBufferCompressionInterface.h"

#include <iostream>
#include <fstream>

namespace IHMC_NETSENSOR
{
class ProtoMessageSender
{
public:
    //Used to add an IP and Port to the delivery address list
    int addRecipient        (const char* pcAddr, const uint32 ui32Port);
    ProtoMessageSender      (void);
    ~ProtoMessageSender     (void);
    void sendNetproxyInfo   (netsensor::NetSensorContainer *pNSC);
    void sendTopology       (netsensor::NetSensorContainer *pNSC);
    void sendTraffic        (netsensor::NetSensorContainer *pNSC);
    void sendICMP           (netsensor::NetSensorContainer *pNsc);
    void configureToUseCompression();

private:
    enum ContainerType { CT_TRAFFIC, CT_TOPOLOGY };
    void initMicroflow(netsensor::Microflow *to, const netsensor::Microflow *from);
    void initStat(netsensor::Stat *to, const netsensor::Stat *from);

    void initContainer(netsensor::NetSensorContainer *pNSC, const google::protobuf::Timestamp *ts, netsensor::DataType dt);

    void initTopologyEl(netsensor::Topology *to, const netsensor::Topology *from);

    void initTrafficEl(netsensor::TrafficByInterface *to, const netsensor::TrafficByInterface *from);

    void initICMPContainer(netsensor::ProtoICMPInfoContainer *to, const netsensor::ProtoICMPInfoContainer *from);

    void initRequiredProtoICMPData(netsensor::ProtoData *to, const netsensor::ProtoData *from);

    int  sendSerializedData(const char* serializedData, const uint32 ui32Size);
    void sendTopologyFragment(netsensor::NetSensorContainer *newC);
    void sendTrafficFragment (netsensor::NetSensorContainer *newC);
    void sendICMPFragment    (netsensor::NetSensorContainer *pNewC);
    int  splitAndSendTopologyPacket(netsensor::NetSensorContainer *nsc);
    int  splitAndSendTrafficPacket (netsensor::NetSensorContainer *nsc);
    int  splitAndSendTrafficPacket2(netsensor::NetSensorContainer *nsc);
    int  splitAndSendICMPPacket    (netsensor::NetSensorContainer *pNsc);
//<--------------------------------------------------------------------------->
private:
    NOMADSUtil::PtrLList<NetSensorRecipient> _recipientList;
    NOMADSUtil::Mutex _pPMSMutex;
    uint32 _ui32Mtu;
    uint32 _ui32MaxPacketSize;
    bool _bUseCompression;
};
}
#endif