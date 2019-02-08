/*
 * NetworkMessageReceiver.cpp
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
 */

#include "NetworkMessageReceiver.h"
#include "MessageFactory.h"
#include "MulticastUDPDatagramSocket.h"
#include "NetworkInterfaceManager.h"
#include "NetUtils.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

NetworkMessageReceiver::NetworkMessageReceiver (NetworkInterfaceManagerListener *pNMSParent, NetworkInterface *pNetIface,
												bool bReceive)
    : _bReceive (bReceive), _pNMSParent (pNMSParent), _pNetIface (pNetIface)
{
    _bEstimateRate = false;
    _pRateEstimator = new RateEstimator();
    _type = NET_RCV;
}

NetworkMessageReceiver::NetworkMessageReceiver (NetworkInterfaceManagerListener *pNMSParent, NetworkInterface *pNetIface,
												bool bReceive, RateEstimator *pRateEstimator, Type type)
    : _bReceive (bReceive), _pNMSParent (pNMSParent), _pNetIface (pNetIface)
{
    _bEstimateRate = false;
    _pRateEstimator = pRateEstimator;
    _type = type;
}

NetworkMessageReceiver::~NetworkMessageReceiver (void)
{
    if (isRunning()) {
        requestTermination();
    }
    delete _pRateEstimator;
    _pRateEstimator = NULL;
    _pNMSParent = NULL;
}

void NetworkMessageReceiver::run (void)
{
    started();
    char achBuf [MAX_MESSAGE_SIZE];
    while (!terminationRequested()) {
        if (_pNetIface->rebind() != 0) {
            sleepForMilliseconds (15000);
        }
        else if (_bReceive) {
            receive (achBuf, MAX_MESSAGE_SIZE);
        }
    }
    terminating();
}

int NetworkMessageReceiver::receive (char *achBuf, int iBufLen)
{
    if (achBuf == NULL || iBufLen <= 0) {
        return -1;
    }
    const char *pszMethodName = "NetworkMessageReceiver::receive";
    InetAddr remoteAddr;
    InetAddr incomingIfaceByAddr;
    // Max returned value should be about 65,000 (udp mtu), but we can have negative values
    int32 rc = _pNetIface->receive (achBuf, iBufLen, &incomingIfaceByAddr, &remoteAddr);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "error receiving packets (_pmcSocket.receive (%s) returned negative value: %d)\n", _pNetIface->getBindingInterfaceSpec(), rc);
    }
    else if ((rc > 0) && (remoteAddr.getIPAddress() != incomingIfaceByAddr.getIPAddress())) {
        if (incomingIfaceByAddr.getIPAddress() == INADDR_ANY) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "incomingIfaceByAddr was not correctly set\n");
        }

        // De-serialize
        NetworkMessage *pNetMsg = MessageFactory::createNetworkMessageFromBuffer (achBuf, rc, NetworkMessage::getVersionFromBuffer (achBuf, rc));
        if ((pNetMsg != NULL) && (!pNetMsg->isMalformed())) {
            pNetMsg->display(); // debug
            if (!NetUtils::isLocalAddress (remoteAddr.getIPAddress(), NULL) &&
                !NetUtils::isLocalAddress (pNetMsg->getSourceAddr(), NULL)) {
                if (_bEstimateRate && _pRateEstimator != NULL) {
                    _pRateEstimator->update (remoteAddr.getIPAddress(), static_cast<uint32>(rc));
                }
                pNetMsg->incrementHopCount();
                InetAddr dst (pNetMsg->getDestinationAddr());
                _pNMSParent->messageArrived (pNetMsg, incomingIfaceByAddr.getIPAsString(), remoteAddr.getIPAddress());
            }
            else {
                delete pNetMsg;
                pNetMsg = NULL;
            }
            // NOTE: if pNetMsg is passed to NetworkMessageService::messageArrived
            // then pNetMsg does NOT have to be deallocated. (See note
            // in NetworkMessageService::messageArrived's prototype for
            // more details).
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "invalid message received - ignoring\n");
            if (pNetMsg) {
                delete pNetMsg;
                pNetMsg = NULL;
            }
        }
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                       "ignoring my own broadcast\n");
    }
    return 0;
}

NetworkMessageReceiver::RateEstimator::RateEstimator (void)
{
    reset();
}

NetworkMessageReceiver::RateEstimator::~RateEstimator (void)
{
}

void NetworkMessageReceiver::RateEstimator::reset (void)
{
    _ui32RateInterval = 0;
    _i64IntervalStartTime = 0;
    _ui32BytesReceived = 0;
    _ui32ComputedRate = 0;
}

void NetworkMessageReceiver::RateEstimator::setInterval (uint32 ui32IntervalInMS)
{
    reset();
    _ui32RateInterval = ui32IntervalInMS;
}

void NetworkMessageReceiver::RateEstimator::update (uint32, uint32 ui32BytesReceived)
{
    if (_ui32RateInterval != 0) {
        int64 i64CurrTime = getTimeInMilliseconds();
        if (_i64IntervalStartTime == 0) {
            _i64IntervalStartTime = i64CurrTime;
            _ui32BytesReceived = ui32BytesReceived;
        }
        else {
            uint32 ui32ElapsedTime = (uint32)(i64CurrTime - _i64IntervalStartTime);
            if (ui32ElapsedTime < _ui32RateInterval) {
                // Still in the same interval - accumulate the number of bytes
                _ui32BytesReceived += ui32BytesReceived;
            }
            else if (ui32ElapsedTime > (_ui32RateInterval * 2)) {
                // Skipped an interval - reset
                _ui32ComputedRate = 0;
                _i64IntervalStartTime = i64CurrTime;
                _ui32BytesReceived = ui32BytesReceived;
            }
            else {
                // In the next interval
                _ui32ComputedRate = (_ui32BytesReceived * 1000) / _ui32RateInterval;
                _i64IntervalStartTime += _ui32RateInterval;
                _ui32BytesReceived = ui32BytesReceived;
                checkAndLogMsg ("NetworkMessageReceiver::RateEstimator::update", Logger::L_LowDetailDebug,
                                "computed receive rate to be %lu during the last interval\n", _ui32ComputedRate);
            }
        }
    }
}

uint32 NetworkMessageReceiver::RateEstimator::getRate (void)
{
    return _ui32ComputedRate;
}

