/*
 * PositionUpdater.cpp
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

#include "PositionUpdater.h"

#include "Controller.h"
#include "Defs.h"
#include "DSPro.h"
#include "DSProImpl.h"
#include "DSSFLib.h"
#include "MessageIdGenerator.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"
#include "NodeContextManager.h"

#include "BufferWriter.h"
#include "Logger.h"
#include "NLFLib.h"
#include "DataStore.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

PositionUpdater::PositionUpdater (NodeContextManager *pNodeContexMgr,
                                  DSProImpl *pDSPro)
    : _bMessageRequested (false), _i64TimeStamp (0), _cv (&_m)
{
    _pNodeContexMgr = pNodeContexMgr;
    _pDSPro = pDSPro;
    _pMsgToNotify = new StringHashtable<LList<String> >(true, true, true, true);
}

PositionUpdater::~PositionUpdater (void)
{
}

void PositionUpdater::addMetadataToNotify (const char *pszQueryId, const char **ppszMsgIds)
{
    if (ppszMsgIds == NULL || ppszMsgIds == NULL) {
        return;
    }

    _m.lock();
    MsgIdList *pMsgIds = _pMsgToNotify->get (pszQueryId);
    bool bAtLeastOne = false;
    bool bNewMsgIdList = (pMsgIds == NULL);
    if (bNewMsgIdList) {
        pMsgIds = new MsgIdList();
        if (pMsgIds == NULL) {
            checkAndLogMsg ("PositionUpdater::addMetadataToNotify", memoryExhausted);
            _m.unlock();
            return;
        }
        _pMsgToNotify->put (pszQueryId, pMsgIds);
    }
    for (unsigned int i = 0; ppszMsgIds[i] != NULL; i++) {
        String msgId (ppszMsgIds[i]);
        if (bNewMsgIdList || (pMsgIds->search (msgId) == 0)) {
            bAtLeastOne = true;
            pMsgIds->add (msgId);
            checkAndLogMsg ("PositionUpdater::addMetadataToNotify", Logger::L_Info,
                            "requested message %s\n", ppszMsgIds[i]);
        }
    }

    if (bAtLeastOne) {
        _cv.notify();
    }
    _m.unlock();
}

void PositionUpdater::addSearchToNotify (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen)
{
    // TODO: implement this
}

void PositionUpdater::positionUpdated (void)
{
    _m.lock();
    _i64TimeStamp = getTimeInMilliseconds();
    _m.unlock();
}

void PositionUpdater::requestMessage (const char *pszMsgId)
{
    requestMessage (pszMsgId, NULL, NULL, NULL);
}

void PositionUpdater::requestMessage (const char *pszMsgId, const char *pszPublisherId,
                                      const char *pszSenderNodeId, DArray<uint8> *pLocallyCachedChunkIds)
{
    if (pszMsgId == NULL) {
        return;
    }
    MsgIdWrapper *pMsgIdWr = new MsgIdWrapper (pszMsgId, pszPublisherId, pszSenderNodeId, pLocallyCachedChunkIds);
    if (pMsgIdWr == NULL) {
        checkAndLogMsg ("PositionUpdater::requestMessage", memoryExhausted);
    }
    _m.lock();
    MsgIdWrapper *pOldMsgIdWr = _msgToRequest.search (pMsgIdWr);
    if (pOldMsgIdWr == NULL) {
        _msgToRequest.prepend (pMsgIdWr);
        checkAndLogMsg ("PositionUpdater::requestMessage", Logger::L_Info,
                        "requested message %s\n", pszMsgId);
        _bMessageRequested = true;
        _cv.notify();
    }
    else {
        if (pLocallyCachedChunkIds != NULL) {
            // Update the list of locally cached chunk IDs
            for (unsigned int i = 0; i < pLocallyCachedChunkIds->size(); i++) {
                pOldMsgIdWr->locallyCachedChunkIds[i] = (*pLocallyCachedChunkIds)[i];
            }
        }
        delete pMsgIdWr;
    }
    _m.unlock();
}

void PositionUpdater::removeMessageRequest (const char *pszMsgId)
{
    MsgIdWrapper msgIdWr (pszMsgId, NULL, NULL, NULL);
    _m.lock();
    MsgIdWrapper *pMsgIdWrToRemove = _msgToRequest.remove (&msgIdWr);
    if (pMsgIdWrToRemove != NULL) {
        checkAndLogMsg ("PositionUpdater::removeMessageRequest", Logger::L_Info,
                        "removed requested message %s\n", pszMsgId);
        delete pMsgIdWrToRemove;
    }
    _m.unlock();
}

void PositionUpdater::run (void)
{
    const char *pszMethodName = "PositionUpdater::run";
    setName (pszMethodName);

    started();

    BufferWriter bw;   
    do {
        _m.lock();
        if (!_bMessageRequested) {
            _cv.wait (DSPro::DEFAULT_UPDATE_TIMEOUT);
        }

        if (pTopoLog != NULL) {
            if (_pDSPro != NULL && _pDSPro->_pTopology != NULL) {
                logTopology (pszMethodName, Logger::L_Info, "\n==== TOPOLOGY ===\n");
                _pDSPro->_pTopology->display (pTopoLog->getLogFileHandle());
                logTopology (pszMethodName, Logger::L_Info, "\n=================\n");
            }
        }

        LList<MsgIdWrapper> msgToRequestCpy;
        int64 i64Now = getTimeInMilliseconds();
        for (MsgIdWrapper *pMsgIdWr = _msgToRequest.getFirst(); pMsgIdWr != NULL; pMsgIdWr = _msgToRequest.getNext()) {
            msgToRequestCpy.add (*pMsgIdWr); // copy the IDs of the message to request
            pMsgIdWr->ui64LatestRequestTime = i64Now;
        }
        _msgToRequest.removeAll();

        StringHashtable<LList<String> > *pMsgToNotify = _pMsgToNotify;
        _pMsgToNotify = new StringHashtable<LList<String> >(true, true, true, true);

        _m.unlock();

        MsgIdWrapper msgIdWr;
        for (int rc = msgToRequestCpy.getFirst (msgIdWr); rc == 1; rc = msgToRequestCpy.getNext (msgIdWr)) {
            int64 i64Elapsed = i64Now - msgIdWr.ui64LatestRequestTime; 
            if (msgIdWr.ui64LatestRequestTime == 0U || (i64Elapsed > DSPro::DEFAULT_UPDATE_TIMEOUT)) {
                Targets **ppTargets;
                if (msgIdWr.senderId.length() <= 0) {
                    ppTargets = _pDSPro->_pTopology->getNeighborsAsTargets();
                }
                else {
                    ppTargets = _pDSPro->_pTopology->getForwardingTargets (_pDSPro->getNodeId(), msgIdWr.senderId);
                }
                if ((ppTargets != NULL) && (ppTargets[0] != NULL)) {
                    int rc = 0;
                    String publisher (msgIdWr.publisherId.length() <= 0 ? _pDSPro->getNodeId() : msgIdWr.publisherId.c_str());
                    if (isOnDemandDataID (msgIdWr.msgId)) {
                        rc = _pDSPro->_adaptMgr.sendChunkRequestMessage (msgIdWr.msgId, &(msgIdWr.locallyCachedChunkIds),
                                                                         publisher, ppTargets);
                    }
                    else {
                        rc = _pDSPro->_adaptMgr.sendMessageRequestMessage (msgIdWr.msgId, publisher, ppTargets);
                    }
                    if (rc == 0) {
                        checkAndLogMsg (pszMethodName, Logger::L_Info, "Requested request "
                                        "message with id: <%s>.\n", msgIdWr.msgId.c_str());
                    }
                    else {
                        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Can not request message with "
                                        "id = <%s> failed. Returned %d\n", msgIdWr.msgId.c_str(), rc);
                    }
                }
                Targets::deallocateTargets (ppTargets);
            }
        }

        doMetadataArrived (pMsgToNotify);

        _m.lock();
        _bMessageRequested = false;
        int64 i64Tmp = _i64TimeStamp;
        _m.unlock();

        if ((getTimeInMilliseconds() - i64Tmp) > DSPro::DEFAULT_UPDATE_TIMEOUT) {

            if (_pNodeContexMgr->getActivePeerNumber() == 0) {
                continue;
            }

            bw.reset();
            int rc = _pNodeContexMgr->updatePosition (&bw);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "Could not write "
                                "the way point message. Error code %d.\n", rc);
                continue;
            }

            _m.lock();
            _i64TimeStamp = getTimeInMilliseconds();
            _m.unlock();

            _pDSPro->sendWaypointMessage (bw.getBuffer(), bw.getBufferLength());
        }

        if (_pDSPro->_bEnableTopologyExchange && _bTopologyHasChanged) {
            _bTopologyHasChanged = false;
            BufferWriter bw (1024, 1024);
            if (_pDSPro->_pTopology->write (&bw, 0) == 0) {
                Targets **ppTargets = _pDSPro->_pTopology->getNeighborsAsTargets();
                if (ppTargets != NULL && ppTargets[0] != NULL) {
                    int rc = _pDSPro->_adaptMgr.sendTopologyReplyMessage (bw.getBuffer(), bw.getBufferLength(), ppTargets);
                    if (rc != 0) {
                        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Can not send "
                                        "topology reply message. Returned %d\n", rc);
                    }
                    else {
                        checkAndLogMsg (pszMethodName, Logger::L_Info,
                                        "sent topology reply.\n");
                    }
                }
                Targets::deallocateTargets (ppTargets);
            }
        }

    } while (!terminationRequested());

    terminating();
}

void PositionUpdater::doMetadataArrived (StringHashtable<MsgIdList > *pMsgToNotifyByQueryId)
{
    if (pMsgToNotifyByQueryId == NULL) {
        return;
    }
    const char *pszMethodName = "PositionUpdater::doMetadataArrived";

    String msgId;
    StringHashtable<LList<String> >::Iterator iter = pMsgToNotifyByQueryId->getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        const char *pszQueryId = iter.getKey();
        LList<String> *pMsgToNotify = iter.getValue();

        for (int rc = pMsgToNotify->getFirst (msgId); rc == 1; rc = pMsgToNotify->getNext (msgId)) {

            MessageHeaders::MsgType type;
            Message *pMessage = getCompleteMessageAndRemoveDSProMetadata (msgId.c_str(), type);
            if (pMessage == NULL || (type != MessageHeaders::Data && type != MessageHeaders::Metadata)) {
                // The message to be notified to the application was not found at
                // this time. Re-add it to the list of messages to notify so it can
                // be tried again later.
                static const char * messageIds[2];
                messageIds[0] = msgId.c_str();
                messageIds[1] = NULL;
                addMetadataToNotify (pszQueryId, messageIds);
            }
            else {
                MessageInfo *pMI = pMessage->getMessageInfo();
                char *pszId = convertFieldToKey (pMI->getGroupName(), pMI->getPublisherNodeId(), pMI->getMsgSeqId());
                const String currMsgId (pszId);
                if (pszId == NULL) {
                    checkAndLogMsg (pszMethodName, memoryExhausted);
                }
                else {
                    free (pszId);
                    pszId = NULL;
                }
                String sGrpName (MessageIdGenerator::extractSubgroupFromMsgGroup (pMI->getGroupName()));
                if (sGrpName.length() <= 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not extract group message id\n");
                    sGrpName = pMI->getGroupName ();
                }

                if (type == MessageHeaders::Metadata) {
                    MetadataConfiguration *pMetadataConf = MetadataConfiguration::getConfiguration();
                    MetaData *pMetadata = pMetadataConf->createNewMetadataFromBuffer (pMessage->getData(), pMI->getTotalMessageLength());
                    if (pMetadata != NULL) {
                        char *pszRefersTo = NULL;
                        char *pszReferredObjectId = NULL;
                        char *pszReferredInstanceId = NULL;
                        getReferredObjectAndInstanceIds (pMetadata, pszReferredObjectId, pszReferredInstanceId, pszRefersTo);
                        int rc = _pDSPro->metadataArrived (currMsgId, sGrpName, pszReferredObjectId,
                                                           pszReferredInstanceId, pMetadata, pszRefersTo,
                                                           pszQueryId);
                        checkAndLogMsg (pszMethodName, Logger::L_Info, "notified clients "
                                        "with message %s matching query request %s\n/",
                                        currMsgId.c_str(), pszQueryId);
                        delete pMetadata;
                        pMetadata = NULL;
                        if (pszReferredObjectId != NULL) {
                            free (pszReferredObjectId);
                        }
                        if (pszReferredInstanceId != NULL) {
                            free (pszReferredInstanceId);
                        }
                        if (pszRefersTo != NULL) {
                            free (pszRefersTo);
                        }
                        if (rc != 0) {
                            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Can not notify message "
                                            "with id = <%s> failed. Returned %d\n", currMsgId.c_str(), rc);
                        }
                        else {
                            checkAndLogMsg (pszMethodName, Logger::L_Info, "client applications "
                                            "notified message with id: <%s>.\n", currMsgId.c_str());
                        }
                    }
                }
                else {
                    // Data or chunked data
                    uint8 ui8NChunks = pMI->getTotalNumberOfChunks() == 0 ? (uint8) 0 : 1; // HACK: For the general case I need to figure out the current number of chunks from the database
                    rc = _pDSPro->dataArrived (currMsgId, sGrpName, pMI->getObjectId(), pMI->getInstanceId(),
                                               pMI->getAnnotates(), pMI->getMimeType(), pMessage->getData(),
                                               pMI->getTotalMessageLength(), ui8NChunks, pMI->getTotalNumberOfChunks(),
                                               pszQueryId);
                }

                free ((void*) pMessage->getData());
                delete pMessage->getMessageHeader();
                delete pMessage;
            }
        }
    }
    
    delete pMsgToNotifyByQueryId;
}

Message * PositionUpdater::getCompleteMessageAndRemoveDSProMetadata (const char *pszMsgId, MessageHeaders::MsgType &type)
{
    Message *pMsg = _pDSPro->_pDataStore->getCompleteMessage (pszMsgId);
    if (pMsg == NULL) {
        return NULL;
    }
    const void *pData = pMsg->getData();
    MessageHeader *pMH = pMsg->getMessageHeader();
    uint32 ui32NewLen = 0;
    void *pNewData = MessageHeaders::removeDSProHeader (pMsg->getData(), pMH->getFragmentLength(), ui32NewLen, type);
    free ((void*)pData);
    if (pNewData == NULL || ui32NewLen == 0) {
        delete pMsg->getMessageHeader();
        return NULL;
    }

    pMH->setFragmentLength (ui32NewLen);
    pMH->setTotalMessageLength (ui32NewLen);
    pMsg->setData (pNewData);

    return pMsg;
}

int PositionUpdater::getReferredObjectAndInstanceIds (MetaData *pMetadata, char *&pszReferredObjectId,
                                                      char *&pszReferredInstanceId, char *&pszRefersTo)
{
    pszReferredObjectId = pszReferredInstanceId = NULL;
    pMetadata->getFieldValue (MetaData::REFERRED_DATA_OBJECT_ID, &pszReferredObjectId);
    pMetadata->getFieldValue (MetaData::REFERRED_DATA_INSTANCE_ID, &pszReferredInstanceId);
    pMetadata->getFieldValue (MetaData::REFERS_TO, &pszRefersTo);

    return 0;
}

