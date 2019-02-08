/*
 * DisServiceMsg.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "DisServiceMsg.h"

#include "DisServiceDataCacheQuery.h"
#include "DisServiceDefs.h"
#include "DisServiceMsgHelper.h"
#include "Message.h"
#include "MessageInfo.h"
#include "MessageRequestScheduler.h"
#include "Subscription.h"
#include "RangeDLList.h"
#include "SessionId.h"

#include "BufferReader.h"
#include "InstrumentedWriter.h"
#include "Logger.h"
#include "NLFLib.h"
#include "PtrQueue.h"

#include <stdio.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//==============================================================================
//  DisServiceMsg
//==============================================================================

DisServiceMsg::DisServiceMsg (Type type)
{
    _type = type;
    _senderNodeId = "";
    _ui16Size = 0;
    _targetNodeId = "";
}

DisServiceMsg::DisServiceMsg (Type type, const char *pszSenderNodeId)
{
    _type = type;
    _senderNodeId = pszSenderNodeId;
    _ui16Size = 0;
    _targetNodeId = "";
}

DisServiceMsg::DisServiceMsg (Type type, const char *pszSenderNodeId,
                              const char *pszTargetNodeId)
{
    _type = type;
    _senderNodeId = pszSenderNodeId;
    _ui16Size = 0;
    if (pszTargetNodeId != NULL) {
        _targetNodeId = pszTargetNodeId;
    }
    _sessionId = "";
}

DisServiceMsg::~DisServiceMsg()
{
}

DisServiceMsg::Type DisServiceMsg::getType() const
{
    return _type;
}

const char * DisServiceMsg::getSenderNodeId() const
{
    return _senderNodeId.c_str();
}

const char * DisServiceMsg::getTargetNodeId() const
{
    return _targetNodeId.c_str();
}

const char * DisServiceMsg::getSessionId() const
{
    return _sessionId.c_str();
}

uint16 DisServiceMsg::getSize() const
{
    return _ui16Size;
}

void DisServiceMsg::flush()
{
    _ui16Size = 0;
}

void DisServiceMsg::setSenderNodeId (const char *pszId)
{
    if (pszId) {
        _senderNodeId = pszId;
    }
}

void DisServiceMsg::setTargetNodeId (const char *pszTargetNodeId)
{
    if (pszTargetNodeId) {
        _targetNodeId = pszTargetNodeId;
    }
}

void DisServiceMsg::setSessionId (const char *pszSessionId)
{
    if (pszSessionId) {
        _sessionId = pszSessionId;
    }
}

int DisServiceMsg::display (FILE *pFileOut)
{
    if (pFileOut == NULL) {
        return -1;
    }
    fprintf (pFileOut, "******** DisServiceMsg ********\n");
    fprintf (pFileOut, "sender: %s\ttarget: %s\ttype = %d (",
             (_senderNodeId.length() > 0 ? (const char *)_senderNodeId : "NULL"),
             (_targetNodeId.length() > 0 ? (const char *)_targetNodeId : "NULL"),
             (int) _type);
    fprintf (pFileOut, "%s", DisServiceMsgHelper::getMessageTypeAsString (_type));
    fprintf (pFileOut, ")\n");
    return 0;
}

int DisServiceMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }
    uint8 ui8;
    if (pReader->read8 (&ui8) < 0) {
        return -2;
    }
    if (!DisServiceMsgHelper::messageTypeExists (ui8)) {
        return -3;
    }
    _type = (Type) ui8;

    char buf[0xFF];

    // Read _targetNodeId
    if ((pReader->read8 (&ui8) < 0) || (ui8 > (0xFF -1))) {
        return -4;
    }
    if (ui8 > 0) {
        if (pReader->readBytes (buf, ui8) < 0) {
            return -5;
        }
        buf[ui8] = '\0';
        _targetNodeId = buf;   // String makes a copy of buf
    }

    // Read _senderNodeId
    if ((pReader->read8 (&ui8) < 0) || (ui8 > (0xFF -1))) {
        return -6;
    }
    if (ui8 > 0) {
        if (pReader->readBytes (buf, ui8) < 0) {
            return -7;
        }
        buf[ui8] = '\0';
        _senderNodeId = buf;   // String makes a copy of buf
    }

    // Read _sessionId
    if ((pReader->read8 (&ui8) < 0) || (ui8 > (0xFF -1))) {
        return -8;
    }
    if (ui8 > 0) {
        if (pReader->readBytes (buf, ui8) < 0) {
            return -9;
        }
        buf[ui8] = '\0';
        _sessionId = buf;   // String makes a copy of buf
    }

    return 0;
}

int DisServiceMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return 0;
    }
    uint8 ui8 = _type;
    if (pWriter->write8 (&ui8) < 0) {
        return -1;
    }

    // Write _targetNodeId
    int iLen = (_targetNodeId.length() > 0 ? _targetNodeId.length() : 0);
    if (iLen > (0xFF -1)) {
        return -2;
    }
    ui8 = (uint8) iLen;
    if (pWriter->write8 (&ui8) < 0) {
        return -3;
    }
    if (ui8 > 0) {
        if (pWriter->writeBytes (_targetNodeId.c_str(), ui8) < 0) {
            return -4;
        }
    }

    // Write _senderNodeId
    iLen = (_senderNodeId.length() > 0 ? _senderNodeId.length() : 0);
    if (iLen > (0xFF -1)) {
        return -5;
    }
    ui8 = (uint8) iLen;
    if (pWriter->write8 (&ui8) < 0) {
        return -6;
    }
    if (ui8 > 0) {
        if (pWriter->writeBytes (_senderNodeId.c_str(), ui8) < 0) {
            return -7;
        }
    }

    // Write _sessionId
    iLen = (_sessionId.length() > 0 ? _sessionId.length() : 0);
    if (iLen > (0xFF -1)) {
        return -8;
    }
    ui8 = (uint8) iLen;
    if (pWriter->write8 (&ui8) < 0) {
        return -9;
    }
    if (ui8 > 0) {
        if (pWriter->writeBytes (_sessionId.c_str(), ui8) < 0) {
            return -10;
        }
    }

    return 0;
}

DisServiceMsg::Range::Range (uint32 ui32From, uint32 ui32To)
    : from (ui32From), to (ui32To)
{
}

DisServiceMsg::Range::~Range()
{
}

bool DisServiceMsg::Range::operator == (const Range &range) const
{
    if ((from == range.getFrom()) && (to == range.getTo())) {
        return true;
    }
    return false;
}

bool DisServiceMsg::Range::operator > (const Range &range) const
{
    if ((from == range.getFrom()) && (to == range.getTo())) {
        return false;
    }
    // Return true of false randomly
    return ((((rand() % 100) + 1.0f) < 50.0f));
}

bool DisServiceMsg::Range::operator < (const Range &range) const
{
    if ((from == range.getFrom()) && (to == range.getTo())) {
        return false;
    }
    // Return true of false randomly
    return ((((rand() % 100) + 1.0f) < 50.0f));
}

uint32 DisServiceMsg::Range::getFrom (void) const
{
   return from;
}

uint32 DisServiceMsg::Range::getTo (void) const
{
   return to;
}

//==============================================================================
// DisServiceCtrlMsg CONTROL
//==============================================================================

Mutex DisServiceCtrlMsg::_mCtrlMsgSeqNo;
uint32 DisServiceCtrlMsg::_ui32NextCtrlMsgSeqNo;

DisServiceCtrlMsg::DisServiceCtrlMsg (Type type)
    : DisServiceMsg (type)
{
    _ui32CtrlMsgSeqNo = allocateNextSeqNo();
}

DisServiceCtrlMsg::DisServiceCtrlMsg (Type type, const char *pszSenderNodeId)
    : DisServiceMsg (type, pszSenderNodeId)
{
    _ui32CtrlMsgSeqNo = allocateNextSeqNo();
}

DisServiceCtrlMsg::DisServiceCtrlMsg (Type type, const char *pszSenderNodeId, const char *pszTargetNodeId)
    : DisServiceMsg (type, pszSenderNodeId, pszTargetNodeId)
{
    _ui32CtrlMsgSeqNo = allocateNextSeqNo();
}

uint32 DisServiceCtrlMsg::allocateNextSeqNo (void)
{
    _mCtrlMsgSeqNo.lock();
    uint32 ui32NewSeqNo = _ui32NextCtrlMsgSeqNo++;
    _mCtrlMsgSeqNo.unlock();
    return ui32NewSeqNo;
}

uint32 DisServiceCtrlMsg::getCtrlMsgSeqNo (void)
{
    return _ui32CtrlMsgSeqNo;
}


int DisServiceCtrlMsg::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    // Read the CtrlMsgSeqNo
    pReader->read32 (&_ui32CtrlMsgSeqNo);
    return 0;
}

int DisServiceCtrlMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (DisServiceMsg::write (pWriter, ui32MaxSize) != 0) {
        return -1;
    }
    // Write the CtrlMsgSeqNo
    pWriter->write32 (&_ui32CtrlMsgSeqNo);
    return 0;
}

//==============================================================================
//  DisServiceDataMsg
//==============================================================================
DisServiceDataMsg::DisServiceDataMsg()
    : DisServiceMsg (DSMT_Data)
{
    _pMsg = NULL;
    _type = DSMT_Data;
    _bChunk = false;
    _bSendingComplete = false;
    _hasStats = false;
    _bIsRepair = false;
    _bHasRateEstimate = false;
    _bHasSendRate = false;
    _bDoNotForward = false;
}

DisServiceDataMsg::DisServiceDataMsg (const char *pszSenderNodeId, Message *pMsg)
    : DisServiceMsg (DSMT_Data, pszSenderNodeId)
{
    _pMsg = pMsg;
    _bChunk = false;
    _bSendingComplete = false;
    _hasStats = false;
    _bIsRepair = false;
    _bHasRateEstimate = false;
    _bHasSendRate = false;
    _bDoNotForward = false;
}

DisServiceDataMsg::DisServiceDataMsg (const char *pszSenderNodeId, Message *pMsg,
                                      const char *pszTargetNodeId)
    : DisServiceMsg (DSMT_Data, pszSenderNodeId, pszTargetNodeId)
{
    _pMsg = pMsg;
    _bChunk = false;
    _bSendingComplete = false;
    _hasStats = false;
    _bIsRepair = false;
    _bHasRateEstimate = false;
    _bHasSendRate = false;
    _bDoNotForward = false;
}

DisServiceDataMsg::~DisServiceDataMsg (void)
{
    _pMsg = NULL;
}

void DisServiceDataMsg::setMessage (Message *pMsg)
{
    _pMsg = pMsg;
}

MessageHeader * DisServiceDataMsg::getMessageHeader()
{
    if (_pMsg != NULL) {
        return _pMsg->getMessageHeader();
    }
    return NULL;
}

const void * DisServiceDataMsg::getPayLoad()
{
    if (_pMsg != NULL) {
        return _pMsg->getData();
    }
    return NULL;
}

Message * DisServiceDataMsg::getMessage()
{
    return _pMsg;
}

bool DisServiceDataMsg::isChunk() const
{
    return _bChunk;
}

bool DisServiceDataMsg::getSendingCompleteMsg() const
{
    return _bSendingComplete;
}

void DisServiceDataMsg::setSendingCompleteMsg (bool bSendingCompleteMsg)
{
    _bSendingComplete = bSendingCompleteMsg;
}

bool DisServiceDataMsg::isRepair() const
{
    return _bIsRepair;
}

void DisServiceDataMsg::setRepair (bool bIsRepair)
{
    _bIsRepair = bIsRepair;
}

bool DisServiceDataMsg::hasSendRate() const
{
    return _bHasSendRate;
}

bool DisServiceDataMsg::hasRateEstimate() const
{
    return _bHasRateEstimate;
}

void DisServiceDataMsg::setSendRate (uint32 ui32SendRate)
{
    _ui32RateEstimationInfo = ui32SendRate;
    _bHasSendRate = true;
    _bHasRateEstimate = false;
}

void DisServiceDataMsg::setRateEstimate (uint32 ui32RateEstimate)
{
    _ui32RateEstimationInfo = ui32RateEstimate;
    _bHasSendRate = false;
    _bHasRateEstimate = true;
}

uint32 DisServiceDataMsg::getRateEstimationInfo() const
{
    return _ui32RateEstimationInfo;
}

void DisServiceDataMsg::setDoNotForward (bool bDoNotForward)
{
    _bDoNotForward = bDoNotForward;
}

bool DisServiceDataMsg::doNotForward (void) const
{
    return _bDoNotForward;
}

int DisServiceDataMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != DSMT_Data) {
        return -2;
    }

    // Read binary flags
    uint8 ui8ReadByte;
    uint32 ui32ReadUI32;
    pReader->read8 (&ui8ReadByte);
    _bChunk = ((ui8ReadByte & IS_CHUNK) != 0);
    _bSendingComplete = ((ui8ReadByte & SENDING_COMPL_MSG) != 0);
    _hasStats = ((ui8ReadByte & HAS_STATS) != 0);
    _bIsRepair = ((ui8ReadByte & IS_REPAIR) != 0);
    _bHasSendRate = ((ui8ReadByte & HAS_SEND_RATE) != 0);
    _bHasRateEstimate = ((ui8ReadByte & HAS_RATE_ESTIMATE) != 0);
    _bDoNotForward = ((ui8ReadByte & DO_NOT_FORWARD) != 0);

    if (_bHasSendRate || _bHasRateEstimate) {
        pReader->read32 (&ui32ReadUI32);
        _ui32RateEstimationInfo = ui32ReadUI32;
    }
    else {
        _ui32RateEstimationInfo = 0;
    }

    // Read the MessageHeader. Since the type of the MessageHeader is unknown, we need to use the flag
    MessageHeader *pMH;
    if (_bChunk) {
        pMH = new ChunkMsgInfo();
    }
    else {
        pMH = new MessageInfo();
    }
    pMH->read (pReader, ui32MaxSize);
    void *pData = malloc (pMH->getFragmentLength());
    pReader->readBytes (pData, pMH->getFragmentLength());
    _pMsg = new Message (pMH, pData);

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceDataMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (_pMsg == NULL || _pMsg->getMessageHeader() == NULL) {
        return -1;
    }
    if (DisServiceMsg::write (&iw, ui32MaxSize) != 0) {
        return -2;
    }
    MessageHeader *pMH = getMessageHeader();
    if (pMH == NULL) {
        return -1;
    }

    // Write Binary Flags
    uint8 ui8ByteToWrite = 0x00;
    if (pMH->isChunk()) {
        ui8ByteToWrite |= IS_CHUNK;
    }
    if (_bSendingComplete) {
        ui8ByteToWrite |= SENDING_COMPL_MSG;
    }
    if (_hasStats) {
        ui8ByteToWrite |= HAS_STATS;
    }
    if (_bIsRepair) {
        ui8ByteToWrite |= IS_REPAIR;
    }
    if (_bHasSendRate) {
        ui8ByteToWrite |= HAS_SEND_RATE;
    }
    if (_bHasRateEstimate) {
        ui8ByteToWrite |= HAS_RATE_ESTIMATE;
    }
    if (_bDoNotForward) {
        ui8ByteToWrite |= DO_NOT_FORWARD;
    }
    pWriter->write8 (&ui8ByteToWrite);
    //write queue length
    if (_bHasRateEstimate || _bHasSendRate) {
        pWriter->write32 (&_ui32RateEstimationInfo);
    }

    // Write the Message Header
    pMH->write (&iw, ui32MaxSize);
    if ((iw.getBytesWritten() > ui32MaxSize) && (ui32MaxSize != 0)) {
        return 1;
    }
    if (_pMsg->getData() != NULL) {
        iw.writeBytes (_pMsg->getData(), pMH->getFragmentLength());
    }

    _ui16Size = iw.getBytesWritten();
    return 0;
}

//==============================================================================
//  DisServiceSessionSyncMsg
//==============================================================================
DisServiceSessionSyncMsg::DisServiceSessionSyncMsg (void)
    : DisServiceCtrlMsg (DSMT_SessionSync)
{
    _sync.i64Timestamp = 0;
}

DisServiceSessionSyncMsg::DisServiceSessionSyncMsg (const char *pszSenderNodeId)
    : DisServiceCtrlMsg (DSMT_SessionSync, pszSenderNodeId)
{
    int64 i64Timestamp = 0;
    String sessionId (SessionId::getInstance()->getSessionIdAndTimestamp (i64Timestamp));

    _sync._sessionId = sessionId;
    _sync.i64Timestamp = i64Timestamp;
}

DisServiceSessionSyncMsg::~DisServiceSessionSyncMsg (void)
{
}

String DisServiceSessionSyncMsg::getSessionSync (int64 &i64Timestamp)
{
    i64Timestamp = _sync.i64Timestamp;
    return _sync._sessionId;
}

int DisServiceSessionSyncMsg::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    char *pszSession = NULL;
    if (pReader->readString (&pszSession) < 0) {
        return -2;
    }
    int64 i64Timestamp = 0;
    if (pReader->read64 (&i64Timestamp) < 0) {
        return -3;
    }

    _sync._sessionId = pszSession;
    _sync.i64Timestamp = i64Timestamp;

    return 0;
}

int DisServiceSessionSyncMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    _sessionId = "*";
    if (DisServiceCtrlMsg::write (pWriter, ui32MaxSize) != 0) {
        return -1;
    }

    if (pWriter->writeString (_sync._sessionId) < 0) {
        return -2;
    }
    if (pWriter->write64 (&_sync.i64Timestamp) < 0) {
        return -3;
    }

    return 0;
}

//==============================================================================
//  DisServiceDataReqMsg
//==============================================================================
const uint8 DisServiceDataReqMsg::HAS_NEXT = 1;
const uint8 DisServiceDataReqMsg::DOES_NOT_HAVE_NEXT = 0;

DisServiceDataReqMsg::DisServiceDataReqMsg()
    : DisServiceCtrlMsg (DSMT_DataReq),
      _ui16NumberOfNeighbors (0),
      _pFragmentRequests (NULL),
      _pCompleteMessageRequests (NULL),
      _i64SendingTime (getTimeInMilliseconds())
{
}

DisServiceDataReqMsg::DisServiceDataReqMsg (const char *pszSenderNodeId, const char *pszQueryTargetNodeId,
                                            uint16 ui16NumberOfNeighbors,
                                            PtrLList<FragmentRequest> *pMessageRequests,
                                            PtrLList<FragmentRequest> *pFragmentRequests)
    : DisServiceCtrlMsg (DSMT_DataReq, pszSenderNodeId),
      _ui16NumberOfNeighbors (ui16NumberOfNeighbors),
      _pFragmentRequests (pFragmentRequests),
      _pCompleteMessageRequests (pMessageRequests),
      _i64SendingTime (getTimeInMilliseconds()),
      _queryTargetNodeId (pszQueryTargetNodeId)
{
}

DisServiceDataReqMsg::~DisServiceDataReqMsg()
{
    _pFragmentRequests = NULL;
    _pCompleteMessageRequests = NULL;
}

int DisServiceDataReqMsg::displayCompleteMessageReq ()
{
    checkAndLogMsg ("DisServiceDataReqMsg::display", Logger::L_Info,
                    "**************** DisServiceDataReqMsg *****************\n");
    FragmentRequest *pFragRequest;
    Range *pRange;
    if (_pCompleteMessageRequests != NULL) {
        pFragRequest = _pCompleteMessageRequests->getFirst();
        while (pFragRequest != NULL) {
            if (pFragRequest->pMsgHeader->isChunk()) {
                checkAndLogMsg ("DisServiceDataReqMsg::display", Logger::L_Info,
                                "Group = <%s>; Sender = <%s>; Msg Seq Id = <%u>; ChunkId = <%u>; Msg Size = <%u>\n",
                                pFragRequest->pMsgHeader->getGroupName(),
                                pFragRequest->pMsgHeader->getPublisherNodeId(),
                                pFragRequest->pMsgHeader->getMsgSeqId(),
                                ((ChunkMsgInfo*)pFragRequest->pMsgHeader)->getChunkId(),
                                pFragRequest->pMsgHeader->getTotalMessageLength());
            }
            else {
                checkAndLogMsg ("DisServiceDataReqMsg::display", Logger::L_Info,
                                "Group = <%s>; Sender = <%s>; Msg Seq Id = <%u>; Msg Size = <%u>\n",
                                pFragRequest->pMsgHeader->getGroupName(),
                                pFragRequest->pMsgHeader->getPublisherNodeId(),
                                pFragRequest->pMsgHeader->getMsgSeqId(),
                                pFragRequest->pMsgHeader->getTotalMessageLength());
            }

            pRange = pFragRequest->pRequestedRanges->getFirst();
            while (pRange != NULL) {
                checkAndLogMsg ("DisServiceDataReqMsg::display", Logger::L_Info,
                                "%u-%u ", pRange->getFrom(), pRange->getTo());
                pRange = pFragRequest->pRequestedRanges->getNext();
            }
            if (pFragRequest->bRequestMessageTail) {
                checkAndLogMsg ("DisServiceDataReqMsg::display", Logger::L_Info,
                                "%u-%u\n", pFragRequest->ui32NextExpectedOffset,
                                pFragRequest->ui32TotalMessageLength);
            }
            else {
                checkAndLogMsg ("DisServiceDataReqMsg::display", Logger::L_Info, "\n");
            }

            pFragRequest = _pCompleteMessageRequests->getNext();
        }
    }

    return 0;
}

int DisServiceDataReqMsg::display (FILE *pFileOut)
{
    if (pFileOut == NULL) {
        return -1;
    }
    fprintf (pFileOut, "\n\n************* DisServiceDataReqMsg *************\n\n");
    DisServiceCtrlMsg::display (pFileOut);
    FragmentRequest *pFragRequest;
    if (_pCompleteMessageRequests != NULL) {
        pFragRequest = _pCompleteMessageRequests->getFirst();
        while (pFragRequest != NULL) {
            display (pFragRequest, pFileOut);
            pFragRequest = _pCompleteMessageRequests->getNext();
        }
    }
    if (_pFragmentRequests != NULL) {
        pFragRequest = _pFragmentRequests->getFirst();
        while (pFragRequest != NULL) {
            display (pFragRequest, pFileOut);
            pFragRequest = _pFragmentRequests->getNext();
        }
    }
    return 0;
}

int DisServiceDataReqMsg::display (FragmentRequest *pFragRequest, FILE *pFileOut)
{
    if (pFragRequest == NULL) {
        return -1;
    }

    String ranges ("\tMissing Ranges: ");
    for (Range *pRange = pFragRequest->pRequestedRanges->getFirst(); pRange != NULL;
            pRange = pFragRequest->pRequestedRanges->getNext()) {
        ranges += pRange->getFrom();
        ranges += "-";
        ranges += pRange->getTo();
        ranges += " ";
    }
    if (pFragRequest->bRequestMessageTail) {
        ranges += pFragRequest->ui32NextExpectedOffset;
        ranges += "-";
        ranges += pFragRequest->ui32TotalMessageLength;
        ranges += " ";
    }

    if (pFragRequest->pMsgHeader->isChunk()) {
        fprintf (pFileOut, "Requesting ranges for message Group = <%s>; Sender = <%s>; Msg Seq Id = <%u>; ChunkId = <%u>; Msg Size = <%u>.%s\n",
                           pFragRequest->pMsgHeader->getGroupName(),
                           pFragRequest->pMsgHeader->getPublisherNodeId(),
                           pFragRequest->pMsgHeader->getMsgSeqId(),
                           ((ChunkMsgInfo*)pFragRequest->pMsgHeader)->getChunkId(),
                           pFragRequest->pMsgHeader->getTotalMessageLength(), ranges.c_str());
    }
    else {
        fprintf (pFileOut, "Group = <%s>; Sender = <%s>; Msg Seq Id = <%u>; Msg Size = <%u>.%s\n",
                           pFragRequest->pMsgHeader->getGroupName(),
                           pFragRequest->pMsgHeader->getPublisherNodeId(),
                           pFragRequest->pMsgHeader->getMsgSeqId(),
                           pFragRequest->pMsgHeader->getTotalMessageLength(), ranges.c_str());
    }

    return 0;
}

void DisServiceDataReqMsg::setFragmentRequests (PtrLList<FragmentRequest> *pFragmentedRequests)
{
    _pFragmentRequests = pFragmentedRequests;
}

int DisServiceDataReqMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_DataReq) {
        return -2;
    }

    // read the number of neighboring peers
    if (pReader->read16 (&_ui16NumberOfNeighbors) < 0) {
        return -3;
    }

    if (_pFragmentRequests == NULL) {
        _pFragmentRequests = new PtrLList<DisServiceDataReqMsg::FragmentRequest>();
    }

    // Read elements
    uint8 ui8HasNext;
    for (int rc = pReader->read8 (&ui8HasNext); (ui8HasNext == 1) && (rc >= 0); rc = pReader->read8 (&ui8HasNext)) {
        PtrLList<Range> *pLL = new PtrLList<Range>();
        uint16 ui16;
        if (pReader->read16 (&ui16) < 0) {
            return -4;
        }
        char *pszGroupName = new char [ui16 + 1];
        if (pReader->readBytes (pszGroupName, ui16) < 0) {
            return -5;
        }
        pszGroupName[ui16] = '\0';
        if (pReader->read16 (&ui16) < 0) {
            return -6;
        }
        char *pszSenderNodeId = new char [ui16 + 1];
        if (pReader->readBytes (pszSenderNodeId, ui16) < 0) {
            return -7;
        }
        pszSenderNodeId[ui16] = '\0';
        uint32 ui32MsgSeqId;
        if (pReader->read32 (&ui32MsgSeqId) < 0) {
            return -8;
        }

        if (pReader->read16 (&ui16) < 0) {
            return -9;
        }
        char *pszObjectId = NULL;
        if (ui16 > 0) {
            pszObjectId = new char [ui16 + 1];
            if (pReader->readBytes (pszObjectId, ui16) < 0) {
                return -10;
            }
            pszObjectId[ui16] = '\0';
        }

        if (pReader->read16 (&ui16) < 0) {
            return -12;
        }
        char *pszInstanceId = NULL;
        if (ui16 > 0) {
            pszInstanceId = new char [ui16 + 1];
            if (pReader->readBytes (pszInstanceId, ui16) < 0) {
                return -13;
            }
            pszInstanceId[ui16] = '\0';
        }

        uint16 ui16Tag;
        if (pReader->read16 (&ui16Tag) < 0) {  // read the tag
            return -14;
        }
        uint32 ui32TotMsgLen = 0U;
        if (pReader->read32 (&ui32TotMsgLen) < 0) { // read the TotalMessageLengtht
            return -15;
        }
        uint8 ui8ChunkId;
        if (pReader->read8 (&ui8ChunkId) < 0) {
            return -16;
        }

        MessageHeader *pMH = NULL;
        if (ui8ChunkId == MessageHeader::UNDEFINED_CHUNK_ID) {
            pMH = new MessageInfo (pszGroupName, pszSenderNodeId, ui32MsgSeqId, pszObjectId, pszInstanceId, ui16Tag,
                                   0, // ui16ClientId - it's not actually used in this context
                                   0, // ui16ClientType - it's not actually used in this context
                                   NULL, // MIME Type - it's not actually used in this context
                                   NULL, // checksum - it's not actually used in this context
                                   ui32TotMsgLen, // it's not actually used in this context
                                   ui32TotMsgLen, // ui32FragmentLength - it's not actually used in this context
                                   0);            // ui32FragmentOffest - it's not actually used in this context
        }
        else {
            // Read also the ChunkId
            uint8 ui8TotNChunks;
            if (pReader->read8 (&ui8TotNChunks) < 0) {
                return -17;
            }
            pMH = new ChunkMsgInfo (pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui8ChunkId,
                                    pszObjectId, pszInstanceId, ui16Tag,
                                    0, // ui16ClientId - it's not actually used in this context
                                    0, // ui16ClientId - it's not actually used in this context
                                    NULL, // MIME Type - it's not actually used in this context
                                    NULL, // checksum - it's not actually used in this context
                                    0, // ui32FragmentOffset - it's not actually used in this context
                                    0, // ui32FragmentLength - it's not actually used in this context
                                    ui32TotMsgLen, // it's not actually used in this context
                                    ui8TotNChunks);// it's not actually used in this context
        }

        for (pReader->read8 (&ui8HasNext); ui8HasNext == 1; pReader->read8 (&ui8HasNext)) {
            uint32 ui32From;
            uint16 ui16Length;
            if (pReader->read32 (&ui32From) < 0) {
                return -18;
            }
            if (pReader->read16 (&ui16Length) < 0) {
                return -19;
            }
            pLL->insert (new Range (ui32From, ui32From + ui16Length));
        }
        FragmentRequest *ptmp = new FragmentRequest (pMH, pLL);
        if (ptmp != NULL) {
            _pFragmentRequests->insert (ptmp);
        }

        delete[] pszGroupName;
        pszGroupName = NULL;
        delete[] pszSenderNodeId;
        pszSenderNodeId = NULL;
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceDataReqMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -2;
    }

    // write the number of neighboring peers
    iw.write16 (&_ui16NumberOfNeighbors);

    // Write elements
    bool bBufferFull = false;
    writeRequests (_pCompleteMessageRequests, &iw, ui32MaxSize, bBufferFull);
    if (!bBufferFull) {
        writeRequests (_pFragmentRequests, &iw, ui32MaxSize, bBufferFull);
    }

    // Write new element tag
    iw.write8 (&DOES_NOT_HAVE_NEXT);

    _ui16Size = iw.getBytesWritten();

    return 0;
}

int DisServiceDataReqMsg::writeRequests (PtrLList<FragmentRequest> *pRequests, InstrumentedWriter *pIW,
                                         uint32 ui32MaxSize, bool &bBufferFull)
{
    if (pRequests == NULL) {
        return 0;
    }

    BufferWriter bw (ui32MaxSize, ui32MaxSize);
    int rc = 0;

    FragmentRequest *pFragmentRequest = pRequests->getFirst();
    while ((pFragmentRequest != NULL) && (!bBufferFull) && (rc == 0)) {
        if ((rc = writeRequest (pFragmentRequest, bw, ui32MaxSize, pIW->getBytesWritten())) == 0) {
            if ((rc = pIW->writeBytes (bw.getBuffer(), bw.getBufferLength())) == 0) {
                rc = pIW->write8 (&DOES_NOT_HAVE_NEXT);
            }
        }
        else {
            bBufferFull = true;
        }

        bw.reset();

        pFragmentRequest = pRequests->getNext();
    }

    return rc;
}

int DisServiceDataReqMsg::writeRequest (FragmentRequest *pFragmentRequest, BufferWriter &bw,
                                        uint32 ui32MaxSize, uint32 ui32WrittenBytes)
{
    if (pFragmentRequest == NULL) {
        return -1;
    }
    if (ui32MaxSize <= ui32WrittenBytes) {
        return -2;
    }

    // Instantiate a new BufferWriter - to ensure that we do not exceed the alloted space
    uint32 ui32AllottedSpace = ui32MaxSize - ui32WrittenBytes;
    BufferWriter bwTemp (ui32AllottedSpace, 10);

    // Write new element tag
    bwTemp.write8 (&HAS_NEXT);

    // Write message info
    MessageHeader *pMH = pFragmentRequest->pMsgHeader;
    if (pMH == NULL) {
        return -3;
    }

    String tmp (pMH->getGroupName());
    uint16 ui16 = tmp.length();
    bwTemp.write16 (&ui16);
    bwTemp.writeBytes ((const char*) tmp, ui16);

    tmp = pMH->getPublisherNodeId();
    ui16 = tmp.length();
    bwTemp.write16 (&ui16);
    bwTemp.writeBytes ((const char*) tmp, ui16);

    uint32 ui32MsgSeqId = pMH->getMsgSeqId();
    bwTemp.write32 (&ui32MsgSeqId);

    tmp = pMH->getObjectId();
    ui16 = (tmp.length() <= 0 ? 0 : tmp.length());
    bwTemp.write16 (&ui16);
    if (ui16 > 0) {
        bwTemp.writeBytes ((const char*) tmp, ui16);
    }

    tmp = pMH->getInstanceId();
    ui16 = (tmp.length() <= 0 ? 0 : tmp.length());
    bwTemp.write16 (&ui16);
    if (ui16 > 0) {
        bwTemp.writeBytes ((const char*) tmp, ui16);
    }

    ui16 = pMH->getTag();
    bwTemp.write16 (&ui16);

    uint32 ui32 = pMH->getTotalMessageLength();
    bwTemp.write32 (&ui32);

    uint8 ui8 = pMH->getChunkId();
    bwTemp.write8 (&ui8);

    if (pMH->isChunk()) {
        ui8 = ((ChunkMsgInfo *)pMH)->getTotalNumberOfChunks();
        bwTemp.write8 (&ui8);
    }

    // Write the ranges
    int rc = -4;
    bool bTailComputed = false;
    for (Range *pRange = pFragmentRequest->pRequestedRanges->getFirst();
         (pRange != NULL) || ((pRange == NULL) && pFragmentRequest->bRequestMessageTail && (!bTailComputed));
         pRange = pFragmentRequest->pRequestedRanges->getNext()) {
        uint32 ui32From, ui32Len;
        if (pRange != NULL) {
            ui32From = pRange->getFrom();
            ui32Len = pRange->getTo() - pRange->getFrom();
        }
        else {
            // This means that pFragmentRequest->bRequestMessageTail is set on true
            ui32From = pFragmentRequest->ui32NextExpectedOffset;
            ui32Len = pFragmentRequest->ui32TotalMessageLength - pFragmentRequest->ui32NextExpectedOffset;
            bTailComputed = true;
        }
        writeRange (&bwTemp, ui32From, ui32Len);
        if ((bwTemp.getBufferLength() + sizeof (uint8) + sizeof (uint8)) < ui32AllottedSpace) {
            // Copy what we have written so far into the regular buffer
            unsigned long tmpBufLen = bwTemp.getBufferLength();
            if ((bwTemp.getBuffer() != NULL) && (tmpBufLen > 0) && (tmpBufLen <= ui32AllottedSpace)) {
                bw.writeBytes (bwTemp.getBuffer(), tmpBufLen);
                ui32AllottedSpace -= tmpBufLen;
                bwTemp.reset();
                rc = 0;    // Set to 0 if at least one range has been successfully written
            }
        }
        else {
            break;
        }
    }

    return rc;
}

PtrLList<DisServiceDataReqMsg::FragmentRequest> * DisServiceDataReqMsg::getRequests (void)
{
    return _pFragmentRequests;
}

PtrLList<DisServiceDataReqMsg::FragmentRequest> * DisServiceDataReqMsg::relinquishRequests (void)
{
    PtrLList<DisServiceDataReqMsg::FragmentRequest> *pRet = _pFragmentRequests;
    _pFragmentRequests = NULL;
    return pRet;
}

uint16 DisServiceDataReqMsg::getNumberOfActiveNeighbors()
{
    return _ui16NumberOfNeighbors;
}

void DisServiceDataReqMsg::writeRange (Writer *pWriter, uint32 ui32From, uint32 ui32Len)
{
    InstrumentedWriter iw (pWriter);
    iw.write8 (&HAS_NEXT);

    uint16 ui16Len;
    while (ui32Len > 65535) {
        ui16Len = 65535;
        iw.write32 (&ui32From);
        iw.write16 (&ui16Len);
        ui32Len -= ui16Len;
        ui32From += ui16Len;
        iw.write8 (&HAS_NEXT);
    }
    ui16Len = (uint16) ui32Len;
    iw.write32(&ui32From);
    iw.write16(&ui16Len);
}

//==============================================================================
// DisServiceIncrementalDataReqMsg CONTROL
//==============================================================================

DisServiceIncrementalDataReqMsg::DisServiceIncrementalDataReqMsg (const char *pszSenderNodeId, const char* pszQueryTargetNodeId,
                                                                  uint16 ui16NumberOfNeighbors, MessageRequestScheduler *pReqScheduler,
                                                                  uint32 ui32MaxSize)
    : DisServiceDataReqMsg (pszSenderNodeId, pszQueryTargetNodeId, ui16NumberOfNeighbors,
                            NULL, NULL),
      _bw (ui32MaxSize, 1024)
{
    _pReqScheduler = pReqScheduler;
    _ui32MaxSize = ui32MaxSize;
}

DisServiceIncrementalDataReqMsg::~DisServiceIncrementalDataReqMsg()
{
}

int DisServiceIncrementalDataReqMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (_pReqScheduler == NULL) {
        checkAndLogMsg ("DisServiceIncrementalDataReqMsg::write", Logger::L_SevereError,
                        "_pReqScheduler is NULL\n");
    }

    InstrumentedWriter iw (pWriter);

    int rc;
    if ((rc = DisServiceCtrlMsg::write (&iw, ui32MaxSize)) != 0) {
        return rc;
    }
    // write number of neighboring peers
    if ((rc = iw.write16 (&_ui16NumberOfNeighbors)) != 0) {
        return rc;
    }

    // Write requests
    bool bAtLeastOneRequest = false;
    FragmentRequest *pFragReq = _pReqScheduler->getRequest();
    while ((pFragReq != NULL) && (add (&iw, pFragReq) == 0)) {
        bAtLeastOneRequest = true;
        delete pFragReq;
        pFragReq = _pReqScheduler->getRequest();
    }
    delete pFragReq;

    if (!bAtLeastOneRequest) {
        return 1;
    }

    iw.write8 (&DOES_NOT_HAVE_NEXT);

    _ui16Size = iw.getBytesWritten();

    return 0;
}

int DisServiceIncrementalDataReqMsg::add (InstrumentedWriter *pIW, FragmentRequest *pFragReq)
{
    // Write range and ranges terminator
    int rc;
    if ((rc = writeRequest (pFragReq, _bw, _ui32MaxSize, pIW->getBytesWritten())) == 0) {
        if ((rc = pIW->writeBytes (_bw.getBuffer(), _bw.getBufferLength())) == 0) {
            rc = pIW->write8 (&DOES_NOT_HAVE_NEXT);
        }
    }

    if (rc == 0) {
        if (pLogger != NULL) {
            FILE *pLogFile = pLogger->getLogFileHandle();
            if (pLogFile != NULL) {
                display (pFragReq, pLogger->getLogFileHandle());
            }
        }
        display (pFragReq, stdout);
        /*checkAndLogMsg ("DisServiceIncrementalDataReqMsg::add", Logger::L_Info,
                        "Added \n");*/
    }

    _bw.reset();

    return rc;
}

//==============================================================================
//  DisServiceDataReqMsg::FragmentRequest
//==============================================================================

DisServiceDataReqMsg::FragmentRequest::FragmentRequest (MessageHeader *pMH, PtrLList<Range>* pRanges)
{
    pMsgHeader = pMH;
    pRequestedRanges  = pRanges;
    bRequestMessageTail = false;
    ui32NextExpectedOffset = 0;
    ui32TotalMessageLength = 0;    // Only used when the tail of the message is requested
    this->bDelValues = false /*bDelValues*/;
}

DisServiceDataReqMsg::FragmentRequest::FragmentRequest (MessageHeader *pMH, PtrLList<Range>* pRanges,
                                                        uint32 ui32NextExpectedOffset,
                                                        uint32 ui32TotalMessageLength/*, bool bDelValues*/)
{
    pMsgHeader = pMH;
    pRequestedRanges  = pRanges;
    bRequestMessageTail = true;
    this->ui32NextExpectedOffset = ui32NextExpectedOffset;
    this->ui32TotalMessageLength = ui32TotalMessageLength;
    this->bDelValues = false/*bDelValues*/;
}

DisServiceDataReqMsg::FragmentRequest::~FragmentRequest ()
{
    if (bDelValues) {
        delete pMsgHeader;
        delete pRequestedRanges;
    }
    pMsgHeader= NULL;
    pRequestedRanges= NULL;
}

bool DisServiceDataReqMsg::FragmentRequest::operator == (FragmentRequest &toCompare)
{
    return ((pMsgHeader->getMsgId() == toCompare.pMsgHeader->getMsgId()) &&
            (pMsgHeader->getFragmentOffset() == toCompare.pMsgHeader->getFragmentOffset()) &&
            (pMsgHeader->getFragmentLength() == toCompare.pMsgHeader->getFragmentLength()));
}

bool DisServiceDataReqMsg::FragmentRequest::operator > (FragmentRequest &toCompare)
{
    if (*this == toCompare) {
        return false;
    }
    // Return true of false randomly
    return ((((rand() % 100) + 1.0f) < 50.0f));
}

bool DisServiceDataReqMsg::FragmentRequest::operator < (FragmentRequest &toCompare)
{
    if (*this == toCompare) {
        return false;
    }
    // Return true of false randomly
    return ((((rand() % 100) + 1.0f) < 50.0f));
}

DisServiceDataReqMsg::MessageRequest::MessageRequest (const char *pszGroupName,
                                                      const char *pszSenderNodeId,
                                                      uint32 ui32MsgSeqId,
                                                      uint8 ui8Priority)
    : DisServiceDataReqMsg::FragmentRequest (new MessageInfo (pszGroupName,
                                                              pszSenderNodeId,
                                                              ui32MsgSeqId,
                                                              NULL, // pszObjectId
                                                              NULL, // pszInstanceId
                                                              0,    // ui16Tag
                                                              0,    // ui16ClientId
                                                              0,    // ui8ClientType
                                                              NULL, // pszMimeType
                                                              NULL, // pszChecksum
                                                              0,    // ui32TotalMessageLength
                                                              0,    // ui32FragmentLength
                                                              0,    // ui32FragmentOffest
                                                              0,    // ui32MetaDataLength
                                                              0,    // ui16HistoryWindow
                                                              ui8Priority),
                                             new PtrLList<DisServiceMsg::Range>(new DisServiceMsg::Range (0,0)))
{
}

DisServiceDataReqMsg::MessageRequest::~MessageRequest()
{
    delete pMsgHeader;
    pMsgHeader = NULL;

    // I know there's only one element...
    Range *pRange = pRequestedRanges->getFirst();
    delete pRange;
    pRange = NULL;

    delete pRequestedRanges;
    pRequestedRanges = NULL;
}

DisServiceDataReqMsg::ChunkMessageRequest::ChunkMessageRequest (const char *pszGroupName,
                                                                const char *pszSenderNodeId,
                                                                uint32 ui32MsgSeqId,
                                                                uint8 ui8ChunkId,
                                                                uint8 ui8Priority)
    : DisServiceDataReqMsg::FragmentRequest (new ChunkMsgInfo (pszGroupName,
                                                               pszSenderNodeId,
                                                               ui32MsgSeqId,
                                                               ui8ChunkId,
                                                               NULL, // pszObjectId
                                                               NULL, // pszInstanceId
                                                               0,    // ui16Tag
                                                               0,    // ui16ClientId
                                                               0,    // ui8ClientType
                                                               NULL, // pszMimeType
                                                               NULL, // pszChecksum
                                                               0,    // ui32FragmentOffest
                                                               0,    // ui32FragmentLength
                                                               0,    // ui32TotalMessageLength
                                                               0,    // ui8TotalNumOfChunks
                                                               0,    // ui16HistoryWindow
                                                               ui8Priority),
                                             new PtrLList<DisServiceMsg::Range>(new DisServiceMsg::Range(0,0)))
{
}

DisServiceDataReqMsg::ChunkMessageRequest::~ChunkMessageRequest()
{
    delete pMsgHeader;
    pMsgHeader = NULL;

    // I know there's only one element...
    Range *pRange = pRequestedRanges->getFirst();
    delete pRange;
    pRange = NULL;

    delete pRequestedRanges;
    pRequestedRanges = NULL;
}

//==============================================================================
//  DisServiceWorldStateSeqIdMsg
//==============================================================================

DisServiceWorldStateSeqIdMsg::DisServiceWorldStateSeqIdMsg()
    : DisServiceCtrlMsg (DSMT_WorldStateSeqId)
{
    _ui32TopologyStateUpdateSeqId = 0;
    _ui16SubscriptionStateCRC = 0;
    _ui32DataCacheStateUpdateSeqId = 0;
    _ui8repCtrlType = 0;
    _ui8fwdCtrlType = 0;
    _ui8NodeImportance = 0;
    _seqId = 0;

}

DisServiceWorldStateSeqIdMsg::DisServiceWorldStateSeqIdMsg (const char *pszSenderNodeId,
                                                            uint32 ui32TopologyStateUpdateSeqId,
                                                            uint16 ui16SubscriptionStateCRC, uint32 ui32DataCacheStateUpdateSeqId,
                                                            uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType)
    : DisServiceCtrlMsg (DSMT_WorldStateSeqId, pszSenderNodeId)
{
   DisServiceWorldStateSeqIdMsg (pszSenderNodeId, ui32TopologyStateUpdateSeqId, ui16SubscriptionStateCRC, ui32DataCacheStateUpdateSeqId, ui8RepCtrlType, ui8FwdCtrlType, 0);
}

DisServiceWorldStateSeqIdMsg::DisServiceWorldStateSeqIdMsg (const char *pszSenderNodeId,
                                                            uint32 ui32TopologyStateUpdateSeqId,
                                                            uint16 ui16SubscriptionStateCRC, uint32 ui32DataCacheStateUpdateSeqId,
                                                            uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType, uint8 ui8NodeImportance)
    : DisServiceCtrlMsg (DSMT_WorldStateSeqId, pszSenderNodeId)
{
    _ui32TopologyStateUpdateSeqId = ui32TopologyStateUpdateSeqId;
    _ui16SubscriptionStateCRC = ui16SubscriptionStateCRC;
    _ui32DataCacheStateUpdateSeqId = ui32DataCacheStateUpdateSeqId;
    _ui8repCtrlType = ui8RepCtrlType;
    _ui8fwdCtrlType = ui8FwdCtrlType;
    _ui16NumberOfActiveNeighbors = 0;
    _ui8MemorySpace = 0;
    _ui8Bandwidth = 0;
    _ui8NodesInConnectivityHistory = 0;
    _ui8NodesRepetitivity = 0;
    _ui8NodeImportance = ui8NodeImportance;
}

DisServiceWorldStateSeqIdMsg::~DisServiceWorldStateSeqIdMsg()
{
}

int DisServiceWorldStateSeqIdMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_WorldStateSeqId) {
        return -2;
    }

    pReader->read32 (&_ui32TopologyStateUpdateSeqId);
    pReader->read16 (&_ui16SubscriptionStateCRC);
    pReader->read32 (&_ui32DataCacheStateUpdateSeqId);
    pReader->read8 (&_ui8repCtrlType);
    pReader->read8 (&_ui8fwdCtrlType);
    pReader->read8 (&_ui8NodeImportance);
    pReader->read16 (&_ui16NumberOfActiveNeighbors);
    readNodeParameters (pReader);

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceWorldStateSeqIdMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    iw.write32 (&_ui32TopologyStateUpdateSeqId);
    iw.write16 (&_ui16SubscriptionStateCRC);
    iw.write32 (&_ui32DataCacheStateUpdateSeqId);
    iw.write8 (&_ui8repCtrlType);
    iw.write8 (&_ui8fwdCtrlType);
    iw.write8 (&_ui8NodeImportance);
    iw.write16 (&_ui16NumberOfActiveNeighbors);
    writeNodeParameters (&iw);

    _ui16Size = iw.getBytesWritten();
    return 0;
}

int DisServiceWorldStateSeqIdMsg::writeNodeParameters (Writer *pWriter)
{
    uint16 ui16Buffer = 0;
    ui16Buffer += _ui8MemorySpace;
    ui16Buffer = ui16Buffer << 3;
    ui16Buffer += _ui8Bandwidth;
    ui16Buffer = ui16Buffer << 3;
    ui16Buffer += _ui8NodesInConnectivityHistory;
    ui16Buffer = ui16Buffer << 3;
    ui16Buffer += _ui8NodesRepetitivity;
    pWriter->write16 (&ui16Buffer);
    return 0;
}

int DisServiceWorldStateSeqIdMsg::readNodeParameters (Reader *pReader)
{
    uint16 ui16Buffer = 0;
    pReader->read16 (&ui16Buffer);
    _ui8NodesRepetitivity = ui16Buffer & 7;
    ui16Buffer = ui16Buffer >> 3;
    _ui8NodesInConnectivityHistory = ui16Buffer & 7;
    ui16Buffer = ui16Buffer >> 3;
    _ui8Bandwidth = ui16Buffer & 7;
    ui16Buffer = ui16Buffer >> 3;
    _ui8MemorySpace = ui16Buffer & 7;
    return 0;
}

uint32 DisServiceWorldStateSeqIdMsg::getTopologyStateUpdateSeqId()
{
    return _ui32TopologyStateUpdateSeqId;
}

uint16 DisServiceWorldStateSeqIdMsg::getSubscriptionStateCRC()
{
    return _ui16SubscriptionStateCRC;
}

uint32 DisServiceWorldStateSeqIdMsg::getDataCacheStateUpdateSeqId()
{
    return _ui32DataCacheStateUpdateSeqId;
}

uint8 DisServiceWorldStateSeqIdMsg::getRepCtrlType ()
{
    return _ui8repCtrlType;
}

uint8 DisServiceWorldStateSeqIdMsg::getFwdCtrlType ()
{
    return _ui8fwdCtrlType;
}

uint8 DisServiceWorldStateSeqIdMsg::getNumberOfActiveNeighbors()
{
    return (uint8) _ui16NumberOfActiveNeighbors;
}

uint8 DisServiceWorldStateSeqIdMsg::getMemorySpace()
{
    return _ui8MemorySpace;
}

uint8 DisServiceWorldStateSeqIdMsg::getBandwidth()
{
    return _ui8Bandwidth;
}

uint8 DisServiceWorldStateSeqIdMsg::getNodesInConnectivityHistory()
{
    return _ui8NodesInConnectivityHistory;
}

uint8 DisServiceWorldStateSeqIdMsg::getNodesRepetitivity()
{
    return _ui8NodesRepetitivity;
}

uint8 DisServiceWorldStateSeqIdMsg::getNodeImportance (void)
{
    return _ui8NodeImportance;
}

void DisServiceWorldStateSeqIdMsg::setNumberOfActiveNeighbors (uint16 ui16NumberOfActiveNeighbors)
{
    _ui16NumberOfActiveNeighbors = ui16NumberOfActiveNeighbors;
}

void DisServiceWorldStateSeqIdMsg::setMemorySpace (uint8 ui8MemorySpace)
{
    _ui8MemorySpace = ui8MemorySpace;
}

void DisServiceWorldStateSeqIdMsg::setBandwidth (uint8 ui8Bandwidth)
{
    _ui8Bandwidth = ui8Bandwidth;
}

void DisServiceWorldStateSeqIdMsg::setNodesInConnectivityHistory (uint8 ui8NodesInConnectivityHistory)
{
    _ui8NodesInConnectivityHistory = ui8NodesInConnectivityHistory;
}

void DisServiceWorldStateSeqIdMsg::setNodesRepetitivity (uint8 ui8NodesRepetitivity)
{
    _ui8NodesRepetitivity = ui8NodesRepetitivity;
}

void DisServiceWorldStateSeqIdMsg::setPubAdv (const String &group, uint32 ui32) {

}

//==============================================================================
// DisServiceSubscriptionStateMsg
//==============================================================================

DisServiceSubscriptionStateMsg::DisServiceSubscriptionStateMsg()
    : DisServiceCtrlMsg (DSMT_SubStateMessage)
{
}

DisServiceSubscriptionStateMsg::DisServiceSubscriptionStateMsg (const char *pszSenderNodeId,
                                                                StringHashtable<DArray2<String> > *pSubscriptionsTable,
                                                                StringHashtable<uint32> *pNodesTable)
    : DisServiceCtrlMsg (DSMT_SubStateMessage, pszSenderNodeId)
{
    _pSubscriptionsTable = pSubscriptionsTable;
    _pNodesTable = pNodesTable;
}

DisServiceSubscriptionStateMsg::~DisServiceSubscriptionStateMsg (void)
{
}

int DisServiceSubscriptionStateMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != DSMT_SubStateMessage) {
        return -2;
    }

    uint8 ui8Subscriptions = 0;
    _pSubscriptionsTable = new StringHashtable<DArray2<String> >();
    _pNodesTable = new StringHashtable<uint32>();

    pReader->read8 (&ui8Subscriptions);
    for (int i = 0; i < ui8Subscriptions; i++) {
        uint8 ui8SubscriptionLength;
        uint8 ui8Nodes;
        String subscription;
        pReader->read8 (&ui8SubscriptionLength);
        char *pszBuf = new char [ui8SubscriptionLength + 1];
        pReader->readBytes (pszBuf, ui8SubscriptionLength);
        pszBuf[ui8SubscriptionLength] = '\0';
        subscription = pszBuf;
        delete[] pszBuf;
        pszBuf = NULL;
        pReader->read8 (&ui8Nodes);
        DArray2<String> *pNodes = new DArray2<String>();
        for (int j = 0; j < ui8Nodes; j++) {
            String nodeId;
            uint8 ui8NodeIdLength;
            uint32 *pui32SeqId = new uint32();
            pReader->read8 (&ui8NodeIdLength);
            pszBuf = new char [ui8NodeIdLength + 1];
            pReader->readBytes (pszBuf, ui8NodeIdLength);
            pszBuf[ui8NodeIdLength] = '\0';
            nodeId = pszBuf;
            delete[] pszBuf;
            pszBuf = NULL;
            pReader->read32 (pui32SeqId);
            (*pNodes)[j] = nodeId;
            _pNodesTable->put (nodeId, pui32SeqId);
        }
        _pSubscriptionsTable->put (subscription, pNodes);
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceSubscriptionStateMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    BufferWriter subscriptionsBW (ui32MaxSize, ui32MaxSize);
    BufferWriter nodesBW (ui32MaxSize, ui32MaxSize);
    uint8 ui8Subscriptions = 0;
    for (StringHashtable<DArray2<String> >::Iterator iterator = _pSubscriptionsTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        String subscription = iterator.getKey();
        uint8 ui8SubscriptionLength = subscription.length();
        uint8 ui8Nodes = 0;
        DArray2<String> *pNodes = iterator.getValue();
        for (int i = 0; i <= pNodes->getHighestIndex(); i++) {
            String nodeId = (*pNodes)[i];
            uint8 ui8NodeIdLength = nodeId.length();
            uint32 *pui32SeqId = _pNodesTable->get (nodeId);
            if ((nodesBW.getBufferLength() + ui8NodeIdLength + sizeof (uint32) + sizeof (uint8)) <= ui32MaxSize) {
                nodesBW.write8 (&ui8NodeIdLength);
                nodesBW.writeBytes (nodeId, ui8NodeIdLength);
                nodesBW.write32 (pui32SeqId);
                ui8Nodes++;
            }
        }
        if ((subscriptionsBW.getBufferLength() + sizeof (uint8) + ui8SubscriptionLength + nodesBW.getBufferLength() + sizeof (uint8) + sizeof (uint8)) <= ui32MaxSize) {
            subscriptionsBW.write8 (&ui8SubscriptionLength);
            subscriptionsBW.writeBytes (subscription, ui8SubscriptionLength);
            subscriptionsBW.write8 (&ui8Nodes);
            subscriptionsBW.writeBytes (nodesBW.getBuffer(), nodesBW.getBufferLength());
            ui8Subscriptions++;
        }
        nodesBW.reset();
    }
    iw.write8 (&ui8Subscriptions);
    iw.writeBytes (subscriptionsBW.getBuffer(), subscriptionsBW.getBufferLength());

    _ui16Size = iw.getBytesWritten();
    return 0;
}

StringHashtable<DArray2<String> > * DisServiceSubscriptionStateMsg::getSubscriptionsTable()
{
    return _pSubscriptionsTable;
}

StringHashtable<uint32> * DisServiceSubscriptionStateMsg::getNodesTable()
{
    return _pNodesTable;
}

//==============================================================================
// DisServiceSubscriptionStateReqMsg
//==============================================================================
DisServiceSubscriptionStateReqMsg::DisServiceSubscriptionStateReqMsg()
    : DisServiceCtrlMsg (DSMT_SubStateReq)
{
}

DisServiceSubscriptionStateReqMsg::DisServiceSubscriptionStateReqMsg (const char *pszSenderNodeId, const char *pszTargetNodeId)
    : DisServiceCtrlMsg (DSMT_SubStateReq, pszSenderNodeId, pszTargetNodeId)
{
    _pSubscriptionStateTable = NULL;
}

DisServiceSubscriptionStateReqMsg::~DisServiceSubscriptionStateReqMsg (void)
{
}

int DisServiceSubscriptionStateReqMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::DisServiceMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != DSMT_SubStateReq) {
        return -2;
    }

    _pSubscriptionStateTable = new StringHashtable<uint32> (true, true, true, false);
    uint8 ui8NodesIds;
    pReader->read8 (&ui8NodesIds);
    for (int i = 0; i < ui8NodesIds; i++) {
        uint8 ui8NodeIdLength;
        uint32 *pui32SeqId = new uint32();
        String nodeId;
        pReader->read8 (&ui8NodeIdLength);
        char *pszBuf = new char [ui8NodeIdLength + 1];
        pReader->readBytes (pszBuf, ui8NodeIdLength);
        pszBuf[ui8NodeIdLength] = '\0';
        nodeId = pszBuf;
        delete[] pszBuf;
        pszBuf = NULL;
        pReader->read32 (pui32SeqId);
        _pSubscriptionStateTable->put (nodeId, pui32SeqId);
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceSubscriptionStateReqMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::DisServiceMsg::write (&iw, ui32MaxSize) != 0) {
        return -2;
    }

    BufferWriter bw (ui32MaxSize, ui32MaxSize);
    uint8 ui8NodeIds = 0;
    for (StringHashtable<uint32>::Iterator iterator = _pSubscriptionStateTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        String nodeId (iterator.getKey());
        uint8 ui8NodeIdLength = nodeId.length();
        bw.write8 (&ui8NodeIdLength);
        bw.writeBytes (nodeId, ui8NodeIdLength);
        bw.write32 (iterator.getValue());
        if ((bw.getBufferLength() + sizeof (uint8)) <= ui32MaxSize) {
            ui8NodeIds++;
        }
    }
    pWriter->write8 (&ui8NodeIds);
    pWriter->writeBytes (bw.getBuffer(), bw.getBufferLength());

    _ui16Size = iw.getBytesWritten();
    return 0;
}

void DisServiceSubscriptionStateReqMsg::setSubscriptionStateTable (StringHashtable<uint32> *pSubscriptionStateTable)
{
    _pSubscriptionStateTable = pSubscriptionStateTable;
}

StringHashtable<uint32> * DisServiceSubscriptionStateReqMsg::getSubscriptionStateTable (void)
{
    return _pSubscriptionStateTable;
}

//==============================================================================
// DisServiceDataCacheQueryMsg
//==============================================================================
DisServiceDataCacheQueryMsg::DisServiceDataCacheQueryMsg()
    : DisServiceCtrlMsg (DSMT_DataCacheQuery)
{
}

DisServiceDataCacheQueryMsg::DisServiceDataCacheQueryMsg (const char *pszSenderNodeId, const char *pszQueryTargetNodeId)
    : DisServiceCtrlMsg (DSMT_DataCacheQuery, pszSenderNodeId, pszQueryTargetNodeId )
{
}

DisServiceDataCacheQueryMsg::~DisServiceDataCacheQueryMsg()
{
}

int DisServiceDataCacheQueryMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_DataCacheQuery) {
        return -2;
    }

    uint16 ui16Length;
    uint16 ui16NumberOfQueries;
    pReader->read16 (&ui16NumberOfQueries);
    for (uint16 i = 0; i < ui16NumberOfQueries; i++) {
        pReader->read16 (&ui16Length);
        char *pszQuery = new char [ui16Length + 1];
        pReader->readBytes (pszQuery, ui16Length);
        pszQuery [ui16Length] = '\0';
        _queries[i] = String (pszQuery);
        delete[] pszQuery;
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceDataCacheQueryMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    uint16 ui16Length;
    ui16Length = (uint16)(_queries.getHighestIndex() + 1);
    iw.write16(&ui16Length);
    BufferWriter bw (1024, 1024);
    for (uint16 i = 0; i <= _queries.getHighestIndex(); i++) {
        if (_queries.used(i)) {
            ui16Length = _queries[i].length();
            bw.write16 (&ui16Length);
            bw.writeBytes ((const char*) _queries[i], ui16Length);
            if (((iw.getBytesWritten() + bw.getBufferLength()) <= ui32MaxSize) || (ui32MaxSize == 0)) {
                iw.writeBytes(bw.getBuffer(), bw.getBufferLength());
                bw.reset();
            }
            else {
                break;
            }
        }
    }

    _ui16Size = iw.getBytesWritten();
    return 0;
}

int DisServiceDataCacheQueryMsg::addQuery (DisServiceDataCacheQuery * pQuery)
{
    #if defined (USE_SQLITE)
        _queries[_queries.getHighestIndex()+1] = pQuery->getSqlQuery ();
    #else
        //TODO: implement this
        checkAndLogMsg ("DisServiceDataCacheQueryMsg::addQuery", Logger::L_SevereError, "Method not implemented yet.\n");
    #endif
    return 0;
}

uint16 DisServiceDataCacheQueryMsg::getQueryCount()
{
    return (uint16)(_queries.getHighestIndex() + 1);
}

const char * DisServiceDataCacheQueryMsg::getQuery (uint16 ui16Index)
{
    if (ui16Index <= _queries.getHighestIndex()) {
        return _queries[ui16Index];
    }
    else {
        return NULL;
    }
}

//==============================================================================
// DisServiceDataCacheQueryReplyMsg
//==============================================================================
DisServiceDataCacheQueryReplyMsg::DisServiceDataCacheQueryReplyMsg()
    : DisServiceCtrlMsg (DSMT_DataCacheQueryReply)
{
    _pMessageIDs = NULL;
    _bDeleteMessageIDs = false;
}

DisServiceDataCacheQueryReplyMsg::DisServiceDataCacheQueryReplyMsg (Type type)
    : DisServiceCtrlMsg (type)
{
    _pMessageIDs = NULL;
    _bDeleteMessageIDs = false;
}

DisServiceDataCacheQueryReplyMsg::DisServiceDataCacheQueryReplyMsg (const char *pszSenderNodeId, Type type)
    : DisServiceCtrlMsg (type, pszSenderNodeId)
{
}

DisServiceDataCacheQueryReplyMsg::DisServiceDataCacheQueryReplyMsg (const char *pszSenderNodeId, DArray2<String> *pMessageIDs)
    : DisServiceCtrlMsg (DSMT_DataCacheQueryReply, pszSenderNodeId)
{
    _pMessageIDs = pMessageIDs;
    _bDeleteMessageIDs = false;
}

DisServiceDataCacheQueryReplyMsg::~DisServiceDataCacheQueryReplyMsg (void)
{
    if (_bDeleteMessageIDs) {
        delete _pMessageIDs;
        _pMessageIDs = NULL;
    }
}

int DisServiceDataCacheQueryReplyMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if ((_type != DSMT_DataCacheQueryReply) && (_type != DSMT_HistoryReqReply)) {
        return -2;
    }

    _pMessageIDs = new DArray2<String>;
    _bDeleteMessageIDs = true;

    uint8 ui8HasNext;
    uint16 ui16Length, i;
    i = 0;
    for (pReader->read8 (&ui8HasNext); ui8HasNext == 1; pReader->read8 (&ui8HasNext)) {
        pReader->read16 (&ui16Length);
        char *pszMsgID = new char [ui16Length + 1];
        pReader->readBytes (pszMsgID, ui16Length);
        pszMsgID [ui16Length] = '\0';
        (*_pMessageIDs)[i] = String (pszMsgID);
        delete pszMsgID;
        i++;
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceDataCacheQueryReplyMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    BufferWriter bw (ui32MaxSize, ui32MaxSize);
    uint16 ui16Length;
    uint8 ui8HasNext = 1;

    for (uint16 i = 0; (i <= _pMessageIDs->getHighestIndex()); i++) {
        if (_pMessageIDs->used(i)) {
            // Write msg id length
            ui16Length = (*_pMessageIDs)[i].length();
            if (ui16Length > 0) {
                bw.write8(&ui8HasNext);
                // Write msg id
                bw.write16 (&ui16Length);
                bw.writeBytes ((const char*) (*_pMessageIDs)[i], ui16Length);

                if (((iw.getBytesWritten() + bw.getBufferLength() + sizeof (ui8HasNext)) <= ui32MaxSize) || (ui32MaxSize == 0)) {
                    // if there's room enough, write it
                    iw.writeBytes(bw.getBuffer(), bw.getBufferLength());
                    bw.reset();
                }
                else {
                    break;
                }
            }
        }
    }

    ui8HasNext = 0;
    iw.write8(&ui8HasNext);

    _ui16Size = iw.getBytesWritten();
    return 0;
}

uint16 DisServiceDataCacheQueryReplyMsg::getIDsCount (void)
{
    return (uint16)(_pMessageIDs->getHighestIndex() + 1);
}

DArray2<String> * DisServiceDataCacheQueryReplyMsg::getIDs (void)
{
    return _pMessageIDs;
}

const char * DisServiceDataCacheQueryReplyMsg::getID (uint16 ui16Index)
{
    if (ui16Index <= _pMessageIDs->getHighestIndex()) {
        return (*_pMessageIDs)[ui16Index];
    }
    else {
        return NULL;
    }
}

//==============================================================================
// DisServiceDataCacheMessagesRequestMsg
//==============================================================================
DisServiceDataCacheMessagesRequestMsg::DisServiceDataCacheMessagesRequestMsg()
    : DisServiceCtrlMsg (DSMT_DataCacheMessagesRequest)
{
    _pMessageIDs = NULL;
    _bDeleteMessageIDs = false;
}

DisServiceDataCacheMessagesRequestMsg::DisServiceDataCacheMessagesRequestMsg (const char *pszSenderNodeId,
                                                                                         const char *pszQueryTargetNodeId,
                                                                                         DArray2<String> *pMessageIDs)
    : DisServiceCtrlMsg (DSMT_DataCacheMessagesRequest, pszSenderNodeId),
      _targetNodeId (pszQueryTargetNodeId)
{
    _pMessageIDs = pMessageIDs;
    _bDeleteMessageIDs = false;
}

DisServiceDataCacheMessagesRequestMsg::~DisServiceDataCacheMessagesRequestMsg()
{
    if (_bDeleteMessageIDs) {
        delete _pMessageIDs;
        _pMessageIDs = NULL;
    }
}

int DisServiceDataCacheMessagesRequestMsg::read(Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_DataCacheMessagesRequest) {
        return -2;
    }

    _pMessageIDs = new DArray2<String>;
    _bDeleteMessageIDs = true;

    uint16 ui16NumberOfIDs;
    uint16 ui16Length;
    pReader->read16 (&ui16Length);
    char *pszTarget = new char [ui16Length + 1];
    if (ui16Length > 0) {
        pReader->readBytes (pszTarget, ui16Length);
    }
    pszTarget [ui16Length] = '\0';
    _targetNodeId = pszTarget;
    pReader->read16 (&ui16NumberOfIDs);
    for (uint16 i = 0; i < ui16NumberOfIDs; i++) {
        pReader->read16 (&ui16Length);
        char *pszMsgID = new char [ui16Length + 1];
        pReader->readBytes(pszMsgID, ui16Length);
        pszMsgID[ui16Length] = '\0';
        (*_pMessageIDs)[i] = String (pszMsgID);
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceDataCacheMessagesRequestMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    uint16 ui16Length = 0;
    if (_targetNodeId.length() > 0) {
        ui16Length = _targetNodeId.length();
    }
    iw.write16 (&ui16Length);
    if (ui16Length > 0) {
        iw.writeBytes ((const char*) _targetNodeId, ui16Length);
    }
    ui16Length = (uint16)(_pMessageIDs->getHighestIndex() + 1);


    BufferWriter bw (ui32MaxSize, ui32MaxSize);
    uint32 ui32TmpBufLength = 0;
    uint16 i, ui16Count;    // ui16Count count sthe IDs actually written into the buffer
                            // i is the index to scan the array containing the IDs
    for (ui16Count = i = 0; i <= _pMessageIDs->getHighestIndex(); i++) {
        if ((*_pMessageIDs)[i] != NULL) {
            ui16Length = (*_pMessageIDs)[i].length();
            if (ui16Length > 0) {
                bw.write16 (&ui16Length);
                bw.writeBytes ((const char*) (*_pMessageIDs)[i], ui16Length);
                if (((iw.getBytesWritten() + bw.getBufferLength()) <= ui32MaxSize) || (ui32MaxSize == 0)) {
                    ui32TmpBufLength = bw.getBufferLength();
                    ui16Count++;
                }
                else {
                    break;
                }
            }
        }
    }

    if (ui16Count > 0) {
        iw.write16 (&ui16Count);
        iw.writeBytes(bw.getBuffer(), ui32TmpBufLength);
    }

    _ui16Size = iw.getBytesWritten();
    return 0;
}

uint16 DisServiceDataCacheMessagesRequestMsg::getIDsCount()
{
    return (uint16)(_pMessageIDs->getHighestIndex() + 1);
}

const char * DisServiceDataCacheMessagesRequestMsg::getID (uint16 ui16Index)
{
    if (ui16Index <= (*_pMessageIDs).getHighestIndex()) {
        return (*_pMessageIDs)[ui16Index];
    }
    else {
        return NULL;
    }
}

//==============================================================================
// DisServiceAcknowledgmentMessage
//==============================================================================
DisServiceAcknowledgmentMessage::DisServiceAcknowledgmentMessage()
    : DisServiceCtrlMsg (DSMT_AcknowledgmentMessage)
{
}

DisServiceAcknowledgmentMessage::DisServiceAcknowledgmentMessage (const char* pszSenderNodeId, const char* pszMsgToAckId)
    : DisServiceCtrlMsg (DSMT_AcknowledgmentMessage, pszSenderNodeId)
{
    _msgToAckId = pszMsgToAckId;
}

DisServiceAcknowledgmentMessage::~DisServiceAcknowledgmentMessage()
{
}

int DisServiceAcknowledgmentMessage::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_AcknowledgmentMessage) {
        return -2;
    }

    uint16 ui16length;
    pReader->read16 (&ui16length);
    char *pszMsgToAck = new char [ui16length + 1];
    pReader->readBytes (pszMsgToAck, ui16length);
    pszMsgToAck [ui16length] = '\0';
    _msgToAckId = pszMsgToAck;

    delete[] pszMsgToAck;
    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceAcknowledgmentMessage::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    uint16 ui16length = _msgToAckId.length();
    iw.write16 (&ui16length);
    iw.writeBytes ((const char*)_msgToAckId, ui16length);

    _ui16Size = iw.getBytesWritten();
    return 0;
}

const char * DisServiceAcknowledgmentMessage::getAckedMsgId()
{
    return _msgToAckId;
}

//==============================================================================
// DisServiceCompleteMessageReq
//==============================================================================
DisServiceCompleteMessageReqMsg::DisServiceCompleteMessageReqMsg()
    : DisServiceCtrlMsg (DSMT_CompleteMessageReq)
{
}

DisServiceCompleteMessageReqMsg::DisServiceCompleteMessageReqMsg (const char* pszSenderNodeId, const char *pMsgId)
    : DisServiceCtrlMsg (DSMT_CompleteMessageReq, pszSenderNodeId)
{
    _pMsgId = pMsgId;
}

DisServiceCompleteMessageReqMsg::~DisServiceCompleteMessageReqMsg()
{
}

int DisServiceCompleteMessageReqMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_CompleteMessageReq) {
        return -2;
    }

    uint16 ui16length;
    pReader->read16 (&ui16length);
    char *pszMsgId = new char [ui16length + 1];
    pReader->readBytes (pszMsgId, ui16length);
    pszMsgId [ui16length] = '\0';
    _pMsgId = pszMsgId;

    delete[] pszMsgId;
    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceCompleteMessageReqMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    uint16 ui16length = _pMsgId.length();
    iw.write16 (&ui16length);
    iw.writeBytes ((const char*)_pMsgId, ui16length);

    _ui16Size = iw.getBytesWritten();
    return 0;
}

const char * DisServiceCompleteMessageReqMsg::getMsgId()
{
    return _pMsgId;
}

//==============================================================================
// DisServiceCacheEmptyMsg
//==============================================================================
DisServiceCacheEmptyMsg::DisServiceCacheEmptyMsg()
    : DisServiceCtrlMsg (DSMT_CacheEmpty)
{
}

DisServiceCacheEmptyMsg::DisServiceCacheEmptyMsg (const char* pszSenderNodeId)
    : DisServiceCtrlMsg (DSMT_CacheEmpty, pszSenderNodeId)
{
}

DisServiceCacheEmptyMsg::~DisServiceCacheEmptyMsg()
{
}

int DisServiceCacheEmptyMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DSMT_CacheEmpty) {
        return -2;
    }

    return 0;
}

int DisServiceCacheEmptyMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    return 0;
}

//==============================================================================
// DisServiceHistoryRequest
//==============================================================================

DisServiceHistoryRequest::DisServiceHistoryRequest()
    : DisServiceCtrlMsg (DSMT_HistoryReq)
{
    _pReq = NULL;
    _pSenderState = NULL;
    _ui8SenderStateLength = 0;
}

DisServiceHistoryRequest::DisServiceHistoryRequest (const char * pszSenderNodeID, const char * pszTargetNodeID, HReq *pReq)
    : DisServiceCtrlMsg (DSMT_HistoryReq, pszSenderNodeID)
{
    _pReq = pReq;
    _targetNodeId = pszTargetNodeID;
    _pSenderState = NULL;
    _ui8SenderStateLength = 0;
}

DisServiceHistoryRequest::~DisServiceHistoryRequest()
{
    delete _pReq;
    _pReq = NULL;
    delete[] _pSenderState;
}

int DisServiceHistoryRequest::display (FILE *pFileOut)
{
    DisServiceCtrlMsg::display (pFileOut);
    _pReq->display (pFileOut);

    return 0;
}

DisServiceHistoryRequest::HReq * DisServiceHistoryRequest::getHistoryRequest()
{
    return _pReq;
}

int DisServiceHistoryRequest::setSenderState (SenderState **ppSenderState, uint8 ui8SenderStateLength)
{
    _pSenderState = ppSenderState;
    _ui8SenderStateLength = ui8SenderStateLength;
    return 0;
}

DisServiceHistoryRequest::SenderState ** DisServiceHistoryRequest::getSenderState (void)
{
    return _pSenderState;
}

uint8 DisServiceHistoryRequest::getSenderStateLength (void)
{
    return _ui8SenderStateLength;
}

int DisServiceHistoryRequest::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    uint8 ui8;
    pReader->read8 (&ui8);
    switch (ui8) {
        case HT_WIN:
            _pReq = new HWinReq();
            break;

        case HT_RANGE:
            _pReq = new HRangeReq();
            break;

        case HT_TMP_RANGE:
            _pReq = new HTmpRangeReq();
            break;
    }

    if (_pReq == NULL) {
        return -1;
    }

    _pReq->read (pReader, ui32MaxSize);

    // Read Sender State
    pReader->read8 (&_ui8SenderStateLength);
    if (_ui8SenderStateLength > 0) {
        _pSenderState = new SenderState*[_ui8SenderStateLength];
        uint16 ui16;
        uint32 ui32;
        char * pszSenderId;
        for (int i = 0; i < _ui8SenderStateLength; i++) {
            pReader->read16 (&ui16);
            if (ui16 > 0) {
                pszSenderId = new char[ui16+1];
                pReader->readBytes (pszSenderId, ui16);
                pszSenderId[ui16] = '\0';

                pReader->read32 (&ui32);
                _pSenderState[i] = new SenderState (pszSenderId, ui32);
            }
        }
    }

    return 0;
}

int DisServiceHistoryRequest::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::write (pWriter, ui32MaxSize) != 0) {
        return -1;
    }

    if (_pReq == NULL) {
        uint8 ui8Type = 0;
        pWriter->write8 (&ui8Type);
        return 0;
    }

    pWriter->write8 (&(_pReq->_ui8Type));

    _pReq->write (pWriter, ui32MaxSize);

    // Write Sender State
    pWriter->write8 (&_ui8SenderStateLength);
    if (_pSenderState != NULL) {
        uint16 ui16;
        const char *pszSenderId;
        for (int i = 0; i < _ui8SenderStateLength; i++) {
            pszSenderId = _pSenderState[i]->_senderId;

            ui16 = (uint16) strlen (pszSenderId);
            pWriter->write16 (&ui16);
            if (ui16 > 0) {
                pWriter->writeBytes (pszSenderId, ui16);
                pWriter->write32 (&(_pSenderState[i]->_ui32State));
            }
        }
    }

    return 0;
}

//==============================================================================
// DisServiceHistoryRequest::SenderState
//==============================================================================

DisServiceHistoryRequest::SenderState::SenderState()
{
}

DisServiceHistoryRequest::SenderState::SenderState (const char *pszSenderId, uint32 ui32State)
{
    _senderId = pszSenderId;
    _ui32State = ui32State;
}

DisServiceHistoryRequest::SenderState::~SenderState()
{
}

//==============================================================================
// DisServiceHistoryRequest::HReq
//==============================================================================

DisServiceHistoryRequest::HReq::HReq (uint8 ui8Type, const char *pszGroupName, uint16 ui16Tag, int64 i64RequestTimeout)
{
    _ui8Type = ui8Type;
    _groupName = pszGroupName;
    _ui16Tag = ui16Tag;
    _i64RequestTimeout = i64RequestTimeout;
}

DisServiceHistoryRequest::HReq::HReq (uint8 ui8Type)
{
    _ui8Type = ui8Type;
    _i64RequestTimeout = 0;
    _ui16Tag = 0;
}

DisServiceHistoryRequest::HReq::~HReq()
{
}

int DisServiceHistoryRequest::HReq::display (FILE *pFileOut)
{
    if (pFileOut == NULL) {
        return -1;
    }
    fprintf (pFileOut, "Type: %u\n", _ui8Type);
    fprintf (pFileOut, "Request Timeout: %lld\n", _i64RequestTimeout);
    fprintf (pFileOut, "Groupname: %s\n", (const char *) _groupName);
    fprintf (pFileOut, "Tag: %u\n", _ui16Tag);
    fflush (pFileOut);

    return 0;
}

int DisServiceHistoryRequest::HReq::read (Reader *pReader, uint32 ui32MaxSize)
{
    pReader->read64 (&_i64RequestTimeout);

    // Read _pszGroupName
    uint16 ui16;
    pReader->read16 (&ui16);
    if (ui16 > 0) {
        char *pszGroupName = (char *) malloc ((sizeof(char)*ui16)+1);
        pReader->readBytes (pszGroupName, ui16);
        pszGroupName[ui16] = '\0';
        _groupName = pszGroupName;
        free (pszGroupName);
    }

    pReader->read16 (&_ui16Tag);
    return 0;
}

int DisServiceHistoryRequest::HReq::write (Writer *pWriter, uint32 ui32MaxSize)
{
    pWriter->write64 (&_i64RequestTimeout);

    // Write _pszGroupName
    uint16 ui16 = (uint16) strlen (_groupName);
    pWriter->write16 (&ui16);
    if (ui16 > 0) {
        pWriter->writeBytes ((const char*)_groupName, ui16);
    }

    pWriter->write16 (&_ui16Tag);
    return 0;
}

//==============================================================================
// DisServiceHistoryRequest::HWinReq
//==============================================================================

DisServiceHistoryRequest::HWinReq::HWinReq (const char *pszGroupName, uint16 ui16Tag, int64 i64RequestTimeout, uint16 ui16HistoryLength)
    : HReq (HT_WIN, pszGroupName, ui16Tag, i64RequestTimeout)
{
    _ui16HistoryLength = ui16HistoryLength;
}

DisServiceHistoryRequest::HWinReq::HWinReq()
    : HReq (HT_WIN)
{
    _ui16HistoryLength = 0;
}

DisServiceHistoryRequest::HWinReq::~HWinReq ()
{
}

int DisServiceHistoryRequest::HWinReq::display (FILE *pFileOut)
{
    if (pFileOut == NULL) {
        return-1;
    }
    if (0 != DisServiceHistoryRequest::HReq::display (pFileOut)) {
        return -1;
    }

    fprintf (pFileOut, "History Length: %u\n", _ui16HistoryLength);
    fflush (pFileOut);

    return 0;
}

int DisServiceHistoryRequest::HWinReq::read (Reader *pReader, uint32 ui32MaxSize)
{
    int rc = HReq::read (pReader, ui32MaxSize);
    if (rc != 0) {
        return rc;
    }

    pReader->read16 (&_ui16HistoryLength);

    return 0;
}

int DisServiceHistoryRequest::HWinReq::write (Writer *pWriter, uint32 ui32MaxSize)
{
    int rc = HReq::write (pWriter, ui32MaxSize);
    if (rc != 0) {
        return rc;
    }

    pWriter->write16 (&_ui16HistoryLength);

    return 0;
}

//==============================================================================
// DisServiceHistoryRequest::HRangeReq
//==============================================================================

DisServiceHistoryRequest::HRangeReq::HRangeReq (const char *pszGroupName, const char *pszSenderNodeId, uint16 ui16Tag, int64 i64RequestTimeout,
                                                uint32 ui32Begin, uint32 ui32End, UInt32RangeDLList *pRanges)
    : HReq (HT_RANGE, strDup (pszGroupName), ui16Tag, i64RequestTimeout)
{
    _senderNodeId = pszSenderNodeId;
    _ui32Begin = ui32Begin;
    _ui32End = ui32End;
    _pRanges = pRanges;
}

DisServiceHistoryRequest::HRangeReq::HRangeReq()
    : HReq (HT_RANGE)
{
    _pRanges = NULL;
}

DisServiceHistoryRequest::HRangeReq::~HRangeReq()
{
    delete _pRanges;
    _pRanges = NULL;
}

int DisServiceHistoryRequest::HRangeReq::display (FILE *pFileOut)
{
    if (pFileOut == NULL) {
        return -1;
    }
    if (0 != DisServiceHistoryRequest::HReq::display (pFileOut)) {
        return -1;
    }

    fprintf (pFileOut, "Sender Node Id: %s\n", (const char*) _senderNodeId);
    fprintf (pFileOut, "Range Begin: %u\n", _ui32Begin);
    fprintf (pFileOut, "Range End: %u\n", _ui32End);
    fflush (pFileOut);

    return 0;
}

int DisServiceHistoryRequest::HRangeReq::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    int rc = HReq::read (pReader, ui32MaxSize);
    if (rc != 0) {
        return rc;
    }

    // Read Sender Node ID
    uint16 ui16;
    pReader->read16 (&ui16);
    char *pszSenderNodeId = new char[ui16+1];
    pReader->readBytes (pszSenderNodeId, ui16);
    pszSenderNodeId[ui16] = '\0';
    _senderNodeId = pszSenderNodeId;
    free (pszSenderNodeId);

    // Write Reuqested Range
    pReader->read32 (&_ui32Begin);
    pReader->read32 (&_ui32End);

    // Read Ranges
    _pRanges = new UInt32RangeDLList (true);
    _pRanges->read (pReader, ui32MaxSize);

    return 0;
}

int DisServiceHistoryRequest::HRangeReq::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    int rc = HReq::write (pWriter, ui32MaxSize);
    if (rc != 0) {
        return rc;
    }

    // Write Sender Node Id
    uint16 ui16 = (uint16) strlen (_senderNodeId);
    pWriter->write16 (&ui16);
    pWriter->writeBytes ((const char*) _senderNodeId, ui16);

    // Write Reuqested Range
    pWriter->write32 (&_ui32Begin);
    pWriter->write32 (&_ui32End);

    // Write Ranges
    BufferWriter bw;
    if ((rc = _pRanges->write (&bw, ui32MaxSize)) == 0) {
        return pWriter->writeBytes (bw.getBuffer(), bw.getBufferLength());
    }

    return -1;
}

//==============================================================================
// DisServiceHistoryRequest::HTmpRangeReq
//==============================================================================

DisServiceHistoryRequest::HTmpRangeReq::HTmpRangeReq (const char *pszGroupName, uint16 ui16Tag, int64 i64RequestTimeout, int64 i64PublishTimeStart, int64 i64PublishTimeEnd)
    : HReq (HT_TMP_RANGE, pszGroupName, ui16Tag, i64RequestTimeout)
{
    _i64PublishTimeStart = i64PublishTimeStart;
    _i64PublishTimeEnd = i64PublishTimeEnd;
}

DisServiceHistoryRequest::HTmpRangeReq::HTmpRangeReq()
    : HReq (HT_TMP_RANGE)
{
    _i64PublishTimeStart = _i64PublishTimeEnd = 0;
}

DisServiceHistoryRequest::HTmpRangeReq::~HTmpRangeReq()
{
}

int DisServiceHistoryRequest::HTmpRangeReq::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    int rc = HReq::read (pReader, ui32MaxSize);
    if (rc != 0) {
        return rc;
    }

    pReader->read64 (&_i64PublishTimeStart);
    pReader->read64 (&_i64PublishTimeEnd);

    return 0;
}

int DisServiceHistoryRequest::HTmpRangeReq::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    int rc = HReq::write (pWriter, ui32MaxSize);
    if (rc != 0) {
        return rc;
    }

    pWriter->write64 (&_i64PublishTimeStart);
    pWriter->write64 (&_i64PublishTimeEnd);

    return 0;
}

//==============================================================================
// DisServiceHistoryRequestReplyMsg
//==============================================================================

DisServiceHistoryRequestReplyMsg::DisServiceHistoryRequestReplyMsg()
    : DisServiceDataCacheQueryReplyMsg (DSMT_HistoryReqReply)
{
    _pMessageIDs = NULL;
    _bDeleteMessageIDs = false;
}


DisServiceHistoryRequestReplyMsg::DisServiceHistoryRequestReplyMsg (const char *pszSenderNodeId, DArray2<String> *pMessageIDs)
    : DisServiceDataCacheQueryReplyMsg (pszSenderNodeId, DSMT_HistoryReqReply)
{
    _pMessageIDs = pMessageIDs;
    _bDeleteMessageIDs = false;
}

DisServiceHistoryRequestReplyMsg::~DisServiceHistoryRequestReplyMsg()
{
    if (_bDeleteMessageIDs) {
        delete _pMessageIDs;
        _pMessageIDs = NULL;
    }
}

//==============================================================================
// ChunkRetrievalMsgQuery
//==============================================================================

ChunkRetrievalMsgQuery::ChunkRetrievalMsgQuery()
    : DisServiceCtrlMsg (CRMT_Query)
{
    _queryId = NULL;
}

ChunkRetrievalMsgQuery::ChunkRetrievalMsgQuery (const char *pszSenderNodeId, const char *pszQueryId,
                                                NOMADSUtil::PtrLList<MessageHeader> *pLocallyCachedChunks)
    : DisServiceCtrlMsg (CRMT_Query, pszSenderNodeId),
      _queryId (pszQueryId)
{
    if (pLocallyCachedChunks != NULL) {
        unsigned int i = 0;
        for (MessageHeader *pMH = pLocallyCachedChunks->getFirst(); pMH != NULL;
             pMH = pLocallyCachedChunks->getNext()) {
            if (pMH->getChunkId() != MessageHeader::UNDEFINED_CHUNK_ID) {
                _locallyCachedChunks[i] = pMH->getChunkId();
                i++;
            }
        }
    }
}

ChunkRetrievalMsgQuery::~ChunkRetrievalMsgQuery()
{
}

int ChunkRetrievalMsgQuery::read (Reader *pReader, uint32 ui32MaxSize)
{
    /**
     * The method DisServiceCtrlMsg::read reads the TargetNodeId,
     * the type of message and SenderNodeId.
     */
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != CRMT_Query) {
        return -2;
    }

    /* Read pszQueryId */
    char *pszTemp = NULL;
    uint16 ui16;
    if (pReader->read16 (&ui16) < 0) {
        return -3;
    }
    pszTemp = (char *) calloc (ui16+1, sizeof (char));
    if (pszTemp == NULL) {
        return -4;
    }
    if (pReader->readBytes (pszTemp, ui16) < 0) {
        return -5;
    }
    pszTemp[ui16] = '\0';
    _queryId = pszTemp;
    free (pszTemp);

    uint8 ui8NChunks = 0;
    if (pReader->read8 (&ui8NChunks) < 0) {
        return -6;
    }
    for (uint8 i = 0, ui8Idx = 0; i < ui8NChunks; i++) {
        uint8 ui8ChunkId = MessageHeader::UNDEFINED_CHUNK_ID;
        if (pReader->read8 (&ui8ChunkId) < 0) {
            return -7;
        }
        if (ui8ChunkId != MessageHeader::UNDEFINED_CHUNK_ID) {
            _locallyCachedChunks[ui8Idx] = ui8ChunkId;
            ui8Idx++;
        }
    }

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int ChunkRetrievalMsgQuery::write (Writer *pWriter, uint32 ui32MaxSize)
{
    /**
     * The method DisServiceCtrlMsg::write writes the TargetNodeId,
     * the type of message and SenderNodeId.
     */
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    /* Write pszQueryId */
    uint16 ui16 = _queryId.length();
    if (pWriter->write16 (&ui16) < 0) {
        return -2;
    }
    if (pWriter->writeBytes (_queryId.c_str(), ui16) < 0) {
        return -3;
    }

    const unsigned int iNChunks = _locallyCachedChunks.size();
    if (iNChunks > 0xFF) {
        return -4;
    }
    const uint8 ui8NChunks = (uint8) iNChunks;
    if (pWriter->write8 (&ui8NChunks) < 0) {
        return -5;
    }
    for (uint8 i = 0; i < ui8NChunks; i++) {
        uint8 ui8ChunkId = _locallyCachedChunks[i];
        if (pWriter->write8 (&ui8ChunkId) < 0) {
            return -6;
        }
    }

    _ui16Size = iw.getBytesWritten();
    return iw.getBytesWritten() <= ui32MaxSize ? 0 : -7;
}

DArray<uint8> * ChunkRetrievalMsgQuery::getLocallyCachedChunks (void)
{
    return &_locallyCachedChunks;
}

const char * ChunkRetrievalMsgQuery::getQueryId() const
{
    return _queryId.c_str();
}

//==============================================================================
// ChunkRetrievalMsgQueryHits
//==============================================================================

ChunkRetrievalMsgQueryHits::ChunkRetrievalMsgQueryHits (bool bDeallocateChunks)
    : DisServiceCtrlMsg (CRMT_QueryHits),
      _bDeallocateChunks (bDeallocateChunks)
{
     _pCMHs = NULL;
}

ChunkRetrievalMsgQueryHits::ChunkRetrievalMsgQueryHits (const char *pszSenderNodeId,
                                                        const char *pszTargetNodeId,
                                                        PtrLList<MessageHeader> *pCMHs,
                                                        const char *pszQueryId, bool bDeallocateChunks)
    : DisServiceCtrlMsg (CRMT_QueryHits, pszSenderNodeId, pszTargetNodeId),
      _bDeallocateChunks (bDeallocateChunks)
{
    _pCMHs = pCMHs;
    _queryId = pszQueryId;
}

ChunkRetrievalMsgQueryHits::~ChunkRetrievalMsgQueryHits()
{
    if (_pCMHs != NULL && _bDeallocateChunks) {
        MessageHeader *pTmp = _pCMHs->getFirst();
        MessageHeader *pMH;
        while ((pMH = pTmp) != NULL) {
            pTmp = _pCMHs->getNext();
            delete _pCMHs->remove (pMH);
        }
        delete _pCMHs;
        _pCMHs = NULL;
    }
}

int ChunkRetrievalMsgQueryHits::read (Reader *pReader, uint32 ui32MaxSize)
{
    /**
     * The method DisServiceCtrlMsg::read reads the TargetNodeId,
     * the type of message and senderNodeId.
     */
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != CRMT_QueryHits) {
        return -2;
    }

    // Read the MessageHeaders
    uint16 ui16;

    // Read pszQueryId
    char *pszTemp = NULL;
    pReader->read16 (&ui16);
    pszTemp = new char[ui16+1];
    pReader->readBytes (pszTemp, ui16);
    pszTemp[ui16] = '\0';
    _queryId = pszTemp;
    delete[] pszTemp;

    // Read group name
    pReader->read16 (&ui16);
    char *pszGroupName = new char [ui16 + 1];
    pReader->readBytes (pszGroupName, ui16);
    pszGroupName[ui16] = '\0';

    // Read sender node id
    pReader->read16 (&ui16);
    char *pszPublisherNodeId = new char [ui16 + 1];
    pReader->readBytes (pszPublisherNodeId, ui16);
    pszPublisherNodeId[ui16] = '\0';

    // Read message sequence id
    uint32 ui32MsgSeqId;
    pReader->read32 (&ui32MsgSeqId);

    // Read object id
    pReader->read16 (&ui16);
    char *pszObjectId = NULL;
    if (ui16 > 0) {
        pszObjectId = new char [ui16 + 1];
        pReader->readBytes (pszObjectId, ui16);
        pszObjectId[ui16] = '\0';
    }

    // Reade instance id
    pReader->read16 (&ui16);
    char *pszInstanceId = NULL;
    if (ui16 > 0) {
        pszInstanceId = new char [ui16 + 1];
        pReader->readBytes (pszInstanceId, ui16);
        pszInstanceId[ui16] = '\0';
    }

    // Read ui8TotalNumOfChunks
    uint8 ui8TotalNumOfChunks;
    pReader->read8 (&ui8TotalNumOfChunks);

    // Read number of available complete chunks at the peer
    uint8 ui8;
    pReader->read8 (&ui8);
    for (uint8 i = 0; i < ui8; i++) {
        uint8 ui8ChunkId;
        pReader->read8 (&ui8ChunkId);

        uint32 ui32TotalMessageLength;
        pReader->read32 (&ui32TotalMessageLength);
        MessageHeader *pMH = new ChunkMsgInfo (pszGroupName, pszPublisherNodeId,
                                               ui32MsgSeqId, ui8ChunkId,
                                               pszObjectId, pszInstanceId,
                                               0,    // ui16Tag
                                               0,    // ui16ClientId
                                               0,    // ui8ClientType,
                                               NULL, // MIME Type
                                               NULL, // Checksum
                                               0,    // ui32FragmentOffset
                                               ui32TotalMessageLength,
                                               ui32TotalMessageLength,
                                               ui8TotalNumOfChunks);
        if (pMH == NULL) {
            checkAndLogMsg ("ChunkRetrievalMsgQueryHits::read", memoryExhausted);
            break;
        }
        else {
            if (_pCMHs == NULL) {
                _pCMHs = new PtrLList<MessageHeader>();
            }
            _pCMHs->prepend (pMH);
        }
    }
    delete[] pszGroupName;
    pszGroupName = NULL;
    delete[] pszPublisherNodeId;
    pszPublisherNodeId = NULL;

    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int ChunkRetrievalMsgQueryHits::write (Writer *pWriter, uint32 ui32MaxSize)
{
    /**
     * The method DisServiceCtrlMsg::write writes the TargetNodeId,
     * the type of message and senderNodeId.
     */
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }
    if (_pCMHs == NULL) {
        return -2;
    }

    // Write pszQueryId
    uint16 ui16 = _queryId.length();
    iw.write16 (&ui16);
    iw.writeBytes (_queryId.c_str(), ui16);

    MessageHeader *pMH = _pCMHs->getFirst();
    if (pMH == NULL) {
        return -3;
    }

    // Write the Message Header
    String tmp = pMH->getGroupName();
    ui16 = tmp.length();
    iw.write16 (&ui16);
    iw.writeBytes ((const char *) tmp, ui16);

    tmp = pMH->getPublisherNodeId();
    ui16 = tmp.length();
    iw.write16 (&ui16);
    iw.writeBytes ((const char *) tmp, ui16);

    uint32 ui32 = pMH->getMsgSeqId();
    iw.write32 (&ui32);

    tmp = pMH->getObjectId();
    ui16 = (tmp.length() <= 0 ? 0 : tmp.length());
    iw.write16 (&ui16);
    if (ui16 > 0) {
        iw.writeBytes ((const char *) tmp, ui16);
    }

    tmp = pMH->getInstanceId();
    ui16 = (tmp.length() <= 0 ? 0 : tmp.length());
    iw.write16 (&ui16);
    if (ui16 > 0) {
        iw.writeBytes ((const char *) tmp, ui16);
    }

    // Write total number of chunks
    uint8 ui8 = 0;
    if (pMH->isChunk()) {
        ui8 = ((ChunkMsgInfo*)pMH)->getTotalNumberOfChunks();
    }
    else {
        ui8 = 1;
    }
    if (ui8 == 0) {
        return -4;
    }
    iw.write8 (&ui8);

    // Write the list of the chunks in the local data cache
    ui8 = _pCMHs->getCount();
    iw.write8 (&ui8);
    do {
        ui8 = pMH->getChunkId();
        iw.write8 (&ui8);

        ui32 = pMH->getTotalMessageLength();
        iw.write32 (&ui32);

    } while ((pMH = _pCMHs->getNext()) != NULL);

    _ui16Size = iw.getBytesWritten();
    return iw.getBytesWritten() <= ui32MaxSize ? 0 : -5;
}

PtrLList<MessageHeader> * ChunkRetrievalMsgQueryHits::getMessageHeaders()
{
    return _pCMHs;
}

const char *ChunkRetrievalMsgQueryHits::getQueryId()
{
    return _queryId.c_str();
}

//==============================================================================
// DisServiceSubscribtionAdvertisement CONTROL
//==============================================================================

DisServiceSubscribtionAdvertisement::DisServiceSubscribtionAdvertisement()
    : DisServiceCtrlMsg (DisServiceMsg::DSMT_SubAdvMessage)
{
}

DisServiceSubscribtionAdvertisement::DisServiceSubscribtionAdvertisement (const char *pszSenderNodeId, const char *pszOriginatorNodeId,
                                                                          NOMADSUtil::PtrLList<String> *pSubList)
    : DisServiceCtrlMsg (DisServiceMsg::DSMT_SubAdvMessage, pszSenderNodeId),
      _subscriptionList (*pSubList)

{
    _originatorNodeId = pszOriginatorNodeId;
}

DisServiceSubscribtionAdvertisement::DisServiceSubscribtionAdvertisement (const char *pszSenderNodeId, const char *pszOriginatorNodeId,
                                                                          NOMADSUtil::PtrLList<String> *pSubList,
                                                                          NOMADSUtil::PtrLList<String> *pPath)
    : DisServiceCtrlMsg (DisServiceMsg::DSMT_SubAdvMessage, pszSenderNodeId),
      _subscriptionList (*pSubList),
      _path (*pPath)

{
    _originatorNodeId = pszOriginatorNodeId;
}

DisServiceSubscribtionAdvertisement::~DisServiceSubscribtionAdvertisement()
{
}

int DisServiceSubscribtionAdvertisement::addSubscription (const char *pszSubscription)
{
    if (pszSubscription == NULL) {
        return -1;
    }

    String *pszSub = new String (pszSubscription);
    if (pszSub == NULL) {
        return -1;
    }
    _subscriptionList.append (pszSub);

    return 0;
}

int DisServiceSubscribtionAdvertisement::removeSubscription (const char *pszSubscription)
{
    if (pszSubscription == NULL) {
        return -1;
    }

    String sSub (pszSubscription);
    String *pszRet = _subscriptionList.remove (&sSub);
    if (pszRet == NULL) {
        return -1;
    }
    delete pszRet;

    return 0;
}

int DisServiceSubscribtionAdvertisement::prependNode (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        return -1;
    }

    String *pNewString = new String (pszNodeId);
    if (pNewString == NULL) {
        return -1;
    }
    _path.prepend (pNewString);

    return 0;
}

NOMADSUtil::PtrLList<String> * DisServiceSubscribtionAdvertisement::getSubscriptions()
{
    return &_subscriptionList;
}

NOMADSUtil::PtrLList<String> * DisServiceSubscribtionAdvertisement::getPath()
{
    return &_path;
}

const char * DisServiceSubscribtionAdvertisement::getOriginatorNodeId()
{
    return _originatorNodeId.c_str();
}

int DisServiceSubscribtionAdvertisement::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }

    if (_type != DisServiceMsg::DSMT_SubAdvMessage) {
        return -2;
    }

    // Read OriginatorNodeId
    uint16 ui16OriginatorLength;
    pReader->read16 (&ui16OriginatorLength);
    char *pszOriginator = new char [ui16OriginatorLength+1];
    pReader->readBytes (pszOriginator, ui16OriginatorLength);
    pszOriginator[ui16OriginatorLength] = '\0';
    _originatorNodeId = pszOriginator;
    delete[] pszOriginator;

    /* Read the Subscriptions */
    uint8 ui8SubNum;
    pReader->read8 (&ui8SubNum);
    if (ui8SubNum == 0) {
        return -1;
    }
    /* Read Subscriptions */
    for (int i=0; i<ui8SubNum; i++) {
        /* Read Subscription */
        uint16 ui16Length;
        pReader->read16 (&ui16Length);
        char *pszSub = new char [ui16Length+1];
        pReader->readBytes (pszSub, ui16Length);
        pszSub[ui16Length] = '\0';
        _subscriptionList.append (new String (pszSub));
        delete[] pszSub;
    }

    /* Read Path */
    uint16 ui16Nodes;
    pReader->read16 (&ui16Nodes);

    for (int j = 0; j < ui16Nodes; j++) {
        uint16 ui16Length;
        pReader->read16 (&ui16Length);
        char *pszNode = new char [ui16Length+1];
        pReader->readBytes (pszNode, ui16Length);
        pszNode[ui16Length] = '\0';
        _path.append (new String (pszNode));
        delete[] pszNode;
    }

    // If the originator is not included yet add to the path
    if (_path.search (&_originatorNodeId) == NULL) {
        _path.append (&_originatorNodeId);
    }

    return 0;
}

int DisServiceSubscribtionAdvertisement::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    /* Write Type, CtrlMsgSeqNo, SenderNodeId */
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }

    // Write Originator Node Id
    uint16 ui16OriginatorLength = _originatorNodeId.length();
    iw.write16 (&ui16OriginatorLength);
    iw.writeBytes (_originatorNodeId.c_str(), ui16OriginatorLength);

    /* Write Subscriptions */
    uint8 ui8SubNum = _subscriptionList.getCount();
    if (ui8SubNum == 0) {
        return -1;
    }
    iw.write8 (&ui8SubNum);
    for (String *pszSub = _subscriptionList.getFirst(); pszSub != NULL;
         pszSub = _subscriptionList.getNext()) {

        uint16 ui16Length = pszSub->length();
        iw.write16 (&ui16Length);
        iw.writeBytes (pszSub->c_str(), ui16Length);
    }

    /* Write Path if any */
    uint16 ui16Nodes = _path.getCount();
    iw.write16 (&ui16Nodes);
    for (String *pszNode = _path.getFirst(); pszNode != NULL;
         pszNode = _path.getNext()) {

        uint16 ui16Length = pszNode->length();
        iw.write16 (&ui16Length);
        iw.writeBytes (pszNode->c_str(), ui16Length);
    }

    return 0;
}

void DisServiceSubscribtionAdvertisement::display()
{
    //Print type
    String sDisplay = "";
    if (_type == DSMT_SubAdvMessage) {
        sDisplay += "DSMT_SubAdvMsg:";
    }
    sDisplay += _senderNodeId;
    sDisplay += ":";
    char *pszCtrlMsgSeq = new char [11];
    itoa (pszCtrlMsgSeq, getCtrlMsgSeqNo());
    pszCtrlMsgSeq[10] = '\0';
    sDisplay += pszCtrlMsgSeq;
    free (pszCtrlMsgSeq);
    sDisplay += ":";
    sDisplay += _originatorNodeId;
    sDisplay += ":";
    for (String *pszSub = _subscriptionList.getFirst(); pszSub != NULL;
         pszSub = _subscriptionList.getNext()) {

        sDisplay += pszSub->c_str();
        sDisplay += ":";
    }

    for (String *pszNode = _path.getFirst(); pszNode != NULL;
         pszNode = _path.getNext()) {

        sDisplay += pszNode->c_str();
        sDisplay += ":";
    }

    printf ("%s\n", sDisplay.c_str());
}

//==============================================================================
// ControllerToControllerMsg
//==============================================================================

ControllerToControllerMsg::ControllerToControllerMsg()
    : DisServiceMsg (DSMT_CtrlToCtrlMessage)
{
    _senderNodeId = NULL;
    _receiverNodeID = NULL;
    _pMetaData = NULL;
    _pData = NULL;
}

ControllerToControllerMsg::ControllerToControllerMsg (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                                      uint8 ui8CtrlType, uint8 ui8CtrlVersion, void *pMetaData,
                                                      uint32 ui32MetaDataLength, void *pData, uint32 ui32DataLength)
    : DisServiceMsg (DSMT_CtrlToCtrlMessage, pszSenderNodeID)
{
    _receiverNodeID = pszReceiverNodeID;
    _ui8CtrlType = ui8CtrlType;
    _ui8CtrlVersion = ui8CtrlVersion;
    _pMetaData = pMetaData;
    _ui32MetaDataLength = (_pMetaData == NULL ? 0 : ui32MetaDataLength);
    _pData = pData;
    _ui32DataLength = (_pData == NULL ? 0 : ui32DataLength);;
}

ControllerToControllerMsg::~ControllerToControllerMsg()
{
    free (_pMetaData);
    _pMetaData = NULL;
    free (_pData);
    _pData = NULL;
}

int ControllerToControllerMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }
    if (DisServiceMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    uint32 ui32TotLength = 0;
    uint8 ui8Length = 0;
    if (ui32MaxSize < (ui32TotLength + 1)) {
        return -2;
    }
    int rc = pReader->read8 (&ui8Length);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 1;
    if (ui32MaxSize < (ui32TotLength + ui8Length)) {
        return -2;
    }
    char *pBuffer = new char[ui8Length + 1];
    if ((rc = pReader->readBytes (pBuffer, ui8Length)) < 0) {
        delete[] pBuffer;
        pBuffer = NULL;
        return rc;
    }
    pBuffer[ui8Length] = '\0';
    _receiverNodeID = pBuffer;  // NOMADSUtil::String = operator make a strDup
    delete[] pBuffer;           // of the passed const char *. Thus I can delete
    pBuffer = NULL;             // the content of pBuffer.
    ui32TotLength += ui8Length;
    if (ui32MaxSize < (ui32TotLength + 1)) {
        return -2;
    }
    if ((rc = pReader->read8 (&_ui8CtrlType)) < 0) {
        return rc;
    }
    ui32TotLength += 1;
    if (ui32MaxSize < (ui32TotLength + 1)) {
        return -2;
    }
    if ((rc = pReader->read8 (&_ui8CtrlVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 1;
    if (ui32MaxSize < (ui32TotLength + 4)) return -2;
    rc = pReader->read32 (&_ui32MetaDataLength);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if(_ui32MetaDataLength != 0) {
        if (ui32MaxSize < (ui32TotLength + _ui32MetaDataLength)) {
            return -2;
        }
        _pMetaData = calloc (_ui32MetaDataLength, sizeof(char));
        if ((rc = pReader->readBytes ((char *) _pMetaData, _ui32MetaDataLength)) < 0) {
            free(_pMetaData);
            _pMetaData = NULL;
        }
    }
    ui32TotLength += _ui32MetaDataLength;
    if (ui32MaxSize < (ui32TotLength + 4)) {
        return -2;
    }
    if ((rc = pReader->read32 (&_ui32DataLength)) < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if (_ui32DataLength != 0) {
        if(ui32MaxSize < (ui32TotLength + _ui32DataLength)) {
            return -2;
        }
        _pData = calloc (_ui32DataLength, sizeof(char));
        if ((rc = pReader->readBytes ((char *) _pData, _ui32DataLength)) < 0) {
            free(_pData);
            _pData = NULL;
        }
    }
    ui32TotLength += _ui32DataLength;
    return 0;
}

int ControllerToControllerMsg::write (NOMADSUtil::Writer *pWriter)
{
    return writeInternal (pWriter, 0, false);
}

int ControllerToControllerMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    return writeInternal (pWriter, ui32MaxSize, true);
}

int ControllerToControllerMsg::writeInternal (Writer *pWriter, uint32 ui32MaxSize, bool bCheckSize)
{
    if (pWriter == NULL) {
        return -1;
    }
    if ((_receiverNodeID == NULL) || (_senderNodeId == NULL)) {
        return -1;
    }
    if (DisServiceMsg::write (pWriter, ui32MaxSize) != 0) {
        return -1;
    }

    uint8 ui8Length = _receiverNodeID.length();
    if (bCheckSize && ui32MaxSize < 1) {
        return -2;
    }
    int rc = pWriter->write8 (&ui8Length);
    if (rc < 0) {
        return rc;
    }

    uint32 ui32TotLength = 1;
    rc = pWriter->writeBytes ((const char *) _receiverNodeID, ui8Length);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += ui8Length;
    if (bCheckSize && ui32MaxSize < (ui32TotLength + 1)) {
        return -2;
    }
    rc = pWriter->write8 (&_ui8CtrlType);
    if (rc < 0) return rc;
    ui32TotLength += 1;
    if (bCheckSize && ui32MaxSize < (ui32TotLength + 1)) {
        return -2;
    }
    rc = pWriter->write8 (&_ui8CtrlVersion);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 1;
    if (bCheckSize && ui32MaxSize < ui32TotLength + _ui32MetaDataLength) {
        return -2;
    }
    rc = pWriter->write32 (&_ui32MetaDataLength);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if (_ui32MetaDataLength != 0) {
        rc = pWriter->writeBytes ((const char *) _pMetaData, _ui32MetaDataLength);
        if (rc < 0) {
            return rc;
        }
    }
    ui32TotLength += _ui32MetaDataLength;
    if (bCheckSize && ui32MaxSize < ui32TotLength + _ui32DataLength) {
        return -2;
    }
    rc = pWriter->write32 (&_ui32DataLength);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if (_ui32DataLength != 0) {
        rc = pWriter->writeBytes ((const char *) _pData, _ui32DataLength);
        if (rc < 0) {
            return rc;
        }
    }
    ui32TotLength += _ui32DataLength;
    return 0;
}

const char * ControllerToControllerMsg::getReceiverNodeID()
{
    return _receiverNodeID;
}

uint8 ControllerToControllerMsg::getControllerType()
{
    return _ui8CtrlType;
}

uint8 ControllerToControllerMsg::getControllerVersion()
{
    return _ui8CtrlVersion;
}

void * ControllerToControllerMsg::getMetaData()
{
    return _pMetaData;
}

uint32 ControllerToControllerMsg::getMetaDataLength()
{
    return _ui32MetaDataLength;
}

void * ControllerToControllerMsg::getData()
{
    return _pData;
}

uint32 ControllerToControllerMsg::getDataLength()
{
    return _ui32DataLength;
}


//==============================================================================
// TargetBasedReplicationController Messages
//==============================================================================

ReplicationStartReqMsg::ReplicationStartReqMsg (ControllerToControllerMsg *pMsg)
{
    if ((pMsg->getMetaDataLength() != 1) || (((uint8*)pMsg->getMetaData())[0] != ControllerToControllerMsg::DSCTCMT_RepStartReq)) {
        checkAndLogMsg ("ReplicationStartReqMsg::ReplicationStartReqMsg", Logger::L_MildError,
                        "incorrect ControllerToControllerMsg\n");
        return;
    }
    if (pMsg->getDataLength() != 2) {
        checkAndLogMsg ("ReplicationStartReqMsg::ReplicationStartReqMsg", Logger::L_MildError,
                        "incorrect data length for a ReplicationStartReqMsg\n");
        return;
    }
    _bSendCurrentDataList = ((uint8*)pMsg->getData())[0] == 1 ? true : false;
    _bRequireAcks = ((uint8*)pMsg->getData())[1] == 1 ? true : false;
}

ReplicationStartReqMsg::ReplicationStartReqMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                                                uint8 ui8CtrlType, uint8 ui8CtrlVersion, bool bSendCurrentDataList, bool bRequireAcks)
    : ControllerToControllerMsg (pszSenderNodeId, pszReceiverNodeId, ui8CtrlType, ui8CtrlVersion, NULL, 0, NULL, 0)
{
    _bSendCurrentDataList = bSendCurrentDataList;
    _bRequireAcks = bRequireAcks;

    // Store the type of the message as metadata
    _ui32MetaDataLength = 1;
    _pMetaData = malloc (sizeof (uint8));
    ((uint8*)_pMetaData)[0] = ControllerToControllerMsg::DSCTCMT_RepStartReq;

    // Store the two boolean flags as the data for this message
    _ui32DataLength = 2;
    _pData = malloc (sizeof (uint8)*2);
    ((uint8*)_pData)[0] = bSendCurrentDataList ? 1 : 0;
    ((uint8*)_pData)[1] = bRequireAcks ? 1 : 0;
}

bool ReplicationStartReqMsg::sendCurrentDataList (void)
{
    return _bSendCurrentDataList;
}

bool ReplicationStartReqMsg::requireAcks (void)
{
    return _bRequireAcks;
}

ReplicationStartReplyMsg::ReplicationStartReplyMsg (ControllerToControllerMsg *pMsg)
{
    _pReceivedMsgsList = NULL;
    if ((pMsg->getMetaDataLength() != 1) || (((uint8*)pMsg->getMetaData())[0] != ControllerToControllerMsg::DSCTCMT_RepStartReply)) {
        checkAndLogMsg ("ReplicationStartReplyMsg::ReplicationStartReplyMsg", Logger::L_MildError,
                        "incorrect ControllerToControllerMsg\n");
        return;
    }
    int rc;
    if (pMsg->getDataLength() > 0) {
        BufferReader br (pMsg->getData(), pMsg->getDataLength());
        _pReceivedMsgsList = new StringHashtable<ReceivedMessages::ReceivedMsgsByGrp>;
        if ((rc = readMessageList (&br, _pReceivedMsgsList)) < 0) {
            checkAndLogMsg ("ReplicationStartReplyMsg::ReplicationStartReplyMsg", Logger::L_MildError,
                            "failed to read message list; rc = %d\n", rc);
        }
    }
}

ReplicationStartReplyMsg::ReplicationStartReplyMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                                                    uint8 ui8CtrlType, uint8 ui8CtrlVersion,
                                                    StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgsList)
    : ControllerToControllerMsg (pszSenderNodeId, pszReceiverNodeId, ui8CtrlType, ui8CtrlVersion, NULL, 0, NULL, 0)
{
    // Store the type of the message as metadata
    _ui32MetaDataLength = 1;
    _pMetaData = malloc (sizeof (uint8));
    ((uint8*)_pMetaData)[0] = ControllerToControllerMsg::DSCTCMT_RepStartReply;

    _pReceivedMsgsList = pReceivedMsgsList;
}

ReplicationStartReplyMsg::~ReplicationStartReplyMsg (void)
{
    if (_pReceivedMsgsList != NULL) {
        delete _pReceivedMsgsList;
        _pReceivedMsgsList = NULL;
    }
}

StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> * ReplicationStartReplyMsg::relinquishReceivedMsgsList (void)
{
    StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pMsgsList = _pReceivedMsgsList;
    _pReceivedMsgsList = NULL;
    return pMsgsList;
}

int ReplicationStartReplyMsg::write (NOMADSUtil::Writer *pWriter)
{
    int rc;

    if (_pReceivedMsgsList != NULL) {
        // Set data
        BufferWriter bw;
        if ((rc = writeMessageList (&bw, _pReceivedMsgsList)) < 0) {
            return rc;
        }
        _ui32DataLength = bw.getBufferLength();
        _pData = bw.relinquishBuffer();
    }
    return ControllerToControllerMsg::write (pWriter);
}

int ReplicationStartReplyMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    int rc;

    if (_pReceivedMsgsList != NULL) {
        // Set data
        BufferWriter bw;
        if ((rc = writeMessageList (&bw, _pReceivedMsgsList)) < 0) {
            return rc;
        }
        _ui32DataLength = bw.getBufferLength();
        _pData = bw.relinquishBuffer();
    }
    return ControllerToControllerMsg::write (pWriter, ui32MaxSize);
}

int ReplicationStartReplyMsg::writeMessageList (NOMADSUtil::BufferWriter *pWriter, NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgsList)
{
    // Serializes structure ReceivedMsgsByGroup
    if (pWriter == NULL) {
        return -1;
    }
    if (pReceivedMsgsList == NULL) {
        return -2;
    }

    // Format: grpNameLen|grpName|pubNodeIdLen|pubNodeId|ranges|grpName...
    // The lists of ranges are serialized using the method NOMADSUtil::UInt32RangeDLList::write()
    uint16 ui16Term = 0;
    uint16 ui16GrpTerm = 0xFFFF;

    for (StringHashtable<ReceivedMessages::ReceivedMsgsByGrp>::Iterator byGrpIter = pReceivedMsgsList->getAllElements(); !byGrpIter.end(); byGrpIter.nextElement()) {
        // For each group name of the list pRecMsgsByGroup
        const char *pszGrpName = byGrpIter.getKey();
        if (pszGrpName == NULL) {
            return -3;
        }
        uint16 ui16GrpNameLen = (uint16) strlen (pszGrpName);
        if (ui16GrpNameLen <= 0) {
            return -4;
        }

        // Write group name info
        pWriter->write16 (&ui16GrpNameLen);
        checkAndLogMsg ("ReplicationStartReplyMsg::writeMessageList", Logger::L_LowDetailDebug,
                        "writing group name len: %u\n", ui16GrpNameLen);
        pWriter->writeBytes (pszGrpName, ui16GrpNameLen);
        checkAndLogMsg ("ReplicationStartReplyMsg::writeMessageList", Logger::L_Info,
                        "writing group name: %s\n", pszGrpName);

        for (StringHashtable<ReceivedMessages::ReceivedMsgsByPub>::Iterator byPubIter = byGrpIter.getValue()->msgsByPub.getAllElements(); !byPubIter.end(); byPubIter.nextElement()) {
            // For each publisher node ID of the group pMBGTmp->groupName
            const char *pszPubNodeId = byPubIter.getKey();
            if (pszPubNodeId == NULL) {
                return -5;   // to next publisherNodeId
            }
            uint16 ui16PubNodeIdLen = (uint16) strlen (pszPubNodeId);
            if (ui16PubNodeIdLen <= 0) {
                return -6;   // to next publisherNodeId
            }

            // Write publisherNodeId info
            pWriter->write16 (&ui16PubNodeIdLen);
            checkAndLogMsg ("ReceivedMsgsInfo::writeByGroupList", Logger::L_LowDetailDebug,
                            "writing pub id len: %u\n", ui16PubNodeIdLen);
            pWriter->writeBytes (pszPubNodeId, ui16PubNodeIdLen);
            checkAndLogMsg ("ReceivedMsgsInfo::writeByGroupList", Logger::L_LowDetailDebug,
                            "writing pub id: %s\n", pszPubNodeId);

            // Write ranges
            if (byPubIter.getValue()->ranges.write (pWriter, 0xFFFFFFFF) < 0) {
                return -7;
            }
        }
        // To distinguish the end of a group, write a pubNodeId length of "-1"
        pWriter->write16 (&ui16GrpTerm);

    }
    // Terminate the buffer with a groupName length of "0"
    pWriter->write16 (&ui16Term);

    return 0;
}

int ReplicationStartReplyMsg::readMessageList (NOMADSUtil::BufferReader *pReader, NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgsList)
{
    // Deserializes a buffer into a ReceivedMsgsByGrp
    if (pReader == NULL) {
        return -1;
    }
    if (pReceivedMsgsList == NULL) {
        return -2;
    }

    // Format: grpNameLen|grpName|pubNodeIdLen|pubNodeId|ranges
    // The lists of ranges are deserialized using the method NOMADSUtil::UInt32RangeDLList::read()
    uint16 ui16Term = 0;
    uint16 ui16GrpTerm = 0xFFFF;

    uint16 ui16GrpNameLen = 0;
    while ((pReader->read16 (&ui16GrpNameLen) == 0) && (ui16GrpNameLen != ui16Term)) {
        ReceivedMessages::ReceivedMsgsByGrp *byGrpTmp = new ReceivedMessages::ReceivedMsgsByGrp();
        // Read group name
        char *pszGrpName = (char *) calloc (ui16GrpNameLen+1, sizeof (char));
        if ((pszGrpName == NULL) || (pReader->readBytes (pszGrpName, ui16GrpNameLen) != 0)) {
            return -3;
        }
        byGrpTmp->groupName = pszGrpName;
        checkAndLogMsg ("ReceivedMsgsInfo::readByGroupList", Logger::L_LowDetailDebug,
                        "read group name: %s\n", pszGrpName);

        uint16 ui16PubNodeIdLen = 0;
        while ((pReader->read16 (&ui16PubNodeIdLen) == 0) && (ui16PubNodeIdLen != ui16GrpTerm)) {
            ReceivedMessages::ReceivedMsgsByPub *byPubTmp = new ReceivedMessages::ReceivedMsgsByPub();
            // Read pub node id
            char *pszPubNodeId = (char *) calloc (ui16PubNodeIdLen+1, sizeof (char));
            if ((pszPubNodeId == NULL) || (pReader->readBytes (pszPubNodeId, ui16PubNodeIdLen) != 0)) {
                return -4;
            }
            byPubTmp->publisherNodeId = pszPubNodeId;
            checkAndLogMsg ("ReceivedMsgsInfo::readByGroupList", Logger::L_LowDetailDebug,
                            "read group name: %s\n", pszPubNodeId);

            // Read ranges
            if (byPubTmp->ranges.read (pReader, 0xFFFFFFFF) < 0) {
                return -5;
            }

            byGrpTmp->msgsByPub.put (pszPubNodeId, byPubTmp);
        }
        pReceivedMsgsList->put (pszGrpName, byGrpTmp);
    }

    return 0;
}

ReplicationEndMsg::ReplicationEndMsg (void)
{
}

ReplicationEndMsg::ReplicationEndMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                                      uint8 ui8CtrlType, uint8 ui8CtrlVersion)
    : ControllerToControllerMsg (pszSenderNodeId, pszReceiverNodeId, ui8CtrlType, ui8CtrlVersion, NULL, 0, NULL, 0)
{
    // Store the type of the message as metadata
    _ui32MetaDataLength = 1;
    _pMetaData = malloc (sizeof (uint8));
    ((uint8*)_pMetaData)[0] = ControllerToControllerMsg::DSCTCMT_RepEnd;
}

ReplicationAckMsg::ReplicationAckMsg (ControllerToControllerMsg *pMsg)
{
    if ((pMsg->getMetaDataLength() != 1) || (((uint8*)pMsg->getMetaData())[0] != ControllerToControllerMsg::DSCTCMT_RepAck)) {
        checkAndLogMsg ("ReplicationAckMsg::ReplicationAckMsg", Logger::L_MildError,
                        "incorrect ControllerToControllerMsg\n");
        return;
    }
    if (pMsg->getDataLength() > 0) {
        BufferReader br (pMsg->getData(), pMsg->getDataLength());
        uint8 ui8IDLen;
        while (0 == br.read8 (&ui8IDLen)) {
            char szBuf[256];
            if (ui8IDLen == 0) {
                break;
            }
            if (br.readBytes (szBuf, ui8IDLen)) {
                break;
            }
            szBuf[ui8IDLen] = '\0';
            String *pMsgID = new String (szBuf);
            _msgIDs.enqueue (pMsgID);
        }
    }
}

ReplicationAckMsg::ReplicationAckMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                                      uint8 ui8CtrlType, uint8 ui8CtrlVersion, PtrQueue<String> *pMsgIDsToAck)
    : ControllerToControllerMsg (pszSenderNodeId, pszReceiverNodeId, ui8CtrlType, ui8CtrlVersion, NULL, 0, NULL, 0)
{
    // Store the type of the message as metadata
    _ui32MetaDataLength = 1;
    _pMetaData = malloc (sizeof (uint8));
    ((uint8*)_pMetaData)[0] = ControllerToControllerMsg::DSCTCMT_RepAck;

    // Store the message ids as the data
    if ((pMsgIDsToAck == NULL) || (pMsgIDsToAck->isEmpty())) {
        return;
    }
    String *pMsgID;
    BufferWriter bw;
    while (NULL != (pMsgID = pMsgIDsToAck->dequeue())) {
        if (pMsgID->length() > 255) {
            checkAndLogMsg ("ReplicationAckMsg::ReplicationAckMsg", Logger::L_MildError,
                            "message id <%s> is longer than 255 characters - cannot write into ack message\n",
                            pMsgID->c_str());
            delete pMsgID;
            continue;
        }
        uint8 ui8Length = (uint8) pMsgID->length();
        bw.write8 (&ui8Length);
        bw.writeBytes (pMsgID->c_str(), ui8Length);
        delete pMsgID;
    }
    uint8 ui8End = 0;
    bw.write8 (&ui8End);
    _ui32DataLength = bw.getBufferLength();
    _pData = bw.relinquishBuffer();
}

PtrQueue<String> * ReplicationAckMsg::getMsgIDs (void)
{
    return &_msgIDs;
}


//==============================================================================
// SearchMsg
//==============================================================================

SearchMsg::SearchMsg()
    : DisServiceCtrlMsg (DSMT_SearchMsg)
{
    _pQuery = NULL;
    _uiQueryLen = 0;
    _pszQueryId = NULL;
    _pszQuerier = NULL;
    _pszGroupName = NULL;
    _pszQueryType = NULL;
    _pszQueryQualifiers = NULL;
}

SearchMsg::SearchMsg (const char *pszSenderNodeId, const char *pszTargetNodeId)
    : DisServiceCtrlMsg (DSMT_SearchMsg, pszSenderNodeId, pszTargetNodeId)
{
    _pQuery = NULL;
    _uiQueryLen = 0;
    _pszQueryId = NULL;
    _pszQuerier =  NULL;
    _pszGroupName = NULL;
    _pszQueryType = NULL;
    _pszQueryQualifiers = NULL;
}

SearchMsg::~SearchMsg (void)
{
    if (_pQuery != NULL) {
        free (_pQuery);
        _uiQueryLen = 0;
    }
    if (_pszQueryId != NULL) {
        free (_pszQueryId);
        _pszQueryId = NULL;
    }
    if (_pszQuerier != NULL) {
        free (_pszQuerier);
        _pszQuerier = NULL;
    }
    if (_pszGroupName != NULL) {
        free (_pszGroupName);
        _pszGroupName = NULL;
    }
    if (_pszQueryType != NULL) {
        free (_pszQueryType);
        _pszQueryType = NULL;
    }
    if (_pszQueryQualifiers != NULL) {
        free (_pszQueryQualifiers);
        _pszQueryQualifiers = NULL;
    }
}

const void * SearchMsg::getQuery (unsigned int &uiQueryLen) const
{
    uiQueryLen = _uiQueryLen;
    return (const void*) _pQuery;
}

const char * SearchMsg::getQueryId (void) const
{
    return _pszQueryId;
}

const char * SearchMsg::getQuerier (void) const
{
    return _pszQuerier;
}

const char * SearchMsg::getGroupName (void) const
{
    return _pszGroupName;
}

const char * SearchMsg::getQueryType (void) const
{
    return _pszQueryType;
}

const char * SearchMsg::getQueryQualifier (void) const
{
    return _pszQueryQualifiers;
}

int SearchMsg::setQuery (const void *pQuery, unsigned int uiQueryLen)
{
    if (pQuery == NULL || uiQueryLen == 0) {
        return -1;
    }
    _pQuery = malloc (uiQueryLen);
    if (_pQuery == NULL) {
        return -2;
    }
    memcpy (_pQuery, pQuery, uiQueryLen);
    _uiQueryLen = uiQueryLen;
    return 0;
}

int SearchMsg::setQueryId (const char *pszQueryId)
{
    if (pszQueryId == NULL) {
        return -1;
    }
    _pszQueryId = strDup (pszQueryId);
    return 0;
}

int SearchMsg::setQuerier (const char *pszQuerier)
{
    if (pszQuerier == NULL) {
        return -1;
    }
    _pszQuerier = strDup (pszQuerier);
    return 0;
}

int SearchMsg::setGroupName (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return -1;
    }
    _pszGroupName = strDup (pszGroupName);
    return 0;
}

int SearchMsg::setQueryType (const char *pszQueryType)
{
    if (pszQueryType == NULL) {
        return -1;
    }
    _pszQueryType = strDup (pszQueryType);
    return 0;
}

int SearchMsg::setQueryQualifier (const char *pszQueryQualifiers)
{
    if (pszQueryQualifiers == NULL) {
        return -1;
    }
    _pszQueryQualifiers = strDup (pszQueryQualifiers);
    return 0;
}

int SearchMsg::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -2;
    }

    uint16 ui16;
    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQueryId = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQueryId == NULL) {
                return -3;
            }
            if (pReader->readBytes (_pszQueryId, ui16) < 0) {
                return -4;
            }
            _pszQueryId[ui16] = '\0';
        }
    }
    else {
        return -5;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQuerier = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQuerier == NULL) {
                return -6;
            }
            if (pReader->readBytes (_pszQuerier, ui16) < 0) {
                return -7;
            }
            _pszQuerier[ui16] = '\0';
        }
    }
    else {
        return -8;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszGroupName = (char *) calloc (ui16+1, sizeof (char));
            if (_pszGroupName == NULL) {
                return -9;
            }
            if (pReader->readBytes (_pszGroupName, ui16)) {
                return -10;
            }
            _pszGroupName[ui16] = '\0';
        }
    }
    else {
        return -11;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQueryType = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQueryType == NULL) {
                return -12;
            }
            if (pReader->readBytes (_pszQueryType, ui16)) {
                return -13;
            }
            _pszQueryType[ui16] = '\0';
        }
    }
    else {
        return -14;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQueryQualifiers = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQueryQualifiers == NULL) {
                return -15;
            }
            if (pReader->readBytes (_pszQueryQualifiers, ui16)) {
                return -16;
            }
            _pszQueryQualifiers[ui16] = '\0';
        }
    }
    else {
        return -17;
    }

    if (pReader->read32 (&_uiQueryLen) == 0) {
        if (_uiQueryLen > 0) {
            _pQuery = malloc (_uiQueryLen);
            if (_pQuery == NULL) {
                return -18;
            }
            if (pReader->readBytes (_pQuery, _uiQueryLen)) {
                return -19;
            }
        }
    }
    else {
        return -20;
    }

    return 0;
}

int SearchMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }

    if (DisServiceCtrlMsg::write (pWriter, ui32MaxSize) != 0) {
        return -2;
    }

    uint16 ui16 = _pszQueryId == NULL ? 0 : (uint16) strlen (_pszQueryId);
    if (pWriter->write16 (&ui16) < 0) {
        return -3;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQueryId, ui16) < 0) {
        return -4;
    }

    ui16 = _pszQuerier == NULL ? 0 : (uint16) strlen (_pszQuerier);
    if (pWriter->write16 (&ui16) < 0) {
        return -5;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQuerier, ui16) < 0) {
        return -6;
    }

    ui16 = _pszGroupName == NULL ? 0 : (uint16) strlen (_pszGroupName);
    if (pWriter->write16 (&ui16) < 0) {
        return -7;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszGroupName, ui16) < 0) {
        return -8;
    }

    ui16 = _pszQueryType == NULL ? 0 : (uint16) strlen (_pszQueryType);
    if (pWriter->write16 (&ui16) < 0) {
        return -9;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQueryType, ui16) < 0) {
        return -10;
    }

    ui16 = _pszQueryQualifiers == NULL ? 0 : (uint16) strlen (_pszQueryQualifiers);
    if (pWriter->write16 (&ui16) < 0) {
        return -11;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQueryQualifiers, ui16) < 0) {
        return -12;
    }

    if (pWriter->write32 (&_uiQueryLen) < 0) {
        return -13;
    }
    if (_uiQueryLen > 0 && pWriter->writeBytes (_pQuery, _uiQueryLen) < 0) {
        return -14;
    }

    return 0;
}

//==============================================================================
// SearchReplyMsg
//==============================================================================

BaseSearchReplyMsg::BaseSearchReplyMsg (Type type)
    : DisServiceCtrlMsg (type)
{
    _pszQueryId = NULL;
    _pszQuerier = NULL;
    _pszQueryType = NULL;
    _pszMatchingNode = NULL;
}

BaseSearchReplyMsg::BaseSearchReplyMsg (Type type, const char *pszSenderNodeId, const char *pszTargetNodeId)
    : DisServiceCtrlMsg (type, pszSenderNodeId, pszTargetNodeId)
{
    _pszQueryId = NULL;
    _pszQuerier = NULL;
    _pszQueryType = NULL;
    _pszMatchingNode = NULL;
}

BaseSearchReplyMsg::~BaseSearchReplyMsg (void)
{
    if (_pszQueryId != NULL) {
        free (_pszQueryId);
        _pszQueryId = NULL;
    }
    if (_pszQuerier != NULL) {
        free (_pszQuerier);
        _pszQuerier = NULL;
    }
    if (_pszQueryType != NULL) {
        free (_pszQueryType);
        _pszQueryType = NULL;
    }
    if (_pszMatchingNode != NULL) {
        free (_pszMatchingNode);
        _pszMatchingNode = NULL;
    }
}

const char * BaseSearchReplyMsg::getQueryId (void) const
{
    return _pszQueryId;
}

const char * BaseSearchReplyMsg::getQuerier (void) const
{
    return _pszQuerier;
}

const char * BaseSearchReplyMsg::getQueryType (void) const
{
    return _pszQueryType;
}

const char * BaseSearchReplyMsg::getMatchingNode (void) const
{
    return _pszMatchingNode;
}

int BaseSearchReplyMsg::setQueryId (const char *pszQueryId)
{
    if (pszQueryId == NULL) {
        return -1;
    }
    _pszQueryId = strDup (pszQueryId);
    return 0;
}

int BaseSearchReplyMsg::setQuerier (const char *pszQuerier)
{
    if (pszQuerier == NULL) {
        return -1;
    }
    _pszQuerier = strDup (pszQuerier);
    return 0;
}

int BaseSearchReplyMsg::setQueryType (const char *pszQueryType)
{
    if (pszQueryType == NULL) {
        return -1;
    }
    _pszQueryType = strDup (pszQueryType);
    return 0;
}

int BaseSearchReplyMsg::setMatchingNode (const char *pszMatchingNode)
{
    if (pszMatchingNode == NULL) {
        return -1;
    }
    _pszMatchingNode = strDup (pszMatchingNode);
    return 0;
}

int BaseSearchReplyMsg::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -2;
    }

    uint16 ui16;
    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQueryId = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQueryId == NULL) {
                return -3;
            }
            if (pReader->readBytes (_pszQueryId, ui16) < 0) {
                return -4;
            }
            _pszQueryId[ui16] = '\0';
        }
    }
    else {
        return -5;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQuerier = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQuerier == NULL) {
                return -6;
            }
            if (pReader->readBytes (_pszQuerier, ui16) < 0) {
                return -7;
            }
            _pszQuerier[ui16] = '\0';
        }
    }
    else {
        return -8;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszQueryType = (char *) calloc (ui16+1, sizeof (char));
            if (_pszQueryType == NULL) {
                return -9;
            }
            if (pReader->readBytes (_pszQueryType, ui16)) {
                return -10;
            }
            _pszQueryType[ui16] = '\0';
        }
    }
    else {
        return -11;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            _pszMatchingNode = (char *) calloc (ui16+1, sizeof (char));
            if (_pszMatchingNode == NULL) {
                return -9;
            }
            if (pReader->readBytes (_pszMatchingNode, ui16)) {
                return -10;
            }
            _pszMatchingNode[ui16] = '\0';
        }
    }
    else {
        return -13;
    }

    return 0;
}

int BaseSearchReplyMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }

    if (DisServiceCtrlMsg::write (pWriter, ui32MaxSize) != 0) {
        return -2;
    }

    uint16 ui16 = _pszQueryId == NULL ? 0 : strlen (_pszQueryId);
    if (pWriter->write16 (&ui16) < 0) {
        return -3;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQueryId, ui16) < 0) {
        return -4;
    }

    ui16 = _pszQuerier == NULL ? 0 : strlen (_pszQuerier);
    if (pWriter->write16 (&ui16) < 0) {
        return -5;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQuerier, ui16) < 0) {
        return -6;
    }

    ui16 = _pszQueryType == NULL ? 0 : strlen (_pszQueryType);
    if (pWriter->write16 (&ui16) < 0) {
        return -7;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszQueryType, ui16) < 0) {
        return -8;
    }

    ui16 = _pszMatchingNode == NULL ? 0 : strlen (_pszMatchingNode);
    if (pWriter->write16 (&ui16) < 0) {
        return -7;
    }
    if (ui16 > 0 && pWriter->writeBytes (_pszMatchingNode, ui16) < 0) {
        return -8;
    }

    return 0;
}

//-----------------------------------------------------------------------------

SearchReplyMsg::SearchReplyMsg (void)
    : BaseSearchReplyMsg (DSMT_SearchMsgReply),
      _ppszMatchingIds (NULL)
{
}

SearchReplyMsg::SearchReplyMsg (const char *pszSenderNodeId, const char *pszTargetNodeId)
    : BaseSearchReplyMsg (DSMT_SearchMsgReply, pszSenderNodeId, pszTargetNodeId),
      _ppszMatchingIds (NULL)
{
}

SearchReplyMsg::~SearchReplyMsg (void)
{
    if (_ppszMatchingIds != NULL) {
        for (unsigned int i = 0; _ppszMatchingIds[i] != NULL; i++) {
            char *psztmp = _ppszMatchingIds[i];
            free (psztmp);
            _ppszMatchingIds[i] = NULL;
        }
        free (_ppszMatchingIds);
        _ppszMatchingIds = NULL;
    }
}

const char ** SearchReplyMsg::getMatchingMsgIds (void) const
{
    return (const char **) _ppszMatchingIds;
}

int SearchReplyMsg::setMatchingMsgIds (const char **ppszMatchingIds)
{
    if (ppszMatchingIds == NULL) {
        return -1;
    }

    unsigned int i = 0;
    for (; ppszMatchingIds[i] != NULL; i++);

    _ppszMatchingIds = static_cast<char **>(calloc (i + 1, sizeof (char*)));
    for (i = 0; ppszMatchingIds[i] != NULL; i++) {
        _ppszMatchingIds[i] = strDup (ppszMatchingIds[i]);
    }
    _ppszMatchingIds[i] = NULL;

    return 0;
}

int SearchReplyMsg::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    if (BaseSearchReplyMsg::read (pReader, ui32MaxSize) != 0) {
        return -2;
    }

    unsigned int uiCount = 0;
    pReader->read16 (&uiCount);
    if (uiCount == 0) {
        _ppszMatchingIds = NULL;
    }
    else {
        _ppszMatchingIds = (char **) calloc (uiCount + 1, sizeof (char*));
        if (_ppszMatchingIds == NULL) {
            return -12;
        }
        for (unsigned int i = 0; i < uiCount; i++) {
            uint16 ui16 = 0;
            pReader->read16 (&ui16);
            if (ui16 > 0) {
                _ppszMatchingIds[i] = static_cast<char *>(calloc (ui16 + 1, sizeof (char)));
                if (_ppszMatchingIds[i] != NULL) {
                    pReader->readBytes (_ppszMatchingIds[i], ui16);
                    _ppszMatchingIds[i][ui16] = '\0';
                }
            }
        }
        _ppszMatchingIds[uiCount] = NULL;
    }

    return 0;
}

int SearchReplyMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }

    if (BaseSearchReplyMsg::write (pWriter, ui32MaxSize) != 0) {
        return -2;
    }

    if (_ppszMatchingIds != NULL) {
        unsigned int uiCount = 0;
        for (; _ppszMatchingIds[uiCount] != NULL; uiCount++);
        pWriter->write16 (&uiCount);

        for (unsigned int i = 0; i < uiCount; i++) {
            uint16 ui16 = strlen (_ppszMatchingIds[i]);
            pWriter->write16 (&ui16);
            if (ui16 > 0) {
                pWriter->writeBytes (_ppszMatchingIds[i], ui16);
            }
        }
    }
    else {
        uint16 ui16 = 0;
        pWriter->write16 (&ui16);
    }

    return 0;
}

//-----------------------------------------------------------------------------

VolatileSearchReplyMsg::VolatileSearchReplyMsg (void)
    : BaseSearchReplyMsg (DSMT_VolatileSearchMsgReply),
      _ui16ReplyLen (0), _pReply (NULL)
{
}

VolatileSearchReplyMsg::VolatileSearchReplyMsg (const char *pszSenderNodeId, const char *pszTargetNodeId)
    : BaseSearchReplyMsg (DSMT_VolatileSearchMsgReply, pszSenderNodeId, pszTargetNodeId),
      _ui16ReplyLen (0), _pReply (NULL)
{
}

VolatileSearchReplyMsg::~VolatileSearchReplyMsg (void)
{
    if (_pReply != NULL) {
        free (_pReply);
        _pReply = NULL;
    }
    _ui16ReplyLen = 0;
}

const void * VolatileSearchReplyMsg::getReply (uint16 &ui16ReplyLen) const
{
    ui16ReplyLen = _ui16ReplyLen;
    return _pReply;
}

int VolatileSearchReplyMsg::setReply (const void *pReply, uint16 ui16ReplyLen)
{
    if (pReply == NULL || ui16ReplyLen == 0) {
        return -1;
    }
    _pReply = malloc (ui16ReplyLen);
    if (_pReply == NULL) {
        return -2;
    }
    _ui16ReplyLen = ui16ReplyLen;
    memcpy (_pReply, pReply, ui16ReplyLen);
    return 0;
}

int VolatileSearchReplyMsg::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    if (BaseSearchReplyMsg::read (pReader, ui32MaxSize) != 0) {
        return -2;
    }

    if (pReader->read16 (&_ui16ReplyLen) < 0) {
        return -3;
    }
    if (_ui16ReplyLen == 0) {
        return 0;
    }
    _pReply = malloc (_ui16ReplyLen);
    if (_pReply == NULL) {
        return -2;
    }
    if ((_ui16ReplyLen > 0) && (pReader->readBytes (_pReply, _ui16ReplyLen) < 0)) {
        return -4;
    }

    return 0;
}

int VolatileSearchReplyMsg::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }

    if (BaseSearchReplyMsg::write (pWriter, ui32MaxSize) != 0) {
        return -2;
    }

    if (_ui16ReplyLen <= 0 || pWriter->write16 (&_ui16ReplyLen) < 0) {
        return -3;
    }
    if (pWriter->writeBytes (_pReply, _ui16ReplyLen) < 0) {
        return -4;
    }

    return 0;
}


//==============================================================================
// DisServiceImprovedSubscriptionStateMsg
//==============================================================================

DisServiceImprovedSubscriptionStateMsg::DisServiceImprovedSubscriptionStateMsg (void)
    : DisServiceCtrlMsg (DSMT_ImprovedSubStateMessage)
{
}

DisServiceImprovedSubscriptionStateMsg::DisServiceImprovedSubscriptionStateMsg (const char *pszSenderNodeId,
                                                                                StringHashtable<SubscriptionList> *pSubscriptionsTable,
                                                                                StringHashtable<uint32> *pNodesTable)
    : DisServiceCtrlMsg (DSMT_ImprovedSubStateMessage, pszSenderNodeId)
{
    _pSubscriptionsTable = pSubscriptionsTable;
    _pNodesTable = pNodesTable;
}

DisServiceImprovedSubscriptionStateMsg::~DisServiceImprovedSubscriptionStateMsg (void)
{
}

int DisServiceImprovedSubscriptionStateMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != DSMT_ImprovedSubStateMessage) {
        return -2;
    }
    // _pSubscriptionsTable: nodeId->subscriptionList; subscriptionList: groupName->subscription
    // _pNodesTable: nodeId->sequenceId
    _pSubscriptionsTable = new StringHashtable<SubscriptionList>();
    _pNodesTable = new StringHashtable<uint32>();
    uint8 ui8Nodes;
    pReader->read8 (&ui8Nodes);
    for (int i = 0; i < ui8Nodes; i++) {
        uint8 ui8NodeIdLength;
        pReader->read8 (&ui8NodeIdLength);
        char *pszNodeId = new char [ui8NodeIdLength+1];
        pReader->readBytes (pszNodeId, ui8NodeIdLength);
        pszNodeId[ui8NodeIdLength] = '\0';
        String nodeId = pszNodeId;
        delete[] pszNodeId;
        pszNodeId = NULL;
        uint8 ui8Subscriptions;
        pReader->read8 (&ui8Subscriptions);
        SubscriptionList *pSubList = new SubscriptionList();
        for (int j = 0; j < ui8Subscriptions; j++) {
            uint8 ui8groupNameLength;
            pReader->read8 (&ui8groupNameLength);
            char *pszGroupName = new char [ui8groupNameLength+1];
            pReader->readBytes (pszGroupName, ui8groupNameLength);
            pszGroupName[ui8groupNameLength] = '\0';
            String groupName = pszGroupName;
            delete[] pszGroupName;
            pszGroupName = NULL;
            uint8 ui8SubType;
            pReader->read8 (&ui8SubType);
            switch (ui8SubType) {
                case Subscription::GROUP_SUBSCRIPTION:
                {
                    GroupSubscription *pGS = new GroupSubscription();
                    pGS->read (pReader, ui32MaxSize);
                    pSubList->addSubscription (groupName, pGS);
                    break;
                }
                case Subscription::GROUP_TAG_SUBSCRIPTION:
                {
                    GroupTagSubscription *pGTS = new GroupTagSubscription();
                    pGTS->read (pReader, ui32MaxSize);
                    pSubList->addSubscription (groupName, pGTS);
                    break;
                }
                case Subscription::GROUP_PREDICATE_SUBSCRIPTION:
                {
                    GroupPredicateSubscription *pGPS = new GroupPredicateSubscription();
                    pGPS->read (pReader, ui32MaxSize);
                    pSubList->addSubscription (groupName, pGPS);
                    break;
                }
            }
        }
        uint32 *pui32SeqId = new uint32();
        pReader->read32 (pui32SeqId);
        _pNodesTable->put (nodeId, pui32SeqId);
        _pSubscriptionsTable->put (nodeId, pSubList);
    }
    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceImprovedSubscriptionStateMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }
    // _pSubscriptionsTable: nodeId->subscriptionList; subscriptionList: groupName->subscription
    // _pNodesTable: nodeId->sequenceId
    BufferWriter nodesBW (ui32MaxSize, ui32MaxSize);
    BufferWriter subscriptionsBW (ui32MaxSize, ui32MaxSize);
    BufferWriter subBW (ui32MaxSize, ui32MaxSize);
    uint8 ui8Nodes = 0;
    for (StringHashtable<SubscriptionList>::Iterator iterator = _pSubscriptionsTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        String nodeId = iterator.getKey();
        uint8 ui8NodeIdLength = nodeId.length();
        uint32 *pui32SeqId = _pNodesTable->get (nodeId);
        uint8 ui8Subscriptions = 0;
        for (StringHashtable<Subscription>::Iterator i = (iterator.getValue())->getIterator(); !i.end(); i.nextElement()) {
            String groupName = i.getKey();
            uint8 ui8GroupNameLength = groupName.length();
            Subscription *pS = i.getValue();
            uint8 ui8SubType = pS->getSubscriptionType();
            switch (pS->getSubscriptionType()) {
                case Subscription::GROUP_SUBSCRIPTION:
                {
                    GroupSubscription *pGS = (GroupSubscription *) pS;
                    pGS->write (&subBW, ui32MaxSize);
                    break;
                }
                case Subscription::GROUP_TAG_SUBSCRIPTION:
                {
                    GroupTagSubscription *pGTS = (GroupTagSubscription *) pS;
                    pGTS->write (&subBW, ui32MaxSize);
                    break;
                }
                case Subscription::GROUP_PREDICATE_SUBSCRIPTION:
                {
                    GroupPredicateSubscription *pGPS = (GroupPredicateSubscription *) pS;
                    pGPS->write (&subBW, ui32MaxSize);
                    break;
                }
            }
            if ((subscriptionsBW.getBufferLength() + sizeof (uint8) + ui8GroupNameLength + sizeof (uint8) + subBW.getBufferLength()) <= ui32MaxSize) {
                subscriptionsBW.write8 (&ui8GroupNameLength);
                subscriptionsBW.writeBytes (groupName, ui8GroupNameLength);
                subscriptionsBW.write8 (&ui8SubType);
                subscriptionsBW.writeBytes (subBW.getBuffer(), subBW.getBufferLength());
                ui8Subscriptions++;
            }
            subBW.reset();
        }
        if ((nodesBW.getBufferLength() + sizeof (uint8) + ui8NodeIdLength + sizeof (uint8) + subscriptionsBW.getBufferLength() + sizeof (uint32)) <= ui32MaxSize) {
            nodesBW.write8 (&ui8NodeIdLength);
            nodesBW.writeBytes (nodeId, ui8NodeIdLength);
            nodesBW.write8 (&ui8Subscriptions);
            nodesBW.writeBytes (subscriptionsBW.getBuffer(), subscriptionsBW.getBufferLength());
            nodesBW.write32 (pui32SeqId);
            ui8Nodes++;
        }
        subscriptionsBW.reset();
    }
    iw.write8 (&ui8Nodes);
    iw.writeBytes (nodesBW.getBuffer(), nodesBW.getBufferLength());
    _ui16Size = iw.getBytesWritten();
    return 0;
}

StringHashtable<SubscriptionList> * DisServiceImprovedSubscriptionStateMsg::getSubscriptionsTable (void)
{
    return _pSubscriptionsTable;
}

StringHashtable<uint32> * DisServiceImprovedSubscriptionStateMsg::getNodesTable (void)
{
    return _pNodesTable;
}

//==============================================================================
// DisServiceProbabilitiesMsg
//==============================================================================

DisServiceProbabilitiesMsg::DisServiceProbabilitiesMsg (void)
    : DisServiceCtrlMsg (DSMT_ProbabilitiesMsg)
{
}

DisServiceProbabilitiesMsg::DisServiceProbabilitiesMsg (const char *pszSenderNodeId,
                                                        StringHashtable<StringFloatHashtable> *pProbabilitiesTable)
    : DisServiceCtrlMsg (DSMT_ProbabilitiesMsg, pszSenderNodeId)
{
    _pProbabilitiesTable = pProbabilitiesTable;
}

DisServiceProbabilitiesMsg::~DisServiceProbabilitiesMsg (void)
{
}

int DisServiceProbabilitiesMsg::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (DisServiceCtrlMsg::read (pReader, ui32MaxSize) != 0) {
        return -1;
    }
    if (_type != DisServiceMsg::DSMT_ProbabilitiesMsg) {
        return -2;
    }
    // _pProbabilitiesTable: nodeId->indirectProbTable; indirectProbTable: gatewayNodeId->probValue
    _pProbabilitiesTable = new StringHashtable<StringFloatHashtable>();
    uint8 ui8Nodes;
    pReader->read8 (&ui8Nodes);
    for (int i = 0; i < ui8Nodes; i++) {
        uint8 ui8NodeIdLength;
        pReader->read8 (&ui8NodeIdLength);
        char *pszNodeId = new char [ui8NodeIdLength+1];
        pReader->readBytes (pszNodeId, ui8NodeIdLength);
        pszNodeId[ui8NodeIdLength] = '\0';
        String nodeId = pszNodeId;
        delete[] pszNodeId;
        pszNodeId = NULL;
        uint8 ui8Gateways;
        pReader->read8 (&ui8Gateways);
        StringFloatHashtable *pIndProb = new StringFloatHashtable();
        for (int j = 0; j < ui8Gateways; j++) {
            uint8 ui8GatewayNodeIdLength;
            pReader->read8 (&ui8GatewayNodeIdLength);
            char *pszGatewayNodeId = new char [ui8GatewayNodeIdLength+1];
            pReader->readBytes (pszGatewayNodeId, ui8GatewayNodeIdLength);
            pszGatewayNodeId[ui8GatewayNodeIdLength] = '\0';
            String gatewayNodeId = pszGatewayNodeId;
            delete[] pszGatewayNodeId;
            pszGatewayNodeId = NULL;
            float fProb;
            pReader->read32 (&fProb);
            float * pProb = &fProb;
            pIndProb->put (gatewayNodeId, pProb);
        }
        _pProbabilitiesTable->put (nodeId, pIndProb);
    }
    _ui16Size = pReader->getTotalBytesRead();
    return 0;
}

int DisServiceProbabilitiesMsg::write (Writer *pWriter, uint32 ui32MaxSize)
{
    InstrumentedWriter iw (pWriter);
    if (DisServiceCtrlMsg::write (&iw, ui32MaxSize) != 0) {
        return -1;
    }
    // _pProbabilitiesTable: nodeId->indirectProbTable; indirectProbTable: gatewayNodeId->probValue
    BufferWriter nodesBW (ui32MaxSize, ui32MaxSize);
    BufferWriter gatewaysBW (ui32MaxSize, ui32MaxSize);
    uint8 ui8Nodes = 0;
    for (StringHashtable<StringFloatHashtable>::Iterator iterator = _pProbabilitiesTable->getAllElements(); !iterator.end(); iterator.nextElement()) {
        String nodeId = iterator.getKey();
        uint8 ui8NodeIdLength = nodeId.length();
        uint8 ui8Gateways = 0;
        for (StringFloatHashtable::Iterator i = (iterator.getValue())->getAllElements(); !i.end(); i.nextElement()) {
            String gatewayNodeId = i.getKey();
            uint8 ui8GatewayNodeIdLength = gatewayNodeId.length();
            float * pProb = i.getValue();
            if ((gatewaysBW.getBufferLength() + sizeof(uint8) + ui8GatewayNodeIdLength + sizeof(uint32)) <= ui32MaxSize) {
                gatewaysBW.write8 (&ui8GatewayNodeIdLength);
                gatewaysBW.writeBytes (gatewayNodeId, ui8GatewayNodeIdLength);
                gatewaysBW.write32 (pProb);
                ui8Gateways++;
            }
        }
        if ((nodesBW.getBufferLength() + sizeof(uint8) + ui8NodeIdLength + sizeof(uint8) + gatewaysBW.getBufferLength()) <= ui32MaxSize) {
            nodesBW.write8 (&ui8NodeIdLength);
            nodesBW.writeBytes (nodeId, ui8NodeIdLength);
            nodesBW.write8 (&ui8Gateways);
            nodesBW.writeBytes (gatewaysBW.getBuffer(), gatewaysBW.getBufferLength());
            ui8Nodes++;
        }
        gatewaysBW.reset();
    }
    iw.write8 (&ui8Nodes);
    iw.writeBytes (nodesBW.getBuffer(), nodesBW.getBufferLength());
    _ui16Size = iw.getBytesWritten();
    return 0;
}

StringHashtable<StringFloatHashtable> * DisServiceProbabilitiesMsg::getProbabilitiesTable (void)
{
    return _pProbabilitiesTable;
}
