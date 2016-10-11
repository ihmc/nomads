/*
 * ManycastForwardingNetworkInterface.cpp
 *
 *  This file is part of the IHMC Network Message Service Library
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
 * Created on January 28, 2014, 6:43 PM
 */

#include "ManycastForwardingNetworkInterface.h"

#include "Logger.h"
#include "NetUtils.h"
#include "NetworkMessage.h"

using namespace NOMADSUtil;

uint8 ManycastForwardingNetworkInterface::MCAST_FWD_IF = 0x01;

ManycastForwardingNetworkInterface::ManycastForwardingNetworkInterface (NetworkInterface *pNetInt, uint32 ui32StorageDuration)
    : _unicastAddresses (ui32StorageDuration),
      _pNetInt (pNetInt)
{
}

ManycastForwardingNetworkInterface::~ManycastForwardingNetworkInterface()
{
}

bool ManycastForwardingNetworkInterface::addForwardingIpAddress (uint32 ui32IPv4Addr)
{
    char *pszIpAddr = NetUtils::ui32Inetoa (ui32IPv4Addr);
    bool rc = false;
    if (pszIpAddr != NULL) {
        rc = addForwardingIpAddress (pszIpAddr);
        free (pszIpAddr);
    }
    return rc;
}

bool ManycastForwardingNetworkInterface::addForwardingIpAddress (const char *pszIpAddr)
{
    if (pszIpAddr == NULL) {
        return false;
    }
    if (NetUtils::isMulticastAddress(pszIpAddr)) {
        return false;
    }
    _m.lock();
    bool rc = _unicastAddresses.put (pszIpAddr);
    _m.unlock();
    return rc;
}

int ManycastForwardingNetworkInterface::init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                      NetworkInterfaceManagerListener *pNMSParent,
                      bool bReceiveOnly , bool bSendOnly,
                      const char *pszPropagationAddr, uint8 ui8McastTTL)
{
    return _pNetInt->init (ui16Port, pszBindingInterfaceSpec, pNMSParent, bReceiveOnly , bSendOnly, pszPropagationAddr, ui8McastTTL);
}

int ManycastForwardingNetworkInterface::rebind (void)

{
    return _pNetInt->rebind();
}

int ManycastForwardingNetworkInterface::start (void)
{
    return _pNetInt->start();
}

int ManycastForwardingNetworkInterface::stop (void)
{
    return _pNetInt->stop();
}

uint8 ManycastForwardingNetworkInterface::getMode (void)
{
    return _pNetInt->getMode();
}

uint16 ManycastForwardingNetworkInterface::getMTU (void)
{
    return _pNetInt->getMTU();
}

const char * ManycastForwardingNetworkInterface::getBindingInterfaceSpec (void) const
{
    return _pNetInt->getBindingInterfaceSpec();
}

const char * ManycastForwardingNetworkInterface::getNetworkAddr (void)
{
    return _pNetInt->getNetworkAddr();
}

const char * ManycastForwardingNetworkInterface::getBroadcastAddr (void) const
{
    return _pNetInt->getBroadcastAddr();
}

const char * ManycastForwardingNetworkInterface::getNetmask (void) const
{
    return _pNetInt->getNetmask();
}

uint16 ManycastForwardingNetworkInterface::getPort (void)
{
    return _pNetInt->getPort();
}

const char * ManycastForwardingNetworkInterface::getPropagatonAddr (void)
{
    return _pNetInt->getPropagatonAddr();
}

NetworkMessageReceiver * ManycastForwardingNetworkInterface::getReceiver (void)
{
    return _pNetInt->getReceiver();
}

uint8 ManycastForwardingNetworkInterface::getTTL (void)
{
    return _pNetInt->getTTL();
}

void ManycastForwardingNetworkInterface::setDisconnected (void)
{
    return _pNetInt->setDisconnected();
}

bool ManycastForwardingNetworkInterface::isAvailable (void)
{
    return _pNetInt->isAvailable();
}

bool ManycastForwardingNetworkInterface::boundToWildcardAddr (void)
{
    return _pNetInt->boundToWildcardAddr();
}

int ManycastForwardingNetworkInterface::getReceiveBufferSize (void)
{
    return _pNetInt->getReceiveBufferSize();
}

int ManycastForwardingNetworkInterface::setReceiveBufferSize (int iBufSize)
{
    return _pNetInt->setReceiveBufferSize (iBufSize);
}

uint8 ManycastForwardingNetworkInterface::getType (void)
{
    return MCAST_FWD_IF;
}

uint32 ManycastForwardingNetworkInterface::getTransmitRateLimit (const char *pszDestinationAddr)
{
    return _pNetInt->getTransmitRateLimit (pszDestinationAddr);
}

uint32 ManycastForwardingNetworkInterface::getTransmitRateLimit (void)
{
    return _pNetInt->getTransmitRateLimit();
}

int ManycastForwardingNetworkInterface::setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit)
{
    return _pNetInt->setTransmitRateLimit (pszDestinationAddr, ui32RateLimit);
}

int ManycastForwardingNetworkInterface::setTransmitRateLimit (uint32 ui32RateLimit)
{
    return _pNetInt->setTransmitRateLimit (ui32RateLimit);
}

void ManycastForwardingNetworkInterface::setReceiveRateSampleInterval (uint32 ui32IntervalInMS)
{
    _pNetInt->setReceiveRateSampleInterval (ui32IntervalInMS);
}


uint32 ManycastForwardingNetworkInterface::getReceiveRate (void)
{
    return _pNetInt->getReceiveRate();
}

uint32 ManycastForwardingNetworkInterface::getTransmissionQueueSize (void)
{
    return _pNetInt->getTransmissionQueueSize();
}

uint8 ManycastForwardingNetworkInterface::getRescaledTransmissionQueueSize (void)
{
    return _pNetInt->getRescaledTransmissionQueueSize();
}

void ManycastForwardingNetworkInterface::setTransmissionQueueMaxSize (uint32 ui32MaxSize)
{
    _pNetInt->setTransmissionQueueMaxSize (ui32MaxSize);
}

uint32 ManycastForwardingNetworkInterface::getTransmissionQueueMaxSize (void)
{
    return _pNetInt->getTransmissionQueueMaxSize();
}

void ManycastForwardingNetworkInterface::setLinkCapacity (uint32 ui32Capacity)
{
    _pNetInt->setLinkCapacity (ui32Capacity);
}

uint32 ManycastForwardingNetworkInterface::getLinkCapacity (void)
{
    return _pNetInt->getLinkCapacity();
}

void ManycastForwardingNetworkInterface::setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue)
{
    _pNetInt->setAutoResizeQueue (bEnable, ui32MaxTimeInQueue);
}

uint32 ManycastForwardingNetworkInterface::getAutoResizeQueue (void)           
{
    return _pNetInt->getAutoResizeQueue();
}

int ManycastForwardingNetworkInterface::receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr, InetAddr *pRemoteAddr)
{
    return _pNetInt->receive (pBuf, iBufSize, pIncomingIfaceByAddr, pRemoteAddr);
}

int ManycastForwardingNetworkInterface::sendMessage (const NetworkMessage *pNetMsg, bool bExpedited, const char *pszHints)           
{
    return _pNetInt->sendMessage (pNetMsg, bExpedited, pszHints);
}

//int sendMessage (const NetworkMessage *pNetMsg, const char *pszIPAddr, bool bExpedited = false, const char *pszHints = NULL);
int ManycastForwardingNetworkInterface::sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited, const char *pszHints)
{
    int rc = _pNetInt->sendMessage (pNetMsg, ui32IPAddr, bExpedited, pszHints);

    if ((!NetUtils::isMulticastAddress (pNetMsg->getDestinationAddr()))) {
        return rc;
    }
    const InetAddr bcastAddr (_pNetInt->getBroadcastAddr());
    if (bcastAddr.getIPAddress() == ui32IPAddr) {
        return rc;
    }

    _m.lock();
    if (_unicastAddresses.getCount() > 0) {
        TimeBoundedStringHashset::Iterator iter = _unicastAddresses.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            const char *pszIPAddr = iter.getKey();
            if (pszIPAddr != NULL) {
                if (_pNetInt->sendMessage (pNetMsg, inet_addr (pszIPAddr), false, pszHints) != 0) {
                    if (pLogger != NULL) pLogger->logMsg ("ManycastForwardingNetworkInterface::sendMessage",
                            Logger::L_Warning, "could not unicast message to %s\n", pszIPAddr);
                    rc = -2;
                }
            }
        }
    }
    _m.unlock();
    return rc;
}

int ManycastForwardingNetworkInterface::sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints)
{
    int rc = _pNetInt->sendMessageNoBuffering (pNetMsg, ui32IPAddr, pszHints);

    if ((!NetUtils::isMulticastAddress (pNetMsg->getDestinationAddr ()))) {
        return rc;
    }
    const InetAddr bcastAddr (_pNetInt->getBroadcastAddr ());
    if (bcastAddr.getIPAddress() == ui32IPAddr) {
        return rc;
    }

    _m.lock ();
    if (_unicastAddresses.getCount() > 0) {
        TimeBoundedStringHashset::Iterator iter = _unicastAddresses.getAllElements ();
        for (; !iter.end (); iter.nextElement ()) {
            const char *pszIPAddr = iter.getKey ();
            if (pszIPAddr != NULL) {
                if (_pNetInt->sendMessage (pNetMsg, inet_addr (pszIPAddr), false, pszHints) != 0) {
                    if (pLogger != NULL) pLogger->logMsg ("ManycastForwardingNetworkInterface::sendMessageNoBuffering",
                        Logger::L_Warning, "could not unicast message to %s\n", pszIPAddr);
                    rc = -2;
                }
            }
        }
    }
    _m.unlock ();
    return rc;
}

bool ManycastForwardingNetworkInterface::canSend (void)
{
    return _pNetInt->canSend();
}

bool ManycastForwardingNetworkInterface::canReceive (void)
{
    return _pNetInt->canReceive();
}

bool ManycastForwardingNetworkInterface::clearToSend (void)
{
    return _pNetInt->clearToSend();
}

bool ManycastForwardingNetworkInterface::operator == (const NetworkInterface &rhsStr) const
{
    return _pNetInt->operator == (rhsStr);
}

