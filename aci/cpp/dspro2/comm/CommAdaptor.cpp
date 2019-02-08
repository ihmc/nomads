/*
 * CommAdaptor.cpp
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
 */

#include "CommAdaptor.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

CommAdaptor::CommAdaptor (unsigned int uiId, AdaptorType adaptorType, bool bSupportsCaching,
                          bool bSupportsDirectConnection, const char *pszNodeId,
                          CommAdaptorListener *pListener)
    : _pListener (pListener),
      _nodeId (pszNodeId),
      _adptorProperties (uiId, adaptorType, bSupportsCaching, bSupportsDirectConnection)
{
}

CommAdaptor::~CommAdaptor (void)
{
}

const char * CommAdaptor::getAdaptorAsString()
{
    switch (getAdaptorType()) {

        case DISSERVICE:
        {
            static const char *pszDisservice = "DISSERVICE";
            return pszDisservice;
        }

        case MOCKETS:
        {
            static const char *pszMockets = "MOCKETS";
            return pszMockets;
        }

        default:
        {
            static const char *pszUnknown = "UNKNOWN";
            return pszUnknown;
        }
    }
}

String CommAdaptor::getNodeId() const
{
    return _nodeId;
}

bool CommAdaptor::supportsDirectConnection (void)
{
    return _adptorProperties.bSupportsDirectConnection;
}

CommAdaptor::Subscription::Subscription (const char *pszGroupName)
    : ui8Priority ((uint8)0),
      bGroupReliable (false),
      bMsgReliable (false),
      bSequenced (false),
      groupName (pszGroupName)
{
}

CommAdaptor::Subscription::~Subscription (void)
{
}
