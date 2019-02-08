/*
 * DefaultDataCacheExpirationController.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "DefaultDataCacheExpirationController.h"

#include "DataCache.h"
#include "MessageInfo.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "TransmissionHistoryInterface.h"

#include "DataCacheReplicationController.h"

#include "StrClass.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DefaultDataCacheExpirationController::DefaultDataCacheExpirationController (DisseminationService *pDisService)
    : DataCacheExpirationController (pDisService)
{
    _pTransmissionHistory = TransmissionHistoryInterface::getTransmissionHistory();
    if (_pTransmissionHistory == NULL) {
        checkAndLogMsg ("DefaultDataCacheExpirationController::DefaultDataCacheExpirationController",
            Logger::L_Info, "failed to get Transmission History interface\n");
    }
}

DefaultDataCacheExpirationController::~DefaultDataCacheExpirationController()
{
}

void DefaultDataCacheExpirationController::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
}

void DefaultDataCacheExpirationController::capacityReached()
{
    // For now it just delete expired entries
    lockDataCache();
    DArray2<String> *pDArrayExpired = getExpiredMessageIDs();
    if(pDArrayExpired != NULL) {
        if(pDArrayExpired->size() != 0) {
            for (uint32 i = 0; i < pDArrayExpired->size(); i++) {
                deleteMessage((*pDArrayExpired)[i]);
            }
            delete pDArrayExpired;
            releaseDataCache();
            return;
        }
        delete pDArrayExpired;
    }
    releaseDataCache();
}

void DefaultDataCacheExpirationController::thresholdCapacityReached (uint32 ui32Length)
{
    // For now it just delete expired entries
    lockDataCache();
    DArray2<String> *pDArrayExpired = getExpiredMessageIDs();
    if(pDArrayExpired != NULL) {
        if(pDArrayExpired->size() != 0) {
            for (uint32 i = 0; i < pDArrayExpired->size(); i++) {
                deleteMessage ((*pDArrayExpired)[i]);
            }
            delete pDArrayExpired;
            releaseDataCache();
            return;
        }
        delete pDArrayExpired;
    }
    releaseDataCache();
}

void DefaultDataCacheExpirationController::spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo,
	                                                void *pIncomingData)
{
    // For now it just delete expired entries
    lockDataCache();
    DArray2<String> *pDArrayExpired = getExpiredMessageIDs();
    if(pDArrayExpired != NULL) {
        if(pDArrayExpired->size() != 0) {
            for(uint32 i = 0; i < pDArrayExpired->size(); i++) {
                deleteMessage ((*pDArrayExpired)[i]);
            }
            delete pDArrayExpired;
            releaseDataCache();
            return;
        }
        delete pDArrayExpired;
    }
    releaseDataCache();
}

int DefaultDataCacheExpirationController::cacheCleanCycle()
{
    // It deletes expired entries from the Data Cache and from the Transmission History
    lockDataCache();
    DArray2<String> *pDArrayExpired = getExpiredMessageIDs();
    if (pDArrayExpired != NULL) {
        if (pDArrayExpired->size() != 0) {
            int iTH = 0;
            for (uint32 i = 0; i < pDArrayExpired->size(); i++) {
                deleteMessage ((*pDArrayExpired)[i]);
                if (_pTransmissionHistory->hasMessage ((*pDArrayExpired)[i].c_str())) {
                    _pTransmissionHistory->deleteMessage ((*pDArrayExpired)[i].c_str());
                    iTH++;
                }
            }
            int iRet = pDArrayExpired->size();
            checkAndLogMsg ("DefaultDataCacheExpirationController::cacheCleanCycle", Logger::L_Info,
                            "deleted <%d> messages from Data Cache and <%d> from Transmission History\n", iRet, iTH);
            delete pDArrayExpired;
            releaseDataCache();
            return iRet;
        }
        delete pDArrayExpired;
    }
    releaseDataCache();
    return 0;
}
