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
void TrafficTable::cleanTable(uint32 ui32CleaningNumber)
{
    uint32 ui32CleaningCounter = 0;
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        if (_trafficTablesContainer.getCount() > 0) {
            for (NOMADSUtil::StringHashtable<MicroflowNestedHashTable>::
                    Iterator i = _trafficTablesContainer.getAllElements();
                    !i.end() && (ui32CleaningCounter < ui32CleaningNumber);
                    i.nextElement())
            {
                MicroflowNestedHashTable *mfnt = i.getValue();
                if (mfnt != NULL) {
                    ui32CleaningCounter += mfnt->cleanTable(ui32CleaningNumber - ui32CleaningCounter);
                }
            }
        }
        _pTMutex.unlock();
    }
}

bool TrafficTable::put(const char* interfaceName, const MicroflowId & microflowId)
{
    static uint32 ui32NewTrafficElementsCount = 0;
    MicroflowNestedHashTable *pTrafficElementsContainer;
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {       

        pTrafficElementsContainer = _trafficTablesContainer.get(interfaceName);

        if (pTrafficElementsContainer == NULL) {
            pTrafficElementsContainer = new MicroflowNestedHashTable();
            TrafficElement *pNewTrafficElement = new TrafficElement(_uint32msResolution);
            ++ui32NewTrafficElementsCount;
            pNewTrafficElement->classification = microflowId.classification;
            pNewTrafficElement->resolution = _uint32msResolution;
            pNewTrafficElement->i64TimeOfLastChange = microflowId.i64CurrTime;
            pNewTrafficElement->tiaPackets.add(1);
            pNewTrafficElement->tiaTraffic.add(microflowId.ui32Size);
            pTrafficElementsContainer->put(microflowId.sSA, microflowId.sDA, 
                                           microflowId.sProtocol, microflowId.sSP, microflowId.sDP, 
                                           pNewTrafficElement);

            _trafficTablesContainer.put(interfaceName, pTrafficElementsContainer);
        }
        else {
            TrafficElement *pTrafficElement = 
                pTrafficElementsContainer->get(microflowId.sSA, microflowId.sDA,
                                               microflowId.sProtocol, microflowId.sSP,
                                               microflowId.sDP);

            if (pTrafficElement == NULL) {
                pTrafficElement = new TrafficElement(_uint32msResolution);
                ++ui32NewTrafficElementsCount;
                pTrafficElement->classification = microflowId.classification;
                pTrafficElement->resolution = _uint32msResolution;
                pTrafficElement->i64TimeOfLastChange = microflowId.i64CurrTime;
                pTrafficElement->tiaPackets.getAverage();
                pTrafficElement->tiaPackets.add(1);
                pTrafficElement->tiaTraffic.getAverage();
                pTrafficElement->tiaTraffic.add(microflowId.ui32Size);
                pTrafficElementsContainer->put(microflowId.sSA, 
                                               microflowId.sDA, microflowId.sProtocol, microflowId.sSP, 
                                               microflowId.sDP, pTrafficElement);
            }
            else {
                pTrafficElement->i64TimeOfLastChange = microflowId.i64CurrTime;
                pTrafficElement->tiaPackets.getAverage();
                pTrafficElement->tiaPackets.add(1);
                pTrafficElement->tiaTraffic.getAverage();
                pTrafficElement->tiaTraffic.add(microflowId.ui32Size);
                //printf("Values: %d - %f\n", microflowId.ui32Size, pTrafficElement->tiaTraffic.getAverage());
            }
        }        
        _pTMutex.unlock();
        return true;
    }
    printf("Unable to get mutex\n");
    return false;
}
}