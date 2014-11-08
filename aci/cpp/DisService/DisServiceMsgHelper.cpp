/*
 * DisServiceMsgHelper.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "DisServiceMsgHelper.h"

#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "DisseminationService.h"
#include "Message.h"
#include "MessageInfo.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "Logger.h"
#include "NLFLib.h"
#include "PtrLList.h"
#include "StringTokenizer.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char DisServiceMsgHelper::TARGET_SEPARATOR = ';';

DisServiceMsg * DisServiceMsgHelper::clone (DisServiceMsg *pDSMsg)
{
    if (pDSMsg == NULL) {
        return NULL;
    }
    BufferWriter bw (1400, 1400);
    if (pDSMsg->write (&bw, 0) < 0) {
        return NULL;
    }
    DisServiceMsg *pCopy = getInstance (pDSMsg->getType());
    if (pCopy == NULL) {
        return NULL;
    }
    unsigned long ulLen = bw.getBufferLength();
    BufferReader br (bw.relinquishBuffer(), ulLen);
    if (pCopy->read (&br, 0) < 0) {
        return NULL;
    }
    return pCopy;
}

void DisServiceMsgHelper::deallocatedDisServiceMsg (DisServiceMsg *pDSMsg)
{
    if (pDSMsg == NULL) {
        return;
    }

    switch (pDSMsg->getType()) {
        case DisServiceMsg::DSMT_Data:
            deallocatedDisServiceDataMsg ((DisServiceDataMsg *)pDSMsg);
            break;

        case DisServiceMsg::DSMT_DataReq:
            deallocatedDisServiceDataReqMsg ((DisServiceDataReqMsg *)pDSMsg);
            break;

        default:
            delete pDSMsg;
    }
}

void DisServiceMsgHelper::deallocatedDisServiceDataMsg (DisServiceDataMsg *pDSDataMsg)
{
    if (pDSDataMsg == NULL) {
        return;
    }

    Message *pMessage = pDSDataMsg->getMessage();
    if (pMessage != NULL) {
        void *pData = (void *) pMessage->getData();
        if (pData != NULL) {
            free (pData);
        }
        delete pMessage->getMessageHeader();
        delete pMessage;
    }
    delete pDSDataMsg;
}

void DisServiceMsgHelper::deallocatedDisServiceDataReqMsg (DisServiceDataReqMsg *pDSDataReqMsg)
{
    if (pDSDataReqMsg == NULL) {
        return;
    }

    PtrLList<DisServiceDataReqMsg::FragmentRequest> *pLList = pDSDataReqMsg->getRequests();
    if (pLList != NULL) {
        DisServiceDataReqMsg::FragmentRequest *pReq, *pReqTmp;
        pReqTmp = pLList->getFirst();
        while ((pReq = pReqTmp) != NULL) {
            pReqTmp = pLList->getNext();
            pLList->remove (pReq);
            delete pReq;
        }
    }

    delete pLList;
}

DisServiceMsg * DisServiceMsgHelper::getInstance (uint8 ui8Type)
{
    DisServiceMsg *pDSMsg;

    switch (ui8Type) {
        case DisServiceMsg::DSMT_Data:
            pDSMsg = new DisServiceDataMsg();
            break;

        case DisServiceMsg::DSMT_DataReq:
            pDSMsg = new DisServiceDataReqMsg();
            break;

        case DisServiceMsg::DSMT_WorldStateSeqId:
            pDSMsg = new DisServiceWorldStateSeqIdMsg();
            break;

        case DisServiceMsg::DSMT_SubStateMessage:
            pDSMsg = new DisServiceSubscriptionStateMsg();
            break;

        case DisServiceMsg::DSMT_SubStateReq:
            pDSMsg = new DisServiceSubscriptionStateReqMsg();
            break;

        /*case DisServiceMsg::DSMT_TopologyState:
            pDSMsg = new DisServiceTopologyStateMsg();
            break;

        case DisServiceMsg::DSMT_TopologyStateReq:
            pDSMsg = new DisServiceTopologyStateReqMsg();
            break;
         */
        case DisServiceMsg::DSMT_DataCacheQuery:
            pDSMsg = new DisServiceDataCacheQueryMsg();
            break;

        case DisServiceMsg::DSMT_DataCacheQueryReply:
            pDSMsg = new DisServiceDataCacheQueryReplyMsg();
            break;

        case DisServiceMsg::DSMT_DataCacheMessagesRequest:
            pDSMsg = new DisServiceDataCacheMessagesRequestMsg();
            break;

        case DisServiceMsg::DSMT_AcknowledgmentMessage:
            pDSMsg = new DisServiceAcknowledgmentMessage();
            break;

        case DisServiceMsg::DSMT_CompleteMessageReq:
            pDSMsg = new DisServiceCompleteMessageReqMsg();
            break;

        case DisServiceMsg::DSMT_CacheEmpty:
            pDSMsg = new DisServiceCacheEmptyMsg();
            break;

        case DisServiceMsg::DSMT_CtrlToCtrlMessage:
            pDSMsg = new ControllerToControllerMsg();
            break;

        case DisServiceMsg::DSMT_HistoryReq:
            pDSMsg = new DisServiceHistoryRequest();
            break;

        case DisServiceMsg::DSMT_HistoryReqReply:
            pDSMsg = new DisServiceHistoryRequestReplyMsg();
            break;
        
        case DisServiceMsg::CRMT_QueryHits: 
            pDSMsg = new ChunkRetrievalMsgQueryHits();
            break;
        
        case DisServiceMsg::CRMT_Query:
            pDSMsg = new ChunkRetrievalMsgQuery();
            break;
            
        case DisServiceMsg::DSMT_SubAdvMessage:
            pDSMsg = new DisServiceSubscribtionAdvertisement();
            break;

        case DisServiceMsg::DSMT_SearchMsg:
            pDSMsg = new SearchMsg();
            break;

        case DisServiceMsg::DSMT_SearchMsgReply:
            pDSMsg = new SearchReplyMsg();
            break;
            
        case DisServiceMsg::DSMT_ImprovedSubStateMessage:
            pDSMsg = new DisServiceImprovedSubscriptionStateMsg();
            break;

        case DisServiceMsg::DSMT_ProbabilitiesMsg: 
            pDSMsg = new DisServiceProbabilitiesMsg();
            break;

        default:
            pDSMsg = NULL;
    }

    if ((pDSMsg != NULL) && (pDSMsg->getType() != ui8Type)) {
        uint8 ui8DSMsgType = pDSMsg->getType();
        delete pDSMsg;
        pDSMsg = NULL;
        checkAndLogMsg ("DisServiceMsgHelper::getInstance", Logger::L_SevereError,
                        "pDSMsg->getType() != ui8Type (%u != %u)\n", ui8DSMsgType, ui8Type);
    }
    return pDSMsg;
}

int DisServiceMsgHelper::getMessageType (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, uint8 &ui8MsgType)
{
    if (pMsgMetaData == NULL || ui16MsgMetaDataLen == 0) {
        return -1;
    }
    ui8MsgType = *((uint8*)pMsgMetaData);
    return 0;
}

const char * DisServiceMsgHelper::getMessageTypeAsString (uint8 ui8MsgType)
{
    switch (ui8MsgType) {
        case DisServiceMsg::DSMT_Data:
            return "DSMT_Data";

        case DisServiceMsg::DSMT_DataReq:
            return "DSMT_DataReq";

        case DisServiceMsg::DSMT_WorldStateSeqId:
            return "DSMT_WorldStateSeqId";

        case DisServiceMsg::DSMT_SubStateMessage:
            return "DSMT_SubStateMessage";

        case DisServiceMsg::DSMT_SubStateReq:
            return "DSMT_SubStateReq";

        case DisServiceMsg::DSMT_DataCacheQuery:
            return "DSMT_DataCacheQuery";

        case DisServiceMsg::DSMT_DataCacheQueryReply:
            return "DSMT_DataCacheQueryReply";

        case DisServiceMsg::DSMT_DataCacheMessagesRequest:
            return "DSMT_DataCacheMessagesRequest";

        case DisServiceMsg::DSMT_AcknowledgmentMessage:
            return "DSMT_AcknowledgmentMessage";

        case DisServiceMsg::DSMT_CompleteMessageReq:
            return "DSMT_CompleteMessageReq";

        case DisServiceMsg::DSMT_CacheEmpty:
            return "DSMT_CacheEmpty";

        case DisServiceMsg::DSMT_CtrlToCtrlMessage:
            return "DSMT_CtrlToCtrlMessage";

        case DisServiceMsg::DSMT_ChunkReq:
            return "DSMT_ChunkReq";

        case DisServiceMsg::DSMT_HistoryReq:
            return "DSMT_HistoryReq";

        case DisServiceMsg::DSMT_HistoryReqReply:
            return "DSMT_HistoryReqReply";
        
        case DisServiceMsg::CRMT_Query:
            return "CRMT_Query";
            
        case DisServiceMsg::CRMT_QueryHits: 
            return "CRMT_QueryHits";
            
        case DisServiceMsg::DSMT_SubAdvMessage:
            return "DSMT_SubAdvMessage";

        case DisServiceMsg::DSMT_SearchMsg:
            return "DSMT_SearchMsg";
            
        case DisServiceMsg::DSMT_SearchMsgReply:
            return "DSMT_SearchMsgReply";

        case DisServiceMsg::DSMT_ImprovedSubStateMessage:
            return "DSMT_ImprovedSubStateMessage";

        case DisServiceMsg::DSMT_ProbabilitiesMsg:
            return "DSMT_ProbabilitiesMsg";

        default:
            return "Unknown";
    }
}

bool DisServiceMsgHelper::isDisServiceMessage (uint8 ui8Type)
{
    return (ui8Type == DisseminationService::MPSMT_DisService);
}

String DisServiceMsgHelper::getMultiNodeTarget (StringHashset &targets)
{
    if (targets.getCount() == 0) {
        String emptyString;
        return emptyString;
    }
    StringHashset::Iterator iter = targets.getAllElements();
    String target;
    for (bool bIsFirst = true; !iter.end(); iter.nextElement()) {
        if (bIsFirst) {
            bIsFirst = false;
        }
        else {
            target += TARGET_SEPARATOR;
        }
        target += iter.getKey();
    }
    return target;
}

bool DisServiceMsgHelper::messageTypeExists (uint8 ui8MsgType)
{
    switch (ui8MsgType) {
        case DisServiceMsg::DSMT_Data:
        case DisServiceMsg::DSMT_DataReq:
        case DisServiceMsg::DSMT_WorldStateSeqId:
        case DisServiceMsg::DSMT_SubStateMessage:
        case DisServiceMsg::DSMT_SubStateReq:
        case DisServiceMsg::DSMT_DataCacheQuery:
        case DisServiceMsg::DSMT_DataCacheQueryReply:
        case DisServiceMsg::DSMT_DataCacheMessagesRequest:
        case DisServiceMsg::DSMT_AcknowledgmentMessage:
        case DisServiceMsg::DSMT_CompleteMessageReq:
        case DisServiceMsg::DSMT_CacheEmpty:
        case DisServiceMsg::DSMT_CtrlToCtrlMessage:
        case DisServiceMsg::DSMT_ChunkReq:
        case DisServiceMsg::DSMT_HistoryReq:
        case DisServiceMsg::DSMT_HistoryReqReply:
        case DisServiceMsg::CRMT_Query:
        case DisServiceMsg::CRMT_QueryHits: 
        case DisServiceMsg::DSMT_SubAdvMessage:
        case DisServiceMsg::DSMT_SearchMsg:            
        case DisServiceMsg::DSMT_SearchMsgReply:
        case DisServiceMsg::DSMT_ImprovedSubStateMessage:
        case DisServiceMsg::DSMT_ProbabilitiesMsg:
            return true;

        default:
            return false;
    }
}

bool DisServiceMsgHelper::isTarget (const char *pszNodeId, DisServiceMsg *pDSMsg, bool &bTargetSpecified)
{
    bTargetSpecified = false;
    if (pDSMsg == NULL) {
        return false;
    }
    const char *pszTarget = pDSMsg->getTargetNodeId();
    if (pszTarget == NULL || strlen (pszTarget) == 0) {
        // If the target is NULL, it means that the message is directed to any
        // peer, therefore pszNodeId is target 
        return true;
    }
    bTargetSpecified = true;
    StringTokenizer tokenizer;
    tokenizer.init (pszTarget, TARGET_SEPARATOR, TARGET_SEPARATOR);
    for (const char *pszTarget; (pszTarget = tokenizer.getNextToken()) != NULL;) {
        if (strcmp (pszTarget, pszNodeId) == 0) {
            return true;
        }
    }
    return false;
}

bool DisServiceMsgHelper::isInSession (const char *pszSessionId, DisServiceMsg *pDSMsg)
{
    if (pDSMsg == NULL) {
        return false;
    }
    return isInSession (pszSessionId, pDSMsg->getSessionId());
}

bool DisServiceMsgHelper::isInSession (const char *pszSessionId1, const char *pszSessionId2)
{
    if ((pszSessionId1 == NULL) || (strlen (pszSessionId1) == 0)) {
        if ((pszSessionId2 == NULL) || (strlen (pszSessionId2) == 0)) {
            return true;
        }
        return false;
    }
    if ((pszSessionId2 == NULL) || (strlen (pszSessionId2) == 0)) {
        return false;
    }
    return (strcmp (pszSessionId1, pszSessionId2) == 0);
}

bool DisServiceMsgHelper::sentBy (const char *pszNodeId, DisServiceMsg *pDSMsg)
{
    if (pDSMsg == NULL) {
        return false;
    } 
    return strNotNullAndEqual (pszNodeId, pDSMsg->getSenderNodeId());
}

//------------------------------------------------------------------------------
// DisServiceDataMsgFragmenter
//------------------------------------------------------------------------------

DisServiceDataMsgFragmenter::DisServiceDataMsgFragmenter (const char *pszNodeId)
    : _nodeId (pszNodeId)
{
    _pFragBuf = NULL;
    _ui16CurrFragBufLen = 0;
    _pMsgFragment = new Message();
    _pMH = NULL;
}

DisServiceDataMsgFragmenter::~DisServiceDataMsgFragmenter (void)
{
    if (_pFragBuf != NULL) {
        free (_pFragBuf);
        _pFragBuf = NULL;
    }
    if (_pMH != NULL) {
        delete _pMH;
        _pMH = NULL;
    }
    if (_pMsgFragment != NULL) {
        delete _pMsgFragment;
        _pMsgFragment = NULL;
    }
}

void DisServiceDataMsgFragmenter::init (DisServiceDataMsg *pDSDataMsg, uint16 ui16FragSize, uint32 ui32HeaderSize)
{
    if (pDSDataMsg == NULL) {
        return;
    }

    _pDSDataMsg = pDSDataMsg;
    if (_pMH != NULL) {
        delete _pMH;
    }
    _pMH = pDSDataMsg->getMessageHeader()->clone();
    _ui16FragSize = ui16FragSize;
    _ui32HeaderSize = ui32HeaderSize;

    // NB: if it's a complete message pMI->getFragmentLength() should be equal to
    // getTotalMessageLength() if it is a fragment, the fragment length is needed.
    _ui32CurrentOffset = _pMH->getFragmentOffset();
    _ui32StartOffset = _pMH->getFragmentOffset();
    _ui32StopOffset = _pMH->getFragmentOffset() + _pMH->getFragmentLength();
}

DisServiceDataMsg * DisServiceDataMsgFragmenter::getNextFragment (void)
{
    const void *pData = _pDSDataMsg->getMessage()->getData();
    if (pData == NULL) {
        return NULL;
    }

    const uint16 ui16DataFragmentLen = _ui16FragSize - _ui32HeaderSize;
    if (ui16DataFragmentLen > _ui16CurrFragBufLen) {
        if (_pFragBuf != NULL) {
            free (_pFragBuf);
            _ui16CurrFragBufLen = 0;
        }
        _pFragBuf = malloc (ui16DataFragmentLen);
        if (_pFragBuf == NULL) {
            return NULL;
        }
        _ui16CurrFragBufLen = ui16DataFragmentLen;
    }

    _pMsgFragment->setMessageHeader (_pMH);
    _pMsgFragment->setData (_pFragBuf);

    DisServiceDataMsg *pFragDataMsg = new DisServiceDataMsg (_nodeId, _pMsgFragment);
    if (pFragDataMsg == NULL) {
        return NULL;
    }
    pFragDataMsg->setSendingCompleteMsg (_pMH->isCompleteMessage());
    pFragDataMsg->setTargetNodeId (_pDSDataMsg->getTargetNodeId());
    pFragDataMsg->setRepair (_pDSDataMsg->isRepair());
    pFragDataMsg->setDoNotForward (_pDSDataMsg->doNotForward());

    if (_ui32CurrentOffset < _ui32StopOffset) {
        uint32 ui32FragmentLength = _ui32StopOffset - _ui32CurrentOffset;
        if (ui32FragmentLength > (ui16DataFragmentLen)) {
            ui32FragmentLength = (ui16DataFragmentLen);
        }
        _pMsgFragment->getMessageHeader()->setFragmentOffset (_ui32CurrentOffset);
        _pMsgFragment->getMessageHeader()->setFragmentLength (ui32FragmentLength);
        uint32 ui32TmpCurrentOffset = _ui32CurrentOffset - _ui32StartOffset;
        memcpy (_pFragBuf, ((char*)pData)+ui32TmpCurrentOffset, ui32FragmentLength);
        _ui32CurrentOffset += ui32FragmentLength;

        return pFragDataMsg;
    }

    return NULL;
}

