/*
 * LocalNetworkInterface.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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
 */

#include "LocalNetworkInterface.h"

#include "Logger.h"
#include "MulticastUDPDatagramSocket.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

LocalNetworkInterface::LocalNetworkInterface (PROPAGATION_MODE mode, bool bAsyncTransmission)
    : AbstractDatagramNetworkInterface (mode, bAsyncTransmission)
{
}

LocalNetworkInterface::~LocalNetworkInterface (void)
{
}

int LocalNetworkInterface::rejoinMulticastGroup (void)
{
    if ((_pDatagramSocket != NULL) && (_mode == MULTICAST)) {
        return postBind (static_cast<MulticastUDPDatagramSocket *>(_pDatagramSocket));
    }
    return 0;
}

int LocalNetworkInterface::preBind (void)
{
    NICInfo *pNICInfo = NetUtils::getNICInfo (_bindingInterfaceSpec.c_str());
    if (pNICInfo == NULL) {
        return -1;
    }
    _networkAddr = pNICInfo->getIPAddrAsString();
    _broadcastAddr = pNICInfo->getBroadcastAddrAsString();
    _netmask = pNICInfo->getNetmaskAsString();
    delete pNICInfo;

    return 0;
}

int LocalNetworkInterface::postBind (MulticastUDPDatagramSocket *pDatagramSocket)
{
    const char *pszMethodName = "LocalNetworkInterface::postBind";
    if (_mode == MULTICAST) {
        if (!_bSendOnly) {
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                            "interface %s joining group %s\n",
                            _bindingInterfaceSpec.c_str(), _defaultPropagationAddr.c_str());
            pDatagramSocket->joinGroup (inet_addr (_defaultPropagationAddr), inet_addr (_bindingInterfaceSpec.c_str()));
        }
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "interface %s setting TTL to %d\n",
                        _bindingInterfaceSpec.c_str(), static_cast<int>(_ui8McastTTL));
                        pDatagramSocket->setTTL (_ui8McastTTL);
    }
    return 0;
}

int LocalNetworkInterface::bind (void)
{
    const char *pszMethodName = "LocalNetworkInterface::bind";
    if (preBind () < 0) {
        return -1;
    }

    const uint32 ui32ListenAddr = inet_addr (_bindingInterfaceSpec.c_str());
    MulticastUDPDatagramSocket *pDatagramSocket = (_pDatagramSocket == NULL ?
    #if defined UNIX
        new MulticastUDPDatagramSocket (ui32ListenAddr == INADDR_ANY) :
    #else
        new MulticastUDPDatagramSocket() :
    #endif
    static_cast<MulticastUDPDatagramSocket *>(_pDatagramSocket));
    if (pDatagramSocket->init (_ui16Port, ui32ListenAddr) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
            "UDPDatagramSocket init failed\n");
        return -4;
    }

    if (postBind (pDatagramSocket) < 0) {
        return -2;
    }

    _pDatagramSocket = pDatagramSocket;
    _bIsAvailable = true;
    return 0;
}

WildcardNetworkInterface::WildcardNetworkInterface (PROPAGATION_MODE mode, bool bAsyncTransmission)
    : LocalNetworkInterface (mode, bAsyncTransmission)
{
}

WildcardNetworkInterface::~WildcardNetworkInterface (void)
{
}

int WildcardNetworkInterface::preBind (void)
{
    _networkAddr = IN_ADDR_ANY_STR;
    _broadcastAddr = IN_ADDR_ANY_STR;
    _netmask = "255.255.255.255";
    return 0;
}

int WildcardNetworkInterface::postBind (void)
{
    return 0;
}

