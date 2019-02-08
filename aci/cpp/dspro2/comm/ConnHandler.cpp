/*
 * ConnHandler.cpp
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

#include "ConnHandler.h"

#include "CommAdaptorListener.h"
#include "ConnEndPoint.h"
#include "MessageInfo.h"
#include "MessageProperties.h"
#include "SessionIdChecker.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "Logger.h"
#include "NetUtils.h"
#include "NLFLib.h"
#include "ControlMessageNotifier.h"
#include "SearchProperties.h"

#include <stdlib.h>

#define notifyCtrlMsgCtrl (pCtrlMsgNotifier != nullptr) && pCtrlMsgNotifier

using namespace IHMC_ACI;
using namespace NOMADSUtil;

void ConnHandler::run()
{
    started();

    BufferWrapper bw;
    while (!terminationRequested()) {
        char *pszRemotePeerAddr = nullptr;
        int rc = receive (bw, &pszRemotePeerAddr);
        if (rc > 0) {
            processPacket (bw.getBuffer(), bw.getBufferLength(), pszRemotePeerAddr);
        }
        else {
            checkAndLogMsg ("ConnHandler::run", Logger::L_Warning,
                            "receive failed with return code %d\n", rc);
            sleepForMilliseconds (300);
        }
        free (pszRemotePeerAddr);
    }

    terminating();
}

int ConnHandler::doChunkMessageRequest (const char *pszPublisherId, Reader *pReader)
{
    uint32 ui32MsgIdLen = 0;
    if (pReader->read32 (&ui32MsgIdLen) < 0) {
        return -14;
    }

    char *pszMsgId = (char *) calloc (ui32MsgIdLen + 1, sizeof (char));
    if (pReader->readBytes (pszMsgId, ui32MsgIdLen) < 0) {
        return -15;
    }

    uint8 ui8NChunks;
    if (pReader->read8 (&ui8NChunks) < 0) {
        return -16;
    }

    DArray<uint8> *pCachedChunks;
    if (ui8NChunks > 0) {
        pCachedChunks = new DArray<uint8>();
        if (pCachedChunks == nullptr) {
            checkAndLogMsg ("ConnHandler::processCtrlPacket", memoryExhausted);
            return -17;
        }

        for (uint8 i = 0; i < ui8NChunks; i++) {
            uint8 ui8;
            if (pReader->read8 (&ui8) < 0) {
                return -18;
            }
            (*pCachedChunks)[i] = ui8;
        }
    }
    else {
        pCachedChunks = nullptr;
    }

    _pListener->chunkRequestMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                            pszPublisherId, pszMsgId, pCachedChunks);
    if (notifyCtrlMsgCtrl) {
        pCtrlMsgNotifier->chunkRequestMessageArrived (_remotePeerId.c_str(), pszPublisherId);
    }
    if (pCachedChunks != nullptr) {
        delete pCachedChunks;
    }

    return 0;
}

ConnHandler::HandshakeResult ConnHandler::doHandshake (ConnEndPoint *pEndPoint, const char *pszNodeId,
                                                       const char *pszSessionId, CommAdaptorListener *pListener)
{
    HandshakeResult emptyResults ("", "");
    if (pEndPoint == nullptr || pListener == nullptr || pszNodeId == nullptr) {
        return emptyResults;
    }

    const char *pszMethodName = "ConnHandler::doHandshake";

    uint32 uiNodeIdLen = strlen (pszNodeId);
    uint32 uiSessionIdLen = (pszSessionId == nullptr ? 0U : strlen (pszSessionId));
    const String remotePeerAddr = pEndPoint->getRemoteAddress();
    const uint8 ui8RemotePeerAddrLen = remotePeerAddr.length();
    BufferWriter bw ((4 + uiNodeIdLen) + (4 + uiSessionIdLen) + (1 + ui8RemotePeerAddrLen), 0);
    if ((bw.write32 (&uiSessionIdLen) < 0) ||
        (bw.writeBytes (pszSessionId, uiSessionIdLen) < 0) ||
        (bw.write32 (&uiNodeIdLen) < 0) ||
        (bw.writeBytes (pszNodeId, uiNodeIdLen) < 0) ||
        (bw.write8 (&ui8RemotePeerAddrLen) < 0) ||
        (bw.writeBytes (remotePeerAddr, ui8RemotePeerAddrLen) < 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not write session and node ID.\n");
        return emptyResults;
    }

    // Send session ID, my own node ID, and my peer's IP address
    int rc = pEndPoint->send (bw.getBuffer(), bw.getBufferLength());
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not send my peer ID. "
                        "Returned error: %d\n", rc);
        return emptyResults;
    }

    static const int64 ui64Timeout = 1000 *  // 1 second
                                       30;   // 30 seconds
    char buf[1024];
    rc = pEndPoint->receive (buf, 1024, ui64Timeout);
    if (rc <= (4+4)) {
        // If less than 4 bytes were read, it means that not even the length of
        // the connecting peer's ID was written correctly
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not receive connecting peer's ID.\n");
        return emptyResults;
    }

    BufferReader br (buf, (uint32) rc);

    // Read session ID of the connecting peer
    uint32 ui32SessionIdLen = 0;
    if (br.read32 (&ui32SessionIdLen) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not parse connecting peer's session ID.\n");
        return emptyResults;
    }

    const String remotePeerSessionId (buf + 4, ui32SessionIdLen);
    if (!checkSessionId (remotePeerSessionId)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "received message from peer "
                        "with wrong session key: %s.\n", remotePeerSessionId.c_str());
        return emptyResults;
    }

    // Read ID of the connecting peer
    uint32 ui32PeerIdLen = 0;
    if ((br.skipBytes (ui32SessionIdLen) < 0) || (br.read32 (&ui32PeerIdLen) < 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not parse connecting peer's ID.\n");
        return emptyResults;
    }
    const String remotePeer (buf + 4 + 4 + ui32SessionIdLen, ui32PeerIdLen);

    // Read the IP address of my own network interface that connected to the peer
    const uint32 ui32ExpectedMyOwnIPLen = ((uint32) rc) - 4U - ui32SessionIdLen - 4U - ui32PeerIdLen - 1U;
    uint8 ui8MyOwnIpAddrLen = 0;
    if ((br.skipBytes (ui32PeerIdLen) < 0) || (br.read8 (&ui8MyOwnIpAddrLen) < 0) ||
        (ui32ExpectedMyOwnIPLen != ui8MyOwnIpAddrLen)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not parse my own IP address.\n");
        return emptyResults;
    }
    const String myOwnIPAddr (buf + 4 + ui32SessionIdLen + 4 + ui32PeerIdLen + 1, ui8MyOwnIpAddrLen);
    if (!InetAddr::isIPv4Addr (myOwnIPAddr)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not parse my own IP address: not IPv4.\n");
        return emptyResults;
    }

    const HandshakeResult res (remotePeer, myOwnIPAddr);
    return res;
}

int ConnHandler::doMessageRequest (const char *pszPublisherId, Reader *pReader)
{
    if (pReader == nullptr) {
        return -11;
    }
    uint32 ui32MsgIdLen = 0;
    if (pReader->read32 (&ui32MsgIdLen) < 0) {
        return -12;
    }

    char *pszMsgId = (char *) calloc (ui32MsgIdLen + 1, sizeof (char));
    if (pReader->readBytes (pszMsgId, ui32MsgIdLen) < 0) {
        return -13;
    }

    _pListener->messageRequestMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                              pszPublisherId, pszMsgId);
    if (notifyCtrlMsgCtrl) {
        pCtrlMsgNotifier->messageRequestMessageArrived (_remotePeerId.c_str(), pszPublisherId);
    }

    if (pszMsgId != nullptr) {
        free (pszMsgId);
        pszMsgId = nullptr;
    }

    return 0;
}

int ConnHandler::doSearchReply (Reader *pReader)
{
    if (pReader == nullptr) {
        return -1;
    }
    char *pszQueryId = nullptr;
    char *pszQuerier = nullptr;
    char *pszQueryType = nullptr;
    char **ppszMatchingMsgIds = nullptr;
    char *pszMatchingNode = nullptr;

    if (SearchProperties::read (pszQueryId, pszQuerier, pszQueryType, ppszMatchingMsgIds, pszMatchingNode, pReader) == 0) {
        _pListener->searchReplyMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(), (const char *)pszQueryId,
                                               (const char **)ppszMatchingMsgIds, pszMatchingNode, pszMatchingNode);
        if (notifyCtrlMsgCtrl) {
            pCtrlMsgNotifier->searchMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
        }
    }

    if (pszQueryId != nullptr) {
        free (pszQueryId);
    }
    if (pszQuerier != nullptr) {
        free (pszQuerier);
    }
    if (pszQueryType != nullptr) {
        free (pszQueryType);
    }
    if (ppszMatchingMsgIds != nullptr) {
        for (unsigned int i = 0; ppszMatchingMsgIds[i] != nullptr; i++) {
            free (ppszMatchingMsgIds[i]);
        }
        free (ppszMatchingMsgIds);
    }
    if (pszMatchingNode != nullptr) {
        free (pszMatchingNode);
    }

    return 0;
}

int ConnHandler::doVolatileSearchReply (Reader *pReader)
{
    if (pReader == nullptr) {
        return -1;
    }
    char *pszQueryId = nullptr;
    char *pszQuerier = nullptr;
    char *pszQueryType = nullptr;
    char *pszMatchingNode = nullptr;
    void *pReply = nullptr;
    uint16 ui16ReplyLen = 0;

    if (SearchProperties::read (pszQueryId, pszQuerier, pszQueryType, pReply, ui16ReplyLen, pszMatchingNode, pReader) == 0) {
        _pListener->searchReplyMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str (), (const char *) pszQueryId,
                                               pReply, ui16ReplyLen, pszMatchingNode, pszMatchingNode);
        if (notifyCtrlMsgCtrl) {
            pCtrlMsgNotifier->searchMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
        }
    }

    if (pszQueryId != nullptr) {
        free (pszQueryId);
    }
    if (pszQuerier != nullptr) {
        free (pszQuerier);
    }
    if (pszQueryType != nullptr) {
        free (pszQueryType);
    }
    if (pReply != nullptr) {
        free (pReply);
    }
    if (pszMatchingNode != nullptr) {
        free (pszMatchingNode);
    }

    return 0;
}

int ConnHandler::processPacket (const void *pBuf, uint32 ui32BufSize, char *pszRemotePeerAddr)
{
    if (pBuf == nullptr || ui32BufSize == 0 || pszRemotePeerAddr == nullptr) {
        return -1;
    }

    BufferReader br (pBuf, ui32BufSize);

    unsigned int uiShift = 0;
    uint8 ui8MsgType = 0;

    if (br.read8 (&ui8MsgType) != 0) {
        return -2;
    }
    uiShift += 1;

    MessageHeaders::MsgType type;
    if (MessageHeaders::ui8ToMsgType (ui8MsgType, type) < 0) {
        checkAndLogMsg ("ConnHandler::processPacket", Logger::L_Warning,
                        "received packet of unknown type: %d\n", ui8MsgType);
        return -3;
    }

    int rc = 0;
    switch (type) {
        case MessageHeaders::Data:
        case MessageHeaders::Metadata:
        case MessageHeaders::ChunkedData:
            rc = processDataPacket (type, br, ui32BufSize);
            break;

        default:
            rc = processCtrlPacket (type, br, pBuf, ui32BufSize, uiShift);
            break;
    }

    return rc;
}

int ConnHandler::processCtrlPacket (MessageHeaders::MsgType type, BufferReader &br, const void *pBuf,
                                    uint32 ui32BufSize, unsigned int &uiShift)
{
    char *pszPublisherId = nullptr;
    uint32 ui32PublisherLen = 0;
    if (br.read32 (&ui32PublisherLen) < 0) {
        return -10;
    }
    uiShift += 4;

    if (ui32PublisherLen > 0) {
        pszPublisherId = (char *) calloc (ui32PublisherLen+1, sizeof (char));
        if (pszPublisherId == nullptr) {
            checkAndLogMsg ("ConnHandler::processCtrlPacket", memoryExhausted);
            return -11;
        }
        else {
            if (br.readBytes (pszPublisherId, ui32PublisherLen) < 0) {
                return -11;
            }
        }
        uiShift += ui32PublisherLen;
    }

    void *pBufForListener = ((char *)pBuf) + uiShift;

    switch (type) {
        case MessageHeaders::Position:
            _pListener->positionMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                pBufForListener, ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->positionMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
            }
            break;

        case MessageHeaders::Search:
        {
            SearchProperties searchProp;
            BufferReader br (pBufForListener, ui32BufSize-uiShift);
            if (SearchProperties::read (searchProp, &br) == 0) {
                _pListener->searchMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                  &searchProp);
                if (notifyCtrlMsgCtrl) {
                    pCtrlMsgNotifier->searchMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
                }
            }
            SearchProperties::deallocate (searchProp);
            break;
        }

        case MessageHeaders::SearchReply:
        {
            BufferReader br (pBufForListener, ui32BufSize-uiShift);
            int rc = doSearchReply (&br);
            if (rc < 0) {
                return -12;
            }
            break;
        }

        case MessageHeaders::VolSearchReply:
        {
            BufferReader br (pBufForListener, ui32BufSize - uiShift);
            int rc = doVolatileSearchReply (&br);
            if (rc < 0) {
                return -12;
            }
            break;
        }

        case MessageHeaders::TopoReq:
            _pListener->topologyRequestMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                       pBufForListener, ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->topologyRequestMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
            }
            break;

        case MessageHeaders::TopoReply:
            _pListener->topologyReplyMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                     pBufForListener, ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->topologyReplyMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
            }
            break;

        case MessageHeaders::CtxtUpdates_V1:
            _pListener->updateMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                              pszPublisherId, pBufForListener,
                                              ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->updateMessageArrived (_remotePeerId.c_str(), pszPublisherId);
            }
            break;

        case MessageHeaders::CtxtUpdates_V2:
            _pListener->contextUpdateMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                     pBuf, ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->contextUpdateMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
            }
            break;

        case MessageHeaders::CtxtVersions_V1:
            _pListener->versionMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                               pszPublisherId, pBufForListener,
                                               ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->versionMessageArrived (_remotePeerId.c_str(), pszPublisherId);
            }
            break;

        case MessageHeaders::CtxtVersions_V2:
            _pListener->contextVersionMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                      pBufForListener, ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->contextVersionMessageArrived (_remotePeerId.c_str(), _remotePeerId.c_str());
            }
            break;

        case MessageHeaders::WayPoint:
            _pListener->waypointMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                                pszPublisherId, pBufForListener,
                                                ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->waypointMessageArrived (_remotePeerId.c_str(), pszPublisherId);
            }
            break;

        case MessageHeaders::CtxtWhole_V1:
            _pListener->wholeMessageArrived (_adptProp.uiAdaptorId, _remotePeerId.c_str(),
                                             pszPublisherId, pBufForListener,
                                             ui32BufSize-uiShift);
            if (notifyCtrlMsgCtrl) {
                pCtrlMsgNotifier->wholeMessageArrived (_remotePeerId.c_str(), pszPublisherId);
            }
            break;

        case MessageHeaders::MessageRequest:
        {
            int rc = doMessageRequest (pszPublisherId, &br);
            if (rc < 0) {
                return -13;
            }

            break;
        }

        case MessageHeaders::ChunkRequest:
        {
            int rc = doChunkMessageRequest (pszPublisherId, &br);
            if (rc < 0) {
                return -14;
            }

            break;
        }

        default:
            checkAndLogMsg ("ConnHandler::processCtrlPacket", Logger::L_Warning,
                            "Unknown message arrived\n");
    }

    if (pszPublisherId != nullptr) {
        free (pszPublisherId);
        pszPublisherId = nullptr;
    }
    return 0;
}

int ConnHandler::processDataPacket (MessageHeaders::MsgType type, BufferReader &br, uint32 ui32BufSize)
{
    const char *pszMethodName = "ConnHandler::processDataPacket";

    // Read Message Header
    MessageHeader *pMH = nullptr;
    if (type == MessageHeaders::ChunkedData) {
        pMH = new ChunkMsgInfo();
    }
    else {
        pMH = new MessageInfo();
    }
    if (pMH == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return -4;
    }

    if (pMH->read (&br, ui32BufSize - br.getTotalBytesRead()) < 0) {
        delete pMH;
        return -5;
    }

    uint32 ui32PayloadLen = pMH->getFragmentLength();
    if (ui32PayloadLen == 0) {
        delete pMH;
        return -6;
    }

    void *pDataPayload = calloc (ui32PayloadLen, 1);
    if (pDataPayload == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        delete pMH;
        return -7;
    }

    uint32 ui32AnnotationMetadataLen = 0U;
    const void *pAnnotationMetadataBuf = pMH->getAnnotationMetadata (ui32AnnotationMetadataLen);
    MessageProperties msgProp (pMH->getPublisherNodeId(), pMH->getLargeObjectId(),
                               pMH->getObjectId(), pMH->getInstanceId(),
                               pMH->getAnnotates(), pAnnotationMetadataBuf, ui32AnnotationMetadataLen,
                               pMH->getMimeType(), pMH->getChecksum(), nullptr, pMH->getExpiration());

    if (pMH->isChunk()) {
        if (br.readBytes (pDataPayload, ui32PayloadLen) < 0) {
            delete pMH;
            free (pDataPayload);
            return -8;
        }
        ChunkMsgInfo *pCMI = (ChunkMsgInfo*) pMH;

        checkAndLogMsg (pszMethodName, Logger::L_Info, "received message "
                        "chunk with id %s, \n", pMH->getMsgId());

        _pListener->dataArrived (&_adptProp, &msgProp, pDataPayload,
                                 ui32PayloadLen, pMH->getChunkId(),
                                 pCMI->getTotalNumberOfChunks());
    }
    else {
        uint8 ui8Type = 0;
        if (br.read8 (&ui8Type) < 0) {
            delete pMH;
            free (pDataPayload);
            return -9;
        }
        ui32PayloadLen -= 1U;
        if (br.readBytes (pDataPayload, ui32PayloadLen) < 0) {
            delete pMH;
            free (pDataPayload);
            return -10;
        }

        if (ui8Type == MessageHeaders::Data) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "received message with id %s,\n", pMH->getMsgId());
            _pListener->dataArrived (&_adptProp, &msgProp, pDataPayload,
                                     ui32PayloadLen, 0, 0);
        }
        else if (ui8Type == MessageHeaders::Metadata) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "received metadata message with id %s,\n", pMH->getMsgId());

            _pListener->metadataArrived (&_adptProp, &msgProp, pDataPayload,
                                         ui32PayloadLen, nullptr);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "can't parse data message\n");
            delete pMH;
            free (pDataPayload);
            return -11;
        }
    }

    delete pMH;
    free (pDataPayload);
    return 0;
}

ConnHandler::BufferWrapper::BufferWrapper (void)
{
    _pBuf = nullptr;
    _ui32BufSize = 0L;
    _bDeleteWhenDone = false;
}

ConnHandler::BufferWrapper::~BufferWrapper (void)
{
    if (_bDeleteWhenDone && _pBuf != nullptr) {
        free (_pBuf);
        _pBuf = nullptr;
        _ui32BufSize = 0U;
    }
}

int ConnHandler::BufferWrapper::init (void *pBuf, uint32 ui32BufSize, bool bDeleteWhenDone)
{
    if (_bDeleteWhenDone && _pBuf != nullptr) {
        free (_pBuf);
        _pBuf = nullptr;
        _ui32BufSize = 0U;
    }
    _pBuf = pBuf;
    _ui32BufSize = ui32BufSize;
    _bDeleteWhenDone = bDeleteWhenDone;
    return 0;
}


