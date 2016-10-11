/* 
 * MocketsConnHandler.cpp
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

#include "MocketConnHandler.h"

#include "CommAdaptorListener.h"
#include "Mocket.h"
#include "TagGenerator.h"

#include "Logger.h"
#include "NetUtils.h"

#define WAIT_INDEFINITELY -1

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    String ui32InetoaAsString (uint32 ui32Addr)
    {
        char *pszTmp = NetUtils::ui32Inetoa (ui32Addr);
        if (pszTmp == NULL) {
            String empty;
            return empty;
        }
        else {
            String addr (pszTmp);
            free (pszTmp);
            return addr;
        }
    }
}

MocketConnHandler::MocketConnHandler (const AdaptorProperties &adptProp, const char *pszRemotePeerId,
                                      CommAdaptorListener *pListener, Mocket *pMocket)
    : ConnHandler (adptProp, pszRemotePeerId, pMocket->getRemotePort(), pListener),
      _bReliableTransmission (true),
      _bSequencedTransmission (true),
      _bActiveRemotePeer (true),
      _ui32EnqueueTimeout (0),
      _ui32RetryTimeout (0),
      _ui32UncreachablePeerTimeout (30000U),
      _pMocket (pMocket),
      _localPeerAddr (ui32InetoaAsString (pMocket->getLocalAddress())),
      _remotePeerAddr (ui32InetoaAsString (pMocket->getRemoteAddress()))
{
    assert (InetAddr::isIPv4Addr (_localPeerAddr));
    assert (InetAddr::isIPv4Addr (_remotePeerAddr));
}

MocketConnHandler::~MocketConnHandler()
{
    _m.lock();
    if (_pMocket->isConnected()) {
        _pMocket->close();
    }
    delete _pMocket;
    _pMocket = NULL;
    _m.unlock();
}

int MocketConnHandler::init (void)
{
    String sThreadName ("IHMC_ACI::MocketConnHandler_");
    sThreadName += getRemotePeerNodeId();
    setName (sThreadName.c_str());

    _m.lock();
    _pMocket->registerPeerUnreachableWarningCallback (&callDeadPeer, this);
    _pMocket->registerPeerReachableCallback (&callNewPeer, this);
    _m.unlock();

    return 0;
}

bool MocketConnHandler::isConnected (void)
{
    if (_pMocket != NULL) {
        return _pMocket->isConnected();
    }
    return false;
}

const char * MocketConnHandler::getLocalPeerAddress (void) const
{
    return _localPeerAddr.c_str();
}

const char * MocketConnHandler::getRemotePeerAddress (void) const
{
    return _remotePeerAddr.c_str();
}

void MocketConnHandler::resetTransmissionCounters (void)
{
    if (_pMocket == NULL) {
        return;
    }
    _pMocket->resetTransmissionCounters();
}

int MocketConnHandler::send (const void *pBuf, uint32 ui32BufSize, uint8 ui8Priority)
{
    if (pBuf == NULL || ui32BufSize == 0) {
        return -1;
    }

    return send (true, // bReliableTransmission
                 true, // bSequencedTransmission
                 pBuf, ui32BufSize, TagGenerator::getDefaultTag(), ui8Priority);
}

int MocketConnHandler::sendDataMessage (const void *pBuf, uint32 ui32BufSize,
                                        uint8 ui8Priority)
{
    if (pBuf == NULL || ui32BufSize == 0) {
        return -1;
    }

    return send (true,  // reliable transmission
                 false, // sequenced transmission
                 pBuf, ui32BufSize, TagGenerator::getDefaultTag(), ui8Priority);
}

int MocketConnHandler::sendVersionMessage (const void *pBuf, uint32 ui32BufSize,
                                           uint8 ui8Priority)
{
    if (pBuf == NULL || ui32BufSize == 0) {
        return -1;
    }

    return send (true, // reliable transmission
                 true, // sequenced transmission
                 pBuf, ui32BufSize, TagGenerator::getVersionTag(), ui8Priority);
}

int MocketConnHandler::sendWaypointMessage (const void *pBuf, uint32 ui32BufSize,
                                            uint8 ui8Priority, const char *pszPublisherNodeId)
{
    if (pBuf == NULL || ui32BufSize == 0 || pszPublisherNodeId == NULL) {
        return -1;
    }

    return cancelPreviousAndSend (false, // reliable transmission
                                  true,  // sequenced transmission
                                  pBuf, ui32BufSize, TagGenerator::getInstance()->getWaypointTagForPeer (pszPublisherNodeId),
                                  ui8Priority);
}

int MocketConnHandler::receive (BufferWrapper &bw, char **ppszRemotePeerAddr)
{
    if (ppszRemotePeerAddr == NULL || _pMocket == NULL) {
        return -1;
    }

    int iSize = _pMocket->getNextMessageSize (WAIT_INDEFINITELY);
    if (iSize <= 0) {
        return -2;
    }
    else if (iSize > MAX_MESSAGE_SIZE) {
        void *pBuf = NULL;
        int rc = _pMocket->receive (&pBuf);
        assert (rc == iSize);
        if (rc < 0) {
            return -3;
        }
        bw.init (pBuf, rc, true);
    }
    else {
        int rc = _pMocket->receive (_buf, MAX_MESSAGE_SIZE);
        assert (rc == iSize);
        if (rc < 0) {
            return -4;
        }
        bw.init (_buf, rc, false);
    }

    uint32 ui32Ip4Addr = _pMocket->getRemoteAddress();
    *ppszRemotePeerAddr = NetUtils::ui32Inetoa (ui32Ip4Addr);

    return bw.getBufferLength();
}

int MocketConnHandler::cancelPreviousAndSend (bool bReliableTransmission, bool bSequencedTransmission,
                                              const void *pBuf, uint32 ui32BufSize,
                                              uint16 ui16Tag, uint8 ui8Priority)
{
    _m.lock();
    if (_pMocket == NULL || !_pMocket->isConnected()) {
        _m.unlock();
        return -1;
    }
    int rc = _pMocket->cancel (bReliableTransmission, bSequencedTransmission, ui16Tag);
    rc = _pMocket->send (bReliableTransmission, bSequencedTransmission,
                         pBuf, ui32BufSize, ui16Tag, ui8Priority,
                         _ui32EnqueueTimeout, _ui32RetryTimeout);
    _m.unlock();
    return rc;
}

int MocketConnHandler::replace (bool bReliableTransmission, bool bSequencedTransmission,
                                const void *pBuf, uint32 ui32BufSize,
                                uint16 ui16Tag, uint8 ui8Priority)
{
    _m.lock();
    if (_pMocket == NULL || !_pMocket->isConnected()) {
        _m.unlock();
        return -1;
    }
    int rc = _pMocket->replace (bReliableTransmission, bSequencedTransmission,
                                pBuf, ui32BufSize, ui16Tag, ui16Tag, ui8Priority,
                                _ui32EnqueueTimeout, _ui32RetryTimeout);
    _m.unlock();
    return rc;
}

int MocketConnHandler::send (bool bReliableTransmission, bool bSequencedTransmission,
                             const void *pBuf, uint32 ui32BufSize,
                             uint16 ui16Tag, uint8 ui8Priority)
{
    _m.lock();
    if (_pMocket == NULL || !_pMocket->isConnected()) {
        _m.unlock();
        return -1;
    }
    int rc = _pMocket->send (bReliableTransmission, bSequencedTransmission,
                             pBuf, ui32BufSize, ui16Tag, ui8Priority,
                             _ui32EnqueueTimeout, _ui32RetryTimeout);
    _m.unlock();
    return rc;
}

bool MocketConnHandler::callNewPeer (void *pArg, unsigned long ulTimeSinceLastContact)
{
    if (pArg == NULL) {
        return false;
    }

    MocketConnHandler *pThisHandler = (MocketConnHandler*) pArg;
    if (!pThisHandler->isRemotePeerActive()) {
        pThisHandler->setActiveRemotePeer();
        pThisHandler->_pListener->newPeer (&(pThisHandler->_adptProp),
                                           pThisHandler->getRemotePeerNodeId(),
                                           pThisHandler->getRemotePeerAddress(), // pszPeerRemoteAddress
                                           pThisHandler->getLocalPeerAddress()); // pszIncomingInterface
    }

    return true;
}

bool MocketConnHandler::callDeadPeer (void *pArg, unsigned long ulTimeSinceLastContact)
{
    const char *pszMethodName = "MocketConnHandler::callDeadPeer";
    if (pArg != NULL) {
        MocketConnHandler *pThisHandler = (MocketConnHandler*) pArg;
        if (pThisHandler->isRemotePeerActive() &&
            (ulTimeSinceLastContact >= pThisHandler->_ui32UncreachablePeerTimeout)) {
            pThisHandler->setInactiveRemotePeer();
            checkAndLogMsg (pszMethodName, Logger::L_Info, "peer %s has been unreachable for %u "
                            "millisec. Declaring it dead.\n", pThisHandler->getRemotePeerNodeId(),
                            ulTimeSinceLastContact);
            pThisHandler->_pListener->deadPeer (&(pThisHandler->_adptProp),
                                                pThisHandler->getRemotePeerNodeId());
            return true;
        }
    }

    return false;
}

void MocketConnHandler::requestTermination (void)
{
    if (_pMocket->isConnected()) {
        _pMocket->close();
    }
    ManageableThread::requestTerminationAndWait();
}

void MocketConnHandler::requestTerminationAndWait (void)
{
    if (_pMocket->isConnected()) {
        _pMocket->close();
    }
    ManageableThread::requestTerminationAndWait();
}

