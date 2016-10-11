/* 
 * MocketsAdaptor.cpp
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

#include "MocketsAdaptor.h"

#include "ConfigFileReader.h"
#include "Message.h"
#include "MessageInfo.h"
#include "MocketConnHandler.h"
#include "MocketConnListener.h"
#include "MocketsEndPoint.h"
#include "SearchProperties.h"

#include "Mocket.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "FileUtils.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const unsigned short MocketsAdaptor::DEFAULT_PORT = 6688;

MocketsAdaptor::MocketsAdaptor (AdaptorId uiId, const char *pszNodeId, const char *pszSessionId,
                                CommAdaptorListener *pListener, uint16 ui16Port)
    : ConnCommAdaptor (uiId, MOCKETS, false, pszNodeId, pszSessionId, pListener, ui16Port),
      _ui16DefaultPriority (0)
{
}

MocketsAdaptor::~MocketsAdaptor()
{
}

int MocketsAdaptor::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    setName ("IHMC_ACI::MocketsAdaptor");

    const String mocketsCfgFileProperty ("aci.dspro.mockets.conf");
    if (pCfgMgr->hasValue (mocketsCfgFileProperty)) {
        String mocketsCfgFile (pCfgMgr->getValue (mocketsCfgFileProperty));
        if (mocketsCfgFile.length() > 0) {
            if (FileUtils::fileExists (mocketsCfgFile)) {
                checkAndLogMsg ("MocketsAdaptor::init", Logger::L_Info, "found mockets "
                                "config file: %s.\n", (const char *)_mocketsCfgFile);
                _mocketsCfgFile = mocketsCfgFile;
            }
            else {
                checkAndLogMsg ("MocketsAdaptor::init", Logger::L_Warning, "mockets "
                                "config file %s was not found.\n", (const char *)_mocketsCfgFile);
            }
        }
    }

    char **ppszNetIfs = ConfigFileReader::parseNetIFs (pCfgMgr->getValue("aci.dspro.mockets.netIFs"));
    int rc = ConnCommAdaptor::init (ppszNetIfs);
    if (ppszNetIfs != NULL) {
        // deallocate
    }

    return rc;
}

int MocketsAdaptor::connectToPeerInternal (const char *pszRemotePeerAddr, uint16 ui16Port)
{
    const char *pszMethodName = "MocketsAdaptor::connectToPeerInternal";

    if (pszRemotePeerAddr == NULL) {
        return -1;
    }

    Mocket *pMocket = new Mocket (_mocketsCfgFile);
    if (pMocket == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return -2;
    }

    int rc = pMocket->connect (pszRemotePeerAddr, ui16Port);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to connect to "
                        "host %s:%d; rc = %d\n", pszRemotePeerAddr, ui16Port, rc);
        delete pMocket;
        return -3;
    }

    // Create handler and delegate pMocket handling
    MocketEndPoint endPoint (pMocket, ConnEndPoint::DEFAULT_TIMEOUT);
    const ConnHandler::HandshakeResult handshake (ConnHandler::doHandshake (&endPoint, _nodeId, _sessionId, _pListener));
    if (handshake._remotePeerId.length() <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "handshake failed\n");
        pMocket->close();
        delete pMocket;
        return -4;
    }

    MocketConnHandler *pHandler = new MocketConnHandler (_adptorProperties, handshake._remotePeerId,
                                                         _pListener, pMocket);
    if (pHandler == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        pMocket->close();
        delete pMocket;
        return -5;
    }
    else {
        pHandler->init();
        pHandler->start();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "a MocketConnHandler was "
                        "created for peer %s.\n", pHandler->getRemotePeerNodeId());
        addHandler (pHandler);
        return 0;
    }
}

ConnListener * MocketsAdaptor::getConnListener (const char *pszListenAddr, uint16 ui16Port,
                                                const char *pszNodeId, const char *pszSessionId,
                                                CommAdaptorListener *pListener,
                                                ConnCommAdaptor *pCommAdaptor)
{
    return new MocketConnListener (pszListenAddr, ui16Port, pszNodeId, pszSessionId,
                                   _mocketsCfgFile, pListener, pCommAdaptor);
}

int MocketsAdaptor::disconnectFromPeer (const char *pszRemotePeerNodeId,
                                        const char *pszRemotePeerAddr)
{
    if (pszRemotePeerNodeId== NULL || pszRemotePeerAddr == NULL) {
        return -1;
    }

    _mHandlers.lock();
    ConnHandler *pHandler = _handlersByPeerId.remove (pszRemotePeerNodeId);
    _mHandlers.unlock();
    if (pHandler == NULL) {
        return -2;
    }

    pHandler->requestTerminationAndWait();
    delete pHandler;

    return 0;
}

void MocketsAdaptor::resetTransmissionCounters()
{
    _mHandlers.lock();
    ConnHandlers::Iterator iter = _handlersByPeerId.getAllElements();
    for  (; !iter.end(); iter.nextElement()) {
        ConnHandler *pHandler = iter.getValue();
        if (pHandler != NULL) {
            pHandler->resetTransmissionCounters();
        }
    }
    _mHandlers.unlock();
}

int MocketsAdaptor::sendContextUpdateMessage (const void *pBuf, uint32 ui32Len,
                                              const char **ppszRecipientNodeIds,
                                              const char **ppszInterfaces)
{
    return 0;
}

int MocketsAdaptor::sendContextVersionMessage (const void *pBuf, uint32 ui32Len,
                                               const char **ppszRecipientNodeIds,
                                               const char **ppszInterfaces)
{
    return 0;
}

int MocketsAdaptor::sendDataMessage (Message *pMsg, const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces)
{
    if (pMsg == NULL || pMsg->getMessageHeader() == NULL || ppszRecipientNodeIds == NULL) {
        return -1;
    }

    BufferWriter bw (pMsg->getMessageHeader()->getFragmentLength() + 512U, 256U);

    uint8 ui8MsgType = MessageHeaders::Data;
    bw.write8 (&ui8MsgType);
    pMsg->getMessageHeader()->write (&bw, 1024U);
    bw.writeBytes (pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());

    int rc = 0;
    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        MocketConnHandler *pHandler = (MocketConnHandler *) _handlersByPeerId.get (ppszRecipientNodeIds[i]);
        if (pHandler != NULL) {
            int rcTmp = pHandler->sendDataMessage (bw.getBuffer(), bw.getBufferLength(),
                                                   pMsg->getMessageHeader()->getPriority());
            if (rcTmp != 0) {
                checkAndLogMsg ("MocketsAdaptor::sendDataMessage", Logger::L_Warning,
                                "error sending message %s to %s. Returned code: %d\n",
                                pMsg->getMessageHeader()->getMsgId(),
                                pHandler->getRemotePeerAddress(), rcTmp);
                if (rc == 0) {
                    rc = -2;
                }
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), _ui16Port);
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg ("MocketsAdaptor::sendDataMessage", Logger::L_Info,
                                "message %s sent to %s.\n", pMsg->getMessageHeader()->getMsgId(),
                                pHandler->getRemotePeerAddress(), rcTmp); 
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return (rc == 0 ? 0 : -2);
}

int MocketsAdaptor::sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
                                        const char **ppszRecipientNodeIds,
                                        const char **ppszInterfaces)
{
    if (pMsg == NULL || ppszRecipientNodeIds == NULL) {
        return -1;
    }

    // TODO: you may want to do something with pszDataMimeType...
    BufferWriter bw (pMsg->getMessageHeader()->getFragmentLength() + 512U, 256U);

    uint8 ui8MsgType = MessageHeaders::ChunkedData;
    bw.write8 (&ui8MsgType);
    pMsg->getMessageHeader()->write (&bw, 1024U);
    bw.writeBytes (pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());

    int rc = 0;
    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        MocketConnHandler *pHandler = (MocketConnHandler *) _handlersByPeerId.get (ppszRecipientNodeIds[i]);
        if (pHandler != NULL) {
            int rcTmp = pHandler->sendDataMessage (bw.getBuffer(), bw.getBufferLength(),
                                                   pMsg->getMessageHeader()->getPriority());
            if (rcTmp != 0) {
                checkAndLogMsg ("MocketsAdaptor::sendChunkedMessage", Logger::L_Warning,
                                "error sending message %s to %s. Returned code: %d\n",
                                pMsg->getMessageHeader()->getMsgId(),
                                pHandler->getRemotePeerAddress(), rcTmp);
                if (rc == 0) {
                    rc = -2;
                }
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), _ui16Port);
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg ("MocketsAdaptor::sendChunkedMessage", Logger::L_Info,
                                "message %s sent to %s.\n", pMsg->getMessageHeader()->getMsgId(),
                                pHandler->getRemotePeerAddress(), rcTmp); 
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return (rc == 0 ? 0 : -2);
}

int MocketsAdaptor::sendMessageRequestMessage (const char *pszMsgId, const char *pszPublisherNodeId,
                                               const char **ppszRecipientNodeIds,
                                               const char **ppszInterfaces)
{
    if (pszMsgId == NULL || ppszRecipientNodeIds == NULL) {
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
                        pszPublisherNodeId, ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendChunkRequestMessage (const char *pszMsgId,
                                             NOMADSUtil::DArray<uint8> *pCachedChunks,
                                             const char *pszPublisherNodeId,
                                             const char **ppszRecipientNodeIds,
                                             const char **ppszInterfaces)
{
    if (pszMsgId == NULL || ppszRecipientNodeIds == NULL) {
        return -1;
    }

    uint32 ui32IdLen = strlen (pszMsgId);
    if (ui32IdLen == 0) {
        return -2;
    }

    uint8 ui8NCachedChunks = pCachedChunks == NULL ? 0 : pCachedChunks->size();

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
                        pszPublisherNodeId, ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendPositionMessage (const void *pBuf, uint32 ui32Len,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::Position, pBuf, ui32Len, NULL,
                        ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendSearchMessage (SearchProperties &searchProp,
                                       const char **ppszRecipientNodeIds,
                                       const char **ppszInterfaces)
{
    BufferWriter bw (searchProp.uiQueryLen + 128, 128);
    SearchProperties::write (searchProp, &bw);
    return sendMessage (MessageHeaders::Search, bw.getBuffer(), bw.getBufferLength(), NULL,
                        ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendSearchReplyMessage (const char *pszQueryId,
                                            const char **ppszMatchingMsgIds,
                                            const char *pszTarget,
                                            const char *pszMatchingNode,
                                            const char **ppszRecipientNodeIds,
                                            const char **ppszInterfaces)
{
    if (pszQueryId == NULL) {
        return -1;
    }
    uint16 ui16 = strlen (pszQueryId);
    if (ui16 == 0) {
        return -2;
    }
    BufferWriter bw (256, 128);
    if (SearchProperties::write (pszQueryId, pszTarget, NULL, // pszQueryType
                                 ppszMatchingMsgIds, pszMatchingNode, &bw) < 0) {
        return -3;
    }

    return sendMessage (MessageHeaders::SearchReply, bw.getBuffer(), bw.getBufferLength(), NULL,
                        ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendVolatileSearchReplyMessage (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                                    const char *pszTarget, const char *pszMatchingNode,
                                                    const char **ppszRecipientNodeIds,
                                                    const char **ppszInterfaces)
{
    if (pszQueryId == NULL) {
        return -1;
    }
    uint16 ui16 = strlen (pszQueryId);
    if (ui16 == 0) {
        return -2;
    }
    BufferWriter bw (256, 128);
    if (SearchProperties::write (pszQueryId, pszTarget, NULL, // pszQueryType
                                 pReply, ui16ReplyLen, pszMatchingNode, &bw) < 0) {
        return -3;
    }

    return sendMessage (MessageHeaders::VolSearchReply, bw.getBuffer(), bw.getBufferLength(), NULL,
                        ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
                                              const char **ppszRecipientNodeIds,
                                              const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::TopoReply, pBuf, ui32BufLen, NULL,
                        ppszRecipientNodeIds, ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len,
                                                const char **ppszRecipientNodeIds,
                                                const char **ppszInterfaces)
{
    return 0;
}

int MocketsAdaptor::sendUpdateMessage (const void *pBuf, uint32 ui32Len,
                                       const char *pszPublisherNodeId,
                                       const char **ppszRecipientNodeIds,
                                       const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::CtxtUpdates_V1, pBuf, ui32Len,
                        pszPublisherNodeId, ppszRecipientNodeIds,
                        ppszInterfaces, _ui16DefaultPriority);
}

int MocketsAdaptor::sendVersionMessage (const void *pBuf, uint32 ui32Len,
                                        const char *pszPublisherNodeId,
                                        const char **ppszRecipientNodeIds,
                                        const char **ppszInterfaces)
{
    // Add the publisher - if necessary
    uint32 ui32PubLen = (pszPublisherNodeId == NULL ? 0 : strlen (pszPublisherNodeId));
    BufferWriter bw (1U + 4U + ui32PubLen + ui32Len, 128U);

    bw.reset();

    uint8 type = MessageHeaders::CtxtVersions_V1;
    if (bw.write8 (&type) < 0) {
        return -2;
    }
    if (bw.write32 (&ui32PubLen) < 0) {
        return -3;
    }
    if (ui32PubLen > 0 && bw.writeBytes (pszPublisherNodeId, ui32PubLen) < 0) {
        return -4;
    }

    // Add the received payload
    if (ui32PubLen > 0 && bw.writeBytes (pBuf, ui32Len) < 0) {
        return -5;
    }

    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        MocketConnHandler *pHandler = static_cast<MocketConnHandler *>(_handlersByPeerId.get (ppszRecipientNodeIds[i]));
        if (pHandler != NULL) {
            int rc = pHandler->sendVersionMessage (bw.getBuffer(), bw.getBufferLength(),
                                                   _ui16DefaultPriority);
            if (rc < 0) {
                checkAndLogMsg ("MocketsAdaptor::sendVersionMessage", Logger::L_Warning,
                                "could not send version message.  Returned code: %d\n", rc);
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), _ui16Port);
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg ("MocketsAdaptor::sendVersionMessage", Logger::L_Info,
                                "sent version message.\n");
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return 0;
}

int MocketsAdaptor::sendWaypointMessage (const void *pBuf, uint32 ui32Len,
                                         const char *pszPublisherNodeId,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces)
{
    // Add the publisher - if necessary
    uint32 ui32PubLen = (pszPublisherNodeId == NULL ? 0 : strlen (pszPublisherNodeId));
    BufferWriter bw (1U + 4U + ui32PubLen + ui32Len, 128U);

    bw.reset();

    uint8 type = MessageHeaders::WayPoint;
    if (bw.write8 (&type) < 0) {
        return -2;
    }
    if (bw.write32 (&ui32PubLen) < 0) {
        return -3;
    }
    if (ui32PubLen > 0 && bw.writeBytes (pszPublisherNodeId, ui32PubLen) < 0) {
        return -4;
    }

    // Add the received payload
    if (ui32PubLen > 0 && bw.writeBytes (pBuf, ui32Len) < 0) {
        return -5;
    }

    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        MocketConnHandler *pHandler = (MocketConnHandler *) _handlersByPeerId.get (ppszRecipientNodeIds[i]);
        if (pHandler != NULL) {
            int rc = pHandler->sendWaypointMessage (bw.getBuffer(), bw.getBufferLength(),
                                                    _ui16DefaultPriority,
                                                    pszPublisherNodeId == NULL ? _nodeId.c_str() : pszPublisherNodeId);
            if (rc < 0) {
                checkAndLogMsg ("MocketsAdaptor::sendWaypointMessage", Logger::L_Warning,
                                "could not send waypoint message.  Returned code: %d\n", rc);
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), _ui16Port);
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg ("MocketsAdaptor::sendWaypointMessage", Logger::L_Info,
                                "sent version message.\n");
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return 0;
}

int MocketsAdaptor::sendWholeMessage (const void *pBuf, uint32 ui32Len,
                                      const char *pszPublisherNodeId,
                                      const char **ppszRecipientNodeIds,
                                      const char **ppszInterfaces)
{
    return sendMessage (MessageHeaders::CtxtWhole_V1, pBuf, ui32Len,
                        pszPublisherNodeId, ppszRecipientNodeIds,
                        ppszInterfaces, _ui16DefaultPriority);
}

