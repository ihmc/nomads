/*
 * Listener.cpp
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

#include "Listener.h"

using namespace IHMC_ACI;

DataCacheListener::DataCacheListener()
{
}

DataCacheListener::~DataCacheListener()
{
}

MessageListener::MessageListener()
{
}

MessageListener::~MessageListener()
{
}

GroupMembershipListener::GroupMembershipListener()
{
}

GroupMembershipListener::~GroupMembershipListener()
{
}

NetworkStateListener::NetworkStateListener()
{
}

NetworkStateListener::~NetworkStateListener()
{
}

PeerStateListener::PeerStateListener()
{
}

PeerStateListener::~PeerStateListener()
{
}

PeerStateListener::UInt16PeerStateUpdate::UInt16PeerStateUpdate (StateUpdate type, uint16 ui16OldState, uint16 ui16NewState)
    : PeerStateListener::PeerStateUpdate<uint16> (type, ui16OldState, ui16NewState)
{
}

PeerStateListener::UInt16PeerStateUpdate::~UInt16PeerStateUpdate()
{
}

PeerStateListener::UInt32PeerStateUpdate::UInt32PeerStateUpdate (StateUpdate type, uint32 ui16OldState, uint32 ui16NewState)
    : PeerStateListener::PeerStateUpdate<uint32> (type, ui16OldState, ui16NewState)
{
}

PeerStateListener::UInt32PeerStateUpdate::~UInt32PeerStateUpdate()
{
}

SearchListener::SearchListener (const char *pszDescription)
    : _description (pszDescription)
{
}

SearchListener::~SearchListener (void)
{
}

