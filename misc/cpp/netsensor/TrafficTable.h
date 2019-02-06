#ifndef NETSENSOR_TrafficTable__INCLUDED
#define NETSENSOR_TrafficTable__INCLUDED
/*
* TrafficTable.h
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
* This is the main traffic container, each element is indexed by 
* interface name
*
*/

#include"StrClass.h"
#include"Mutex.h"
#include"TrafficElement.h"
#include"StringHashtable.h"
#include"MicroflowId.h"
#include"MicroflowNestedHashTable.h"
#include"traffic.pb.h"

namespace IHMC_NETSENSOR
{
class TrafficTable
{
public:
    void cleanTable (uint32 ui32CleaningNumber);
    void fillTrafficProtoObject (
        const char * pcIname, 
        const char * pszInterfaceAddr,
        netsensor::TrafficByInterface * pT);
    bool mutexTest(void);
    bool lockedPut (const char * interfaceName, const MicroflowId & microflowId);
    void printContent (void);
    TrafficTable (uint32 timeInterval);
private:
    TrafficElement * getNewTrafficElementIfNull(
        MicroflowNestedHashTable * pTrafficElementsContainer,
        const MicroflowId & microflowId);
    MicroflowNestedHashTable * getTrafficElementsContainer (const char * pszInterfaceName);
//<--------------------------------------------------------------------------->
private:
    NOMADSUtil::StringHashtable<MicroflowNestedHashTable> _trafficTablesContainer;
    NOMADSUtil::Mutex _pTMutex;
    uint32 _uint32msResolution;
};


inline void TrafficTable::fillTrafficProtoObject (
    const char * pszIname, 
    const char * pszInterfaceAddr,
    netsensor::TrafficByInterface * pT)
{
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        //printf("Interface name: %s\n", pcIname);
        auto pMFWT = _trafficTablesContainer.get (pszIname);
        if (pMFWT != nullptr) {
            pMFWT->fillTrafficProto (pT);
            pT->set_monitoringinterface (pszInterfaceAddr);
        }
        _pTMutex.unlock();
    }
}

inline bool TrafficTable::mutexTest()
{
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        _pTMutex.unlock();
        return true;
    }
    else {
        return false; 
    }
}

inline void TrafficTable::printContent(void)
{
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        for (NOMADSUtil::StringHashtable<MicroflowNestedHashTable>::Iterator i
            = _trafficTablesContainer.getAllElements();
            !i.end(); i.nextElement()) {
            auto pMNHT = i.getValue();
            printf ("%s : \n", i.getKey());
            pMNHT->print();
        }
        _pTMutex.unlock();
    }
}

inline TrafficTable::TrafficTable (uint32 timeInterval)
    : _trafficTablesContainer (true, true, true, true)
{
    _uint32msResolution = timeInterval;
}

}
#endif
