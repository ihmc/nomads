/* 
 * StorageController.cpp
 *
 * This class should be extended by external components,
 * registered through the proper API, that need to access
 * the _pDataStore and the _pInfoStore.
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 4, 2013, 11:25 PM
 */

#include "StorageController.h"

#include "InformationStore.h"

using namespace IHMC_ACI;

StorageController::StorageController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore)
    : _pDSPro (pDSPro),
      _pDataStore (pDataStore),
      _pInfoStore (pInfoStore)
{
}

StorageController::~StorageController (void)
{
}

DSProImpl * StorageController::getDSPro (void) const
{
    return _pDSPro;
}

DataStore * StorageController::getDataStore (void)
{
    return _pDataStore;
}

InformationStore * StorageController::getInformationStore (void)
{
    return _pInfoStore;
}

const char * StorageController::getMetadataTableName (void)
{
    return _pInfoStore->_pszMetadataTableName;
}

const char * StorageController::getMetadataTableAllColumns (void)
{
    return _pInfoStore->_pszMetadataAll;
}
