#ifndef NETSENSOR_MicroflowNestedHashTable__INCLUDED
#define NETSENSOR_MicroflowNestedHashTable__INCLUDED

/*
* MicroflowNestedHashTable.h
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
* This Class impelemnts a five levels hash map that will contain
* stats identified by source ip, dest ip, protocol, source port, dest port
*/
#include"StringHashtable.h"
#include"TrafficElement.h"
#include"MicroflowId.h"
#include"NLFLib.h"
#include"NetSensorConstants.h"
#include"traffic.pb.h"
#include"NetSensorUtilities.h"

#define levelFive NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<TrafficElement> > > > >
#define levelFour NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<TrafficElement> > > >
#define levelThree NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<TrafficElement> > >
#define levelTwo NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<TrafficElement> >
#define levelOne NOMADSUtil::StringHashtable<TrafficElement>

namespace IHMC_NETSENSOR
{
class MicroflowNestedHashTable
{
public:
    MicroflowNestedHashTable();
    ~MicroflowNestedHashTable();

    /*
    * ui32CleaningNumber : Maximum number of cleanings to perform
    *  Returns : Number of entries removed.
    */
    uint32 cleanTable(uint32 ui32CleaningNumber);
    void fillTrafficProto(netsensor::TrafficByInterface * pT);

    TrafficElement* get(const char* sSA, const char* sDA,
        const char* sProtocol, const char* sSP, const char* sDP);
    void put(const char*sSA, const char*sDA, const char*sProtocol,
        const char*sSP, const char*sDP, TrafficElement *t);
    void print();

private:
    //sip,dip,prot,sp,dp
    levelFive _microflowNestedHashTable;
    NOMADSUtil::Mutex _pTMutex;
};


inline MicroflowNestedHashTable::~MicroflowNestedHashTable()
{

}

inline MicroflowNestedHashTable::MicroflowNestedHashTable()
    : _microflowNestedHashTable(false, true, true, true) { }

}
#endif
