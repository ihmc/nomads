/*
 * DataCacheExpirationController.cpp
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

#include "DataCacheExpirationController.h"

#include "DataCacheInterface.h"
#include "MessageInfo.h"
#include "DisseminationService.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DataCacheExpirationController::DataCacheExpirationController (DisseminationService *pDisService)
    : DataCacheService (pDisService)
{
    _pDisService = pDisService;
    _pDataCacheInterface = pDisService->getDataCacheInterface();
}

DataCacheExpirationController::~DataCacheExpirationController()
{
}

void DataCacheExpirationController::setCapacityThreashold (uint32 ui32Capacity)
{
    _pDataCacheInterface->setSecurityRangeSize(ui32Capacity);
}

void DataCacheExpirationController::setCacheCleanCycle (uint16 ui16Cycle)
{
    _pDisService->_ui16CacheCleanCycle = ui16Cycle;
}

int DataCacheExpirationController::deleteMessage (const char * pszKey)
{
    return DataCacheService::deleteMessage (pszKey);
}

void DataCacheExpirationController::lockDataCache()
{
    _pDataCacheInterface->lock();
}

void DataCacheExpirationController::releaseDataCache()
{
    _pDataCacheInterface->unlock();
}

DArray2<String> * DataCacheExpirationController::getExpiredMessageIDs()
{
    return DataCacheService::getExpiredEntries();
}

