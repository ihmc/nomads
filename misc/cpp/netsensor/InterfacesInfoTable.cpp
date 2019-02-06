/*
* InterfacesInfoTable.cpp
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
*
* This is a container for the InterfaceInfo objects
*/

#include "InterfacesInfoTable.h"
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
InterfacesInfoTable::InterfacesInfoTable (void)
{
    _interfacesInfoTable.configure (true, true, true, true);
}

InterfacesInfoTable::~InterfacesInfoTable (void)
{
}

void InterfacesInfoTable::put (InterfaceInfo * pII)
{
    if ((_pTMutex.lock() == Mutex::RC_Ok)) {
        auto pNewPII = _interfacesInfoTable.get (pII->sInterfaceName);
        if (pNewPII == nullptr) {
            pNewPII = pII;
            _interfacesInfoTable.put (pII->sInterfaceName, pII);
        }
        else {
            delete _interfacesInfoTable.remove (pII->sInterfaceName);
            _interfacesInfoTable.put (pII->sInterfaceName, pII);
        }
        _pTMutex.unlock();
    }
}

int InterfacesInfoTable::fillElementWithCopy (InterfaceInfo & interfaceInfo, const char * interfaceName)
{
    if ((_pTMutex.lock() == Mutex::RC_Ok)) {
        auto pII = _interfacesInfoTable.get (interfaceName);
        interfaceInfo.bIIface           = pII->bIIface;
        interfaceInfo.sInterfaceName    = pII->sInterfaceName;
        interfaceInfo.ui32IpAddr        = pII->ui32IpAddr;
        interfaceInfo.ui32Netmask       = pII->ui32Netmask;
        interfaceInfo.emacInterfaceMAC  = pII->emacInterfaceMAC;
        interfaceInfo.topologyDetectionMechanism               = pII->topologyDetectionMechanism;
        _pTMutex.unlock();
        return 0;
    }
    return -1;
}
}