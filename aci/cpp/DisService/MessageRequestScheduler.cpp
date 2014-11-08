/*
 * MessageRequestScheduler.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 20, 2010, 6:11 PM
 */

#include "MessageRequestScheduler.h"

#include "DisServiceDefs.h"
#include "MessageInfo.h"

#include "Logger.h"
#include "NLFLib.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MessageRequestScheduler::MessageRequestScheduler (float fDefaultReqProb, bool bDeleteDuplicateRequest)
    : _requests (true) // bDescendingOrder
{
    _fDefaultReqProb = fDefaultReqProb;
    _priorityToProb.setInitialValue (_fDefaultReqProb);
    _pCurrReq = NULL;
    _bDeleteDuplicateRequest = bDeleteDuplicateRequest;
    _ui8MaxNAttempts = 2;
    _ui8NAttempts = 0;
    _bAtLeastOnePick = false;

    // initialize random seed:
    srand ((unsigned int)getTimeInMilliseconds());
}

MessageRequestScheduler::~MessageRequestScheduler()
{
    reset();
}

void MessageRequestScheduler::addRequest (DisServiceDataReqMsg::FragmentRequest *pFragReq,
                                          bool bSeq, bool bRel)
{
    if (pFragReq == NULL) {
        return;
    }
    MessageHeader *pMH = pFragReq->pMsgHeader;
    if (pMH != NULL) {
        // Create the proper FragReqWrapper
        FragReqWrapper *pReq = NULL;
        if (pMH->isChunk()) {
            pReq = new ChunkFragReqWrapper (pFragReq, bSeq, bRel, _bDeleteDuplicateRequest);
        }
        else {
            pReq = new MessageFragReqWrapper (pFragReq, bSeq, bRel, _bDeleteDuplicateRequest);
        }

        if (pReq == NULL) {
            checkAndLogMsg ("MessageRequestScheduler::addRequest", memoryExhausted);
            delete pFragReq;
        }
        else {
            checkAndLogMsg ("MessageRequestScheduler::addRequest", Logger::L_HighDetailDebug,
                            "added request for %d fragments\n",
                            pFragReq->pRequestedRanges->getCount());
            // Insert the FragReqWrapper
            _requests.insert (pReq);
        }
    }
}

DisServiceDataReqMsg::FragmentRequest * MessageRequestScheduler::getRequest()
{
    if (_requests.isEmpty()) {
        return NULL;
    }

    FragReqWrapper *pReq = _pCurrReq;
    FragReqWrapper *pRetWr;
    FragmentRequest *pRet;

    while (!_requests.isEmpty()) {
        if (pReq != NULL) {
            if (((rand() % 100 + 1) <= getRequestProbability (pReq->ui8Priority)) ||
                (_ui8NAttempts == _ui8MaxNAttempts)) {
                // Return the element with a certain probability or return it
                // after _ui8MaxNAttempts complete iterations throughout the
                // whole list without returning any element.
                _pCurrReq = _requests.getNext();
                pRetWr = _requests.remove (pReq);
                if (pRetWr != NULL) {
                    if (!_bAtLeastOnePick) {
                        _bAtLeastOnePick = true;
                        _ui8NAttempts = 0;
                    }
                    pRet = pRetWr->reliquishFragmentRequest();
                    delete pRetWr;
                    pRetWr = NULL;
                    return pRet;
                }
                else {
                    // I think this case should never happen...
                    checkAndLogMsg ("MessageRequestScheduler::getRequest",
                                    Logger::L_SevereError, "Calling remove() on "
                                    "_requests returned NULL, when trying to remove"
                                    "the element returned by getNext()\n");
                    pRetWr = _pCurrReq;
                }
            }
            else {
                // try to get next
                pReq = _requests.getNext();
            }
        }
        else {
            // I iterated throughout the whole list. Start again!
            resetGetRequest();
            pReq = _requests.getFirst();
            if (_bAtLeastOnePick) {
                _bAtLeastOnePick = false;
            }
            else {
                _ui8NAttempts++;
            }
        }
    }

    // No more elements
    return NULL;
}

bool MessageRequestScheduler::isEmpty (void)
{
    return _requests.isEmpty();
}

void MessageRequestScheduler::removeRequest (FragReqWrapper *pReq)
{
    if (pReq == NULL) {
        return;
    }
    delete _requests.remove (pReq);
}

void MessageRequestScheduler::reset()
{
    _pCurrReq = NULL;

    // Remove and delete all the elements
    FragReqWrapper *pFragWrap;
    FragReqWrapper *pFragWrapTmp = _requests.getFirst();
    while ((pFragWrap = pFragWrapTmp) != NULL) {
        pFragWrapTmp = _requests.getNext();
        _requests.remove (pFragWrap);
        delete pFragWrap;
    }
}

void MessageRequestScheduler::resetGetRequest()
{
    _pCurrReq = NULL;
    _requests.resetGet();
}

float MessageRequestScheduler::getRequestProbability (uint8 ui8Priority) const
{
    if (ui8Priority < _priorityToProb.size()) {
        return _priorityToProb.get (ui8Priority);
    }
    return _fDefaultReqProb;
}

void MessageRequestScheduler::setRequestProbability (uint8 ui8Priority, float lReqProb)
{
    if (lReqProb < 0) {
        checkAndLogMsg ("MessageRequestScheduler::setRequestProbability", Logger::L_Warning,
                        "Invalid value of request probability: %f. "
                        "Setting it to 0", lReqProb);
        lReqProb = 0;
    }
    else if (lReqProb > 100) {
        checkAndLogMsg ("MessageRequestScheduler::setRequestProbability", Logger::L_Warning,
                        "Invalid value of request probability: %f. "
                        "Setting it to 100", lReqProb);
        lReqProb = 100;
    }
    _priorityToProb[ui8Priority] = lReqProb;
}

////////////////////////////// Request Wrapper /////////////////////////////////

MessageRequestScheduler::FragReqWrapper::FragReqWrapper (DisServiceDataReqMsg::FragmentRequest *pFragReq,
                                                         bool bSeq, bool bRel, bool bDeleteFragmentRequest,
                                                         uint8 ui8Prority, const FRAG_WRAPPER_TYPE fragType)
    : bSequenced (bSeq),
      bReliable (bRel),
      bDelFragReq (bDeleteFragmentRequest),
      ui8Priority (ui8Prority),
      type (fragType),
      pFRequests (pFragReq)
{
}

MessageRequestScheduler::FragReqWrapper::~FragReqWrapper()
{
    delete pFRequests;
    pFRequests = NULL;
}

const char * MessageRequestScheduler::FragReqWrapper::getGroupName() const
{
    if (pFRequests != NULL) {
        MessageHeader *pMH = pFRequests->pMsgHeader;
        if (pMH != NULL) {
            return pMH->getGroupName();
        }
    }
    return NULL;
}

const char * MessageRequestScheduler::FragReqWrapper::getSenderNodeId() const
{
    if (pFRequests != NULL) {
        MessageHeader *pMH = pFRequests->pMsgHeader;
        if (pMH != NULL) {
            return pMH->getPublisherNodeId();
        }
    }
    return NULL;
}

uint32 MessageRequestScheduler::FragReqWrapper::getMsgSeqId() const
{
    return pFRequests->pMsgHeader->getMsgSeqId();
}

DisServiceDataReqMsg::FragmentRequest * MessageRequestScheduler::FragReqWrapper::reliquishFragmentRequest()
{
    FragmentRequest *pTMPFRequests = pFRequests;
    pFRequests = NULL;
    return pTMPFRequests;
}

bool MessageRequestScheduler::FragReqWrapper::operator > (const FragReqWrapper& rhsReq) const
{
    if (ui8Priority > rhsReq.ui8Priority) {
        return true;
    }
    if (ui8Priority == rhsReq.ui8Priority) {
        // They have the same value of priority; other characteristics - such as
        // reliability and sequentiality of the transmission and the seqId of the
        // message - have to be considered in order to prioritize the messages.
        if (bReliable) {
            if (bSequenced) {
                // This message belongs to a sequenced and reliable subscription
                if ((!rhsReq.bSequenced) || (!rhsReq.bReliable)) {
                    // if rhsReq does not belong to a sequenced and reliable
                    // transmission too, this message has to be prioritized more
                    return true;
                }
                else {
                    // if rhsReq belong to a sequenced and reliable subscription
                    if ((strcmp (getGroupName(), rhsReq.getGroupName()) == 0) &&
                            (strcmp (getSenderNodeId(), rhsReq.getSenderNodeId()) == 0)) {
                        // and rhsReq belong to the same <groupName, senderNodeId>
                        if (getMsgSeqId() < rhsReq.getMsgSeqId()) {
                            // prioritize the message with smaller seq id
                            return true;
                        }
                    }
                }
            }
        } else if (bSequenced) {
            if ((strcmp (getGroupName(), rhsReq.getGroupName()) == 0) &&
                (strcmp (getSenderNodeId(), rhsReq.getSenderNodeId()) == 0)) {
                // and rhsReq belong to the same <groupName, senderNodeId>
                // than this message
                if (getMsgSeqId() > rhsReq.getMsgSeqId()) {
                    // prioritize the message with greater seq id
                    return true;
                }
            }
        }
    }
    return false;
}

bool MessageRequestScheduler::FragReqWrapper::operator < (const FragReqWrapper &rhsReq) const
{
    return (rhsReq > (*this));
}

bool MessageRequestScheduler::FragReqWrapper::operator == (const FragReqWrapper& rhsReq) const
{
    if (strcmp (getGroupName(), rhsReq.getGroupName()) != 0) {
        return false;
    }
    if (strcmp (getSenderNodeId(), rhsReq.getSenderNodeId()) != 0) {
        return false;
    }
    if (getMsgSeqId() != rhsReq.getMsgSeqId()) {
        return false;
    }
    return true;
}

///////////////////////////// MessageFragReqWrapper ////////////////////////////

MessageRequestScheduler::MessageFragReqWrapper::MessageFragReqWrapper (DisServiceDataReqMsg::FragmentRequest *pFragReq,
                                                                       bool bSeq, bool bRel, bool bDeleteFragmentRequest)
    : FragReqWrapper (pFragReq, bSeq, bRel, bDeleteFragmentRequest,
                      ((MessageInfo*)pFragReq->pMsgHeader)->getPriority(), MSG_FW)
{
}

MessageRequestScheduler::MessageFragReqWrapper::~MessageFragReqWrapper()
{
}

uint8 MessageRequestScheduler::MessageFragReqWrapper::getPriority (void) const
{
    return ui8Priority;
}

///////////////////////////// ChunkFragReqWrapper //////////////////////////////

MessageRequestScheduler::ChunkFragReqWrapper::ChunkFragReqWrapper (DisServiceDataReqMsg::FragmentRequest *pFragReq,
                                                                   bool bSeq, bool bRel, bool bDeleteFragmentRequest)
    : FragReqWrapper (pFragReq, bSeq, bRel,
                      bDeleteFragmentRequest, 0, // TODO: for now, chunks are assigned the lowest priority.
                      CHK_FW)                    // Consider if it should be changed.                      
{
}

MessageRequestScheduler::ChunkFragReqWrapper::~ChunkFragReqWrapper()
{
}

uint8 MessageRequestScheduler::ChunkFragReqWrapper::getPriority (void) const
{
    return ui8Priority;
}
