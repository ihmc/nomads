/*
 * NormNetworkInterface.cpp
 *
 * This file is part of the IHMC Util Library
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
 */

#include "NormNetworkInterface.h"

#include "NetworkInterfaceManager.h"
#include "NetworkMessage.h"
#include "Nocket.h"

#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace IHMC_MISC;

NormNetworkInterface::NormNetworkInterface (void)
    : AbstractNetworkInterface (false)
{
}

NormNetworkInterface::~NormNetworkInterface (void)
{
}

int NormNetworkInterface::init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                                NetworkInterfaceManagerListener *pNMSParent,
                                bool bReceiveOnly, bool bSendOnly,
                                const char *pszPropagationAddr, uint8 ui8McastTTL)
{
    if (AbstractNetworkInterface::init (ui16Port, pszBindingInterfaceSpec, pNMSParent, bReceiveOnly,
                                        bSendOnly, pszPropagationAddr, ui8McastTTL) < 0) {
        return -1;
    }
    _pNock = new Nocket (!bSendOnly, !bReceiveOnly);
    if (_pNock == NULL) {
        return -2;
    }
    if (_pNock->init (ui16Port, pszPropagationAddr) < 0) {
        return -3;
    }
    _pNock->setTTL (ui8McastTTL);
    return 0;
}

int NormNetworkInterface::rebind (void)
{
    return 0;
}

uint8 NormNetworkInterface::getMode (void)
{
   return NORM; 
}

uint16 NormNetworkInterface::getMTU (void)
{
    return 0xFFFF;
}

const char * NormNetworkInterface::getNetworkAddr (void)
{
    return _bindingInterfaceSpec;
}

void NormNetworkInterface::setDisconnected (void)
{
    // TODO: implement this
}

bool NormNetworkInterface::isAvailable (void)
{
    return (_pNock != NULL);
}

int NormNetworkInterface::getReceiveBufferSize (void)
{
    return _pNock->getReceiveBufferSize();
}

int NormNetworkInterface::setReceiveBufferSize (int iBufSize)
{
    return _pNock->setReceiveBufferSize (iBufSize);
}

uint8 NormNetworkInterface::getType (void)
{
    // TODO: fix this
    return 0;
}

uint32 NormNetworkInterface::getTransmitRateLimit (const char *pszDestinationAddr)
{
    return 0U;
}

uint32 NormNetworkInterface::getTransmitRateLimit (void)
{
    return 0U;
}

int NormNetworkInterface::setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit)
{
    return -1;
}

int NormNetworkInterface::setTransmitRateLimit (uint32 ui32RateLimit)
{
    return -1;
}

int NormNetworkInterface::setTTL (uint8 ui8TTL)
{
    _ui8McastTTL = ui8TTL;
    return _pNock->setTTL (ui8TTL);
}

int NormNetworkInterface::receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr, InetAddr *pRemoteAddr)
{
    if (pIncomingIfaceByAddr != NULL) {
        *pIncomingIfaceByAddr = _pNock->getLocalAddr();
    }
    return _pNock->receive (pBuf, iBufSize, pRemoteAddr);
}

int NormNetworkInterface::sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *)
{
    const char * const pszMethodName = "NormNetworkInterface::sendMessageNoBuffering";

    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                    "sending to NORM message of length %u (%u  of metadata)\n",
                    pNetMsg->getMsgLen(), pNetMsg->getMetaDataLen());

    if (pNetMsg == NULL) {
        return -1;
    }

    // Make sure the target address is set to the right value
    NetworkMessage *pModifiedNetMsg = (NetworkMessage*) pNetMsg;
    pModifiedNetMsg->setTargetAddress (ui32IPAddr);

    if (_pNock == NULL) {
        // The socket is not connected - the message can't be sent
        return -2;
    }

    // Send the message
    int rc = _pNock->sendTo (ui32IPAddr, _ui16Port, pModifiedNetMsg->getBuf(), pModifiedNetMsg->getLength());
    if (rc > 0) {
        _pNMSParent->messageSent (pNetMsg, _bindingInterfaceSpec);
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "packet sent %lu:%d\n", pNetMsg->getDestinationAddr(),
                        (int) _ui16Port);
    }

    return rc;
}

bool NormNetworkInterface::clearToSend (void)
{
    return (_pNock != NULL);
}

