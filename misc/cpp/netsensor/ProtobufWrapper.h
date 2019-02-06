#ifndef NETSENSOR_ProtobufWrapper__INCLUDED
#define NETSENSOR_ProtobufWrapper__INCLUDED
/*
* ProtobufWrapper.h
* Author: bordway@ihmc.us
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
* The TCPRTTTable is a tiered hashtable containing the source ip, dest ip,
* source port, dest port, and finally the object for the TCP calculations.
* This class stores outgoing TCP information, and calculates the RTT when
* an incoming TCP packet is received. It does not work in the opposite way.
*
*/

#include <google/protobuf/timestamp.pb.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#endif
#ifdef LINUX 
#include <sys/time.h>
#endif

#include "measure.pb.h"
#include "container.pb.h"
#include "DefaultValues.h"
#include "subject.pb.h"
#include <google/protobuf/timestamp.pb.h>
#include "StrClass.h"

#include "IWStationDump.h"


namespace IHMC_NETSENSOR
{
    class ProtobufWrapper
    {
    public:
        ProtobufWrapper(void);
        ~ProtobufWrapper(void);
        measure::Measure * createNewMeasure();
        
        measure::Measure * getMeasureIW (NOMADSUtil::String sSensorIP, IWStationDump& dump);

        measure::Measure * getMeasureRTT(NOMADSUtil::String sSensorIP, NOMADSUtil::String sSourceIP,
            NOMADSUtil::String sDestIP, NOMADSUtil::String sProtocol,
            NOMADSUtil::String sSourcePort, NOMADSUtil::String sDestPort,
            long minRTT, long maxRTT, long mostRecentRTT, long resolution, float avgRTT);

        void setSubject(measure::Measure * pMeasure, measure::Subject subject);
        void setMeasureTimestamp(measure::Measure * pMeasure, google::protobuf::Timestamp * pTs);
    private:
    };
}

#endif
