/*
 * SchedulerGeneratedMetadata.cpp
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc)
 * Created on November 21, 2011, 4:32 PM
 */

#include "SchedulerGeneratedMetadata.h"

#include "Defs.h"

using namespace NOMADSUtil;
using namespace IHMC_ACI;

SchedulerGeneratedMetadata *SchedulerGeneratedMetadata::_pInstance = nullptr;

SchedulerGeneratedMetadata::SchedulerGeneratedMetadata()
    : _m (MutexId::SchedulerGeneratedMetadata_m, LOG_MUTEX),
      _generatedMetadataIds (true,  // bCaseSensitiveKeys
                             true,  // bCloneKeys
                             true,  // bDeleteKeys
                             true,  // bCloneValues
                             true)  // bDeleteValues
{
}

SchedulerGeneratedMetadata::~SchedulerGeneratedMetadata (void)
{
}

const char * SchedulerGeneratedMetadata::remove (const char *pszGenMetadataID)
{
    _m.lock (1080);
    const char *pszBaseMetadataID = (const char *) _generatedMetadataIds.remove (pszGenMetadataID);
    _m.unlock (1080);
    return pszBaseMetadataID;
}

const char * SchedulerGeneratedMetadata::getBaseMetadataID (const char *pszGenMetadataID)
{
    _m.lock (1081);
    const char *pszBaseMetadataID = _generatedMetadataIds.get (pszGenMetadataID);
    _m.unlock (1081);
    return pszBaseMetadataID;
}

void SchedulerGeneratedMetadata::put (const char *pszGenMetadataID, const char *pszBaseMetadataID)
{
    _m.lock (1082);
    _generatedMetadataIds.put (pszGenMetadataID, (char *) pszBaseMetadataID);
    _m.unlock (1082);
}

SchedulerGeneratedMetadata * SchedulerGeneratedMetadata::getInstance (void)
{
    if (_pInstance == nullptr) {
        _pInstance = new SchedulerGeneratedMetadata();
    }
    return _pInstance;
}

