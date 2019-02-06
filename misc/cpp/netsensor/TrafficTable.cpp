/*
* TrafficTable.cpp
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
#include"TrafficTable.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

namespace IHMC_NETSENSOR
{
void TrafficTable::cleanTable (uint32 ui32CleaningNumber)
{
    auto pszMethodName = "TrafficTable::cleanTable";
    uint32 ui32CleaningCounter = 0;
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        if (_trafficTablesContainer.getCount() > 0) {
            for (auto i = _trafficTablesContainer.getAllElements(); !i.end() && (ui32CleaningCounter < ui32CleaningNumber); i.nextElement()) {
                auto pValue = i.getValue();
                if (pValue != NULL) {
                    ui32CleaningCounter += pValue->cleanTable2 (ui32CleaningNumber - ui32CleaningCounter);
                }
            }
        }
        _pTMutex.unlock();
    }
}

TrafficElement* TrafficTable::getNewTrafficElementIfNull (
    MicroflowNestedHashTable* pTrafficElementsContainer,
    const MicroflowId& microflowId)
{
	NOMADSUtil::String id = microflowId.sSA + ":" + microflowId.sDA + ":" + microflowId.sProtocol + ":" + microflowId.sSP + ":" + microflowId.sDP;
    auto pTrafficElement = pTrafficElementsContainer->get (id);
    if (pTrafficElement == nullptr) {
        pTrafficElement = new TrafficElement (_uint32msResolution);
        pTrafficElement->classification = microflowId.classification;
		pTrafficElement->srcAddr		= microflowId.sSA;
		pTrafficElement->dstAddr		= microflowId.sDA;
		pTrafficElement->protocol		= microflowId.sProtocol;
		pTrafficElement->srcPort		= microflowId.sSP;
		pTrafficElement->dstPort		= microflowId.sDP;
        pTrafficElementsContainer->put (id, pTrafficElement);
    }
    return pTrafficElement;
}

MicroflowNestedHashTable * TrafficTable::getTrafficElementsContainer (const char * pszInterfaceName)
{
    auto pTrafficElementsContainer = _trafficTablesContainer.get (pszInterfaceName);
    if (pTrafficElementsContainer == NULL) {
        pTrafficElementsContainer = new MicroflowNestedHashTable();
        _trafficTablesContainer.put (pszInterfaceName, pTrafficElementsContainer);
    }
    return pTrafficElementsContainer;
}

bool TrafficTable::lockedPut (const char * interfaceName, const MicroflowId & microflowId)
{
    auto pszMethodName = "TrafficTable::lockedPut";
    if ((_pTMutex.lock() != NOMADSUtil::Mutex::RC_Ok)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Unable to get mutex\n");
        return false;
    }

    auto pTrafficElementsContainer = getTrafficElementsContainer (interfaceName);
    auto pTrafficElement = getNewTrafficElementIfNull (pTrafficElementsContainer, microflowId);
    pTrafficElement->i64TimeOfLastChange = microflowId.i64CurrTime;
    pTrafficElement->tiaPackets.add (1);
    pTrafficElement->tiaTraffic.getAverage();
    pTrafficElement->tiaTraffic.add (microflowId.ui32Size);
    _pTMutex.unlock();
    return true;
}
}