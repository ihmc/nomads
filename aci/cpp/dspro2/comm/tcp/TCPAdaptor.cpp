/*
 * TCPAdaptor.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 14, 2014, 12:04 PM
 */

#include "TCPAdaptor.h"

#include "Defs.h"
#include "TCPConnHandler.h"
#include "TCPConnListener.h"
#include "TCPEndPoint.h"

#include "ConfigFileReader.h"
#include "Message.h"
#include "MessageInfo.h"
#include "SearchProperties.h"
#include "SessionId.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "TCPSocket.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const unsigned short TCPAdaptor::DEFAULT_PORT = 8888;

TCPAdaptor::TCPAdaptor (AdaptorId uiId, const char *pszNodeId,
                        CommAdaptorListener *pListener, uint16 ui16Port)
    : ConnCommAdaptor (uiId, TCP, false, pszNodeId, pListener, ui16Port),
      _ui8DefaultPriority (5)
{
}

TCPAdaptor::~TCPAdaptor (void)
{
}

int TCPAdaptor::init (ConfigManager *pCfgMgr)
{
    setName ("IHMC_ACI::TCPAdaptor");
    char **ppszNetIfs = ConfigFileReader::parseNetIFs (pCfgMgr->getValue ("aci.dspro.tcp.netIFs"));
    int rc = ConnCommAdaptor::init (ppszNetIfs);
    if (ppszNetIfs != nullptr) {
        // deallocate
    }
    return rc;
}

void TCPAdaptor::resetTransmissionCounters (void)
{
    // Nothing to do
}

bool TCPAdaptor::supportsManycast (void)
{
    return false;
}

int TCPAdaptor::connectToPeerInternal (const char *pszRemotePeerAddr, uint16 ui16Port)
{
    const char *pszMethodName = "TCPAdaptor::connectToPeerInternal";

    if (pszRemotePeerAddr == nullptr) {
        return -1;
    }

    TCPSocket *pSocket = new TCPSocket();
    if (pSocket == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return -2;
    }

    // pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, nullptr);
    int rc = pSocket->connect (pszRemotePeerAddr, ui16Port);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to connect to "
                        "host %s:%d; rc = %d\n", pszRemotePeerAddr, ui16Port, rc);
        delete pSocket;
        return -3;
    }

    // Create handler and delegate pMocket handling
    const String sessionId (SessionId::getInstance()->getSessionId());
    TCPEndPoint endPoint (pSocket, ConnEndPoint::DEFAULT_TIMEOUT);
    const ConnHandler::HandshakeResult handshake (ConnHandler::doHandshake (&endPoint, _nodeId, sessionId, _pListener));
    if (handshake._remotePeerId.length() <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "handshake failed\n");
        pSocket->disconnect();
        delete pSocket;
        return -4;
    }

    TCPConnHandler *pHandler = new TCPConnHandler (_adptorProperties, handshake._remotePeerId,
                                                   _pListener, pSocket, handshake._localConnectionIfaceAddr);
    if (pHandler == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        pSocket->disconnect();
        delete pSocket;
        return -5;
    }
    else {
        pHandler->init();
        pHandler->start();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "a TCPConnHandler was "
                        "created for peer %s.\n", pHandler->getRemotePeerNodeId());
        addHandler (pHandler);
        return 0;
    }
}

ConnListener * TCPAdaptor::getConnListener (const char *pszListenAddr, uint16 ui16Port,
                                            const char *pszNodeId, const char *pszSessionId,
                                            CommAdaptorListener *pListener,
                                            ConnCommAdaptor *pCommAdaptor)
{
    return new TCPConnListener (pszListenAddr, ui16Port, pszNodeId, pszSessionId,
                                pListener, pCommAdaptor);
}

int TCPAdaptor::sendContextUpdateMessage (const void *pBuf, uint32 ui32Len,
                                          const char **ppszRecipientNodeIds,
                                          const char **ppszInterfaces)
{
    return 0;
}

int TCPAdaptor::sendContextVersionMessage (const void *pBuf, uint32 ui32Len,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces)
{
    return 0;
}

int TCPAdaptor::sendDataMessage (Message *pMsg, const char **ppszRecipientNodeIds,
                                 const char **ppszInterfaces)
{
    if (pMsg == nullptr || pMsg->getMessageHeader() == nullptr || ppszRecipientNodeIds == nullptr) {
        return -1;
    }

    BufferWriter bw (pMsg->getMessageHeader()->getFragmentLength() + 512U, 256U);

    const uint8 ui8MsgType = MessageHeaders::Data;
    bw.write8 (&ui8MsgType);
    pMsg->getMessageHeader()->write (&bw, 1024U);
    bw.writeBytes (pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());

    int rc = 0;
    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != nullptr; i++) {
        ConnHandler *pHandler = _handlersByPeerId.get (ppszRecipientNodeIds[i]);
        if (pHandler != nullptr) {
            int rc = pHandler->send (bw.getBuffer(), bw.getBufferLength(), pMsg->getMessageHeader()->getPriority());
            if (rc < 0) {
                checkAndLogMsg ("TCPAdaptor::sendDataMessage", Logger::L_Warning,
                                "could not send %s message to peer %s (%s).  Returned code: %d\n",
                                MessageHeaders::getMetadataAsString (&ui8MsgType), pHandler->getRemotePeerNodeId(),
                                pHandler->getRemotePeerAddress(), rc);
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), pHandler->getRemotePeerAddressPort());
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg ("TCPAdaptor::sendDataMessage", Logger::L_Info, "sent %s message.\n",
                                MessageHeaders::getMetadataAsString (&ui8MsgType));
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return (rc == 0 ? 0 : -2);
}

int TCPAdaptor::sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
                                    const char **ppszRecipientNodeIds,
                                    const char **ppszInterfaces)
{
    if (pMsg == nullptr || ppszRecipientNodeIds == nullptr) {
        return -1;
    }

    // TODO: you may want to do something with pszDataMimeType...
    BufferWriter bw (pMsg->getMessageHeader()->getFragmentLength() + 512U, 256U);

    const uint8 ui8MsgType = MessageHeaders::ChunkedData;
    bw.write8 (&ui8MsgType);
    pMsg->getMessageHeader()->write (&bw, 1024U);
    bw.writeBytes (pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());

    int rc = 0;
    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != nullptr; i++) {
        ConnHandler *pHandler = _handlersByPeerId.get (ppszRecipientNodeIds[i]);
        if (pHandler != nullptr) {
            int rc = pHandler->send (bw.getBuffer(), bw.getBufferLength(), pMsg->getMessageHeader()->getPriority());
            if (rc < 0) {
                checkAndLogMsg ("TCPAdaptor::sendDataMessage", Logger::L_Warning,
                                "could not send %s message to peer %s (%s).  Returned code: %d\n",
                                MessageHeaders::getMetadataAsString (&ui8MsgType), pHandler->getRemotePeerNodeId(),
                                pHandler->getRemotePeerAddress(), rc);
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), pHandler->getRemotePeerAddressPort());
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg ("TCPAdaptor::sendDataMessage", Logger::L_Info, "sent %s message.\n",
                                MessageHeaders::getMetadataAsString (&ui8MsgType));
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return (rc == 0 ? 0 : -2);
}

int TCPAdaptor::sendMessageRequestMessage (const char *pszMsgId, const char *pszPublisherNodeId,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces)
{
    if (pszMsgId == nullptr || ppszRecipientNodeIds == nullptr) {
        return -1;
    }

    uint32 ui32IdLen = strlen (pszMsgId);
    if (ui32IdLen == 0) {
        return -2;
    }

    BufferWriter bw (4+ui32IdLen, 64);
    bw.write32 (&ui32IdLen);
    bw.writeBytes (pszMsgId, ui32IdLen);

    return sendMessage (MessageHeaders::MessageRequest, bw.getBuffer(), bw.getBufferLength(),
                        pszPublisherNodeId, ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendChunkRequestMessage (const char *pszMsgId, DArray<uint8> *pCachedChunks,
                                         const char *pszPublisherNodeId,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces)
{
    if (pszMsgId == nullptr || ppszRecipientNodeIds == nullptr) {
        return -1;
    }

    uint32 ui32IdLen = strlen (pszMsgId);
    if (ui32IdLen == 0) {
        return -2;
    }

    uint8 ui8NCachedChunks = pCachedChunks == nullptr ? 0 : pCachedChunks->size();

    BufferWriter bw (4+ui32IdLen+(ui8NCachedChunks+1), 64);
    bw.write32 (&ui32IdLen);
    bw.writeBytes (pszMsgId, ui32IdLen);

    // Write locally cached chunks
    bw.write8 (&ui8NCachedChunks);
    for (unsigned int i = 0; i < ui8NCachedChunks; i++) {
        uint8 ui8 = (*pCachedChunks)[i];
        bw.write8 (&ui8);
    }

    return sendMessage (MessageHeaders::ChunkRequest, bw.getBuffer(), bw.getBufferLength(),
                        pszPublisherNodeId, ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendPositionMessage (const void *pBuf, uint32 ui32Len,
                                     const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::Position, pBuf, ui32Len, nullptr,
                        ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendSearchMessage (SearchProperties &searchProp,
                                   const char **ppszRecipientNodeIds,
                                   const char **ppszInterfaces)
{
    BufferWriter bw (searchProp.uiQueryLen + 128, 128);
    SearchProperties::write (searchProp, &bw);
    return sendMessage (MessageHeaders::Search, bw.getBuffer(), bw.getBufferLength(), nullptr,
                        ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendSearchReplyMessage (const char *pszQueryId,
                                        const char **ppszMatchingMsgIds,
                                        const char *pszTarget,
                                        const char *pszMatchingNode,
                                        const char **ppszRecipientNodeIds,
                                        const char **ppszInterfaces)
{
    if (pszQueryId == nullptr) {
        return -1;
    }
    uint16 ui16 = strlen (pszQueryId);
    if (ui16 == 0) {
        return -2;
    }
    BufferWriter bw (256, 128);
    if (SearchProperties::write (pszQueryId, pszTarget, nullptr, // pszQueryType
                                 ppszMatchingMsgIds, pszMatchingNode, &bw) < 0) {
        return -3;
    }

    return sendMessage (MessageHeaders::SearchReply, bw.getBuffer(), bw.getBufferLength(), nullptr,
                        ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendVolatileSearchReplyMessage (const char *pszQueryId,
                                                const void *pReply, uint16 ui16ReplyLen,
                                                const char *pszTarget, const char *pszMatchingNode,
                                                const char **ppszRecipientNodeIds,
                                                const char **ppszInterfaces)
{
    if (pszQueryId == nullptr) {
        return -1;
    }
    uint16 ui16 = strlen (pszQueryId);
    if (ui16 == 0) {
        return -2;
    }
    BufferWriter bw (256, 128);
    if (SearchProperties::write (pszQueryId, pszTarget, nullptr, // pszQueryType
                                 pReply, ui16ReplyLen, pszMatchingNode, &bw) < 0) {
        return -3;
    }

    return sendMessage (MessageHeaders::SearchReply, bw.getBuffer(), bw.getBufferLength(), nullptr,
                        ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
                                              const char **ppszRecipientNodeIds,
                                              const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::TopoReply, pBuf, ui32BufLen, nullptr,
                        ppszRecipientNodeIds, ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len,
                                                const char **ppszRecipientNodeIds,
                                                const char **ppszInterfaces)
{
    return 0;
}

int TCPAdaptor::sendUpdateMessage (const void *pBuf, uint32 ui32Len,
                                   const char *pszPublisherNodeId,
                                   const char **ppszRecipientNodeIds,
                                   const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::CtxtUpdates_V1, pBuf, ui32Len,
                        pszPublisherNodeId, ppszRecipientNodeIds,
                        ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendVersionMessage (const void *pBuf, uint32 ui32Len,
                                    const char *pszPublisherNodeId,
                                    const char **ppszRecipientNodeIds,
                                    const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::CtxtVersions_V1, pBuf, ui32Len,
                        pszPublisherNodeId, ppszRecipientNodeIds,
                        ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendWaypointMessage (const void *pBuf, uint32 ui32Len,
                                     const char *pszPublisherNodeId,
                                     const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::WayPoint, pBuf, ui32Len,
                        pszPublisherNodeId, ppszRecipientNodeIds,
                        ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::sendWholeMessage (const void *pBuf, uint32 ui32Len,
                                  const char *pszPublisherNodeId,
                                  const char **ppszRecipientNodeIds,
                                  const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::CtxtWhole_V1, pBuf, ui32Len,
                        pszPublisherNodeId, ppszRecipientNodeIds,
                        ppszInterfaces, _ui8DefaultPriority);
}

int TCPAdaptor::notifyEvent (const void *pBuf, uint32 ui32Len,
                             const char *pszPublisherNodeId,
                             const char *pszTopic, const char **ppszInterfaces)
{
    return 0;
}

