/*
 * MetaDataRankerConfiguration.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on July 5, 2013, 12:39 PM
 */

#include "MetadataRankerLocalConfiguration.h"

#include "MetadataInterface.h"
#include "Logger.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

MetadataRankerLocalConfiguration::MetadataRankerLocalConfiguration (const char *pszNodeId)
    : _bInstrumented (true),
      _nodeId (pszNodeId),
      _rangeOfInfluenceKeyAttributeName (MetadataInterface::DESCRIPTION)
{
}

MetadataRankerLocalConfiguration::~MetadataRankerLocalConfiguration (void)
{
}

void MetadataRankerLocalConfiguration::addFilter (const char *pszDataType, const char *pszPeerNodeId)
{
    if (pszDataType == NULL) {
        return;
    }
    if (pszPeerNodeId == NULL) {
        _dataTypesToFilter.put (pszDataType);
    }
    _dataTypesNotToMatchByNodeId.put (pszPeerNodeId, pszDataType);
}

bool MetadataRankerLocalConfiguration::hasFilterForTypeAndPeer (const char *pszNodeId, const char *pszDataType)
{
    if (pszDataType == NULL) {
        return false;
    }
    if (_dataTypesToFilter.containsKey (pszDataType)) {
        return true;
    }
    if (pszNodeId == NULL) {
        return false;
    }
    return _dataTypesNotToMatchByNodeId.hasKeyValue (pszNodeId, pszDataType);
}

String MetadataRankerLocalConfiguration::getRangeOfInfluenceAttributeName (void)
{
    return _rangeOfInfluenceKeyAttributeName;
}

void MetadataRankerLocalConfiguration::setRangeOfInfluenceAttributeName (const String &rangeOfInfluenceKeyAttributeName)
{
    _rangeOfInfluenceKeyAttributeName = rangeOfInfluenceKeyAttributeName;
}

//-----------------------------------------------------------------------------

const float MetadataRankerLocalConfiguration::TrackVoIConf::DEFAULT_INSIGNIFICANT_MOVEMENT_PERC = 0.1f;

MetadataRankerLocalConfiguration::TrackVoIConf::TrackVoIConf (void)
    : _enable (true), _fInsignificantMovementPerc (DEFAULT_INSIGNIFICANT_MOVEMENT_PERC)
{
}

MetadataRankerLocalConfiguration::TrackVoIConf::~TrackVoIConf (void)
{
}

bool MetadataRankerLocalConfiguration::TrackVoIConf::enabled (void)
{
    return _enable;
}

float MetadataRankerLocalConfiguration::TrackVoIConf::getInsignificantTrackMovementFactor (void) const
{
    return _fInsignificantMovementPerc;
}

void MetadataRankerLocalConfiguration::TrackVoIConf::setInsignificantTrackMovementFactor (float fPerc)
{
    if ((fPerc < 0.0f) || (fPerc > 1.0f)) {
        return;
    }
    _fInsignificantMovementPerc = fPerc;
}

//-----------------------------------------------------------------------------

const float MetadataRankerLocalConfiguration::LogStatVoIConf::DEFAULT_INSIGNIFICANT_UPDATE_PERC = 0.1f;

MetadataRankerLocalConfiguration::LogStatVoIConf::LogStatVoIConf (void)
    : _enable (true), _fInsignificantUpdatePerc (DEFAULT_INSIGNIFICANT_UPDATE_PERC)
{
}

MetadataRankerLocalConfiguration::LogStatVoIConf::~LogStatVoIConf (void)
{
}

bool MetadataRankerLocalConfiguration::LogStatVoIConf::enabled (void)
{
    return _enable;
}

float MetadataRankerLocalConfiguration::LogStatVoIConf::getInsignificantUpdatePerc (void) const
{
    return _fInsignificantUpdatePerc;
}

void MetadataRankerLocalConfiguration::LogStatVoIConf::setInsignificantUpdatePerc (float fPerc)
{
    if ((fPerc < 0.0f) || (fPerc > 1.0f)) {
        return;
    }
    _fInsignificantUpdatePerc = fPerc;
}

