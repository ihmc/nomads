/*
 * ProxyNetworkInterface.cpp
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

#include "ProxyNetworkInterface.h"

#include "Logger.h"
#if !defined (ANDROID)
    #include "ProxyDatagramSocket.h"
#endif
#include "StringTokenizer.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

ProxyNetworkInterface::ProxyNetworkInterface (PROPAGATION_MODE mode, bool bAsyncTransmission)
    : AbstractDatagramNetworkInterface (mode, bAsyncTransmission)
{
}

ProxyNetworkInterface::~ProxyNetworkInterface (void)
{
}

int ProxyNetworkInterface::bind (void)
{
    const char *pszMethodName = "ProxyNetworkInterface::bind";

    // This is an address for the ProxyDatagramSocket
    #if defined (ANDROID)
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "ProxyDatagramSocket not supported on Android\n");
        return -1;
    #else
    int rc;
    StringTokenizer st (((const char*) _bindingInterfaceSpec) + 6, ':');
    const char *pszProxyAddr = st.getNextToken ();
    const char *pszProxyPort = st.getNextToken ();
    if ((pszProxyAddr == NULL) || (pszProxyPort == NULL)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
            "proxy address and/or proxy port not specified for ProxyDatagramSocket address\n");
        return -2;
    }
    if (_pDatagramSocket == NULL) {
        ProxyDatagramSocket *pDatagramSocket = new ProxyDatagramSocket();
        if ((rc = pDatagramSocket->init (pszProxyAddr, (uint16) atoui32 (pszProxyPort), _ui16Port) < 0)) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                "init() for ProxyDatagramSocket failed with rc = %d\n", rc);
            return -3;
        }
        _pDatagramSocket = pDatagramSocket;
    }
    /*!!*/ // For now - assume all ProxyDatagramSockets are used for CSR
    // CSR address is stuffed into the last two octets - set netmask and broadcast address appropriately
    _netmask = "255.255.0.0";
    _broadcastAddr = "0.0.255.255";
#endif

    _bIsAvailable = true;
    return 0;
}