/*
 * DisServiceScheduler.cpp
 *
 *This file is part of the IHMC DisService Library/Component
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

#if defined (USE_SCHEDULER)

#include "DisseminationService.h"

#include "DisServiceMsg.h"
#include "DisServiceScheduler.h"
#include "MessageInfo.h"

#include "NLFLib.h"
#include "Mutex.h"
#include "Logger.h"
#include "MessageInfo.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace IHMC_ACI;

const int DisServiceScheduler::_DATA_MESSAGES_TO_SEND = 1;
const uint8 DisServiceScheduler::_DEFAULT_PRIORITY = 5;

DisServiceScheduler::DisServiceScheduler(DisseminationService * pDisService) {
    _pOutgoingDataQueue = new PtrLList<MsgWrap>();
    _pOutgoingCtrlQueue = new PtrLList<MsgWrap>();
    _dataQueueCount = 0;
    _ctrlQueueCount = 0;
    _pDisService = pDisService;
}

DisServiceScheduler::~DisServiceScheduler() {
    delete _pOutgoingDataQueue;
    delete _pOutgoingCtrlQueue;
}


int DisServiceScheduler::enqueueMessage(DisServiceMsg * pMessage, const char * pszPurpose) {
    _m.lock();
    if(pMessage == NULL) {
        if(pLogger) pLogger->logMsg("DisServiceScheduler::enqueueMessage", Logger::L_SevereError,
                "The given message is a NULL pointer\n");
        _m.unlock();
        return -1;
    }
    MsgWrap * pMsgWrapper;
    uint8 ui8Priority;
    if(pMessage->getType() == DisServiceMsg::DSMT_Data) {
        DisServiceDataMsg *pDataMsg = (DisServiceDataMsg *) pMessage;
        pMsgWrapper = new MsgWrap(pMessage,
                                         (pDataMsg->getMessageHeader()->isChunk() ? 1 : ((MessageInfo*)pDataMsg->getMessageHeader())->getPriority()),
                                         pszPurpose,
                                         getTimeInMilliseconds());
        _dataQueueCount++;
    }
    else {
        pMsgWrapper = new MsgWrap(pMessage, _DEFAULT_PRIORITY, pszPurpose, getTimeInMilliseconds());
        _ctrlQueueCount++;
    }
    _pOutgoingDataQueue->insert(pMsgWrapper);
    _m.unlock();
    return 0;
}

int DisServiceScheduler::sendMessages(void) {
    _m.lock();
    if(_ctrlQueueCount > 0) {
        for(; _ctrlQueueCount > 0; _ctrlQueueCount--) {
            MsgWrap * pMsgWrapper = _pOutgoingCtrlQueue->getFirst();
            if (pMsgWrapper != NULL) {
                if(pMsgWrapper->getMessage()->getType() == DisServiceMsg::DSMT_CtrlToCtrlMessage) {
                    int retVal = _pDisService->reliableUnicastDisServiceControllerMsg((ControllerToControllerMsg *) pMsgWrapper->getMessage());
                    if(retVal != 0) {
                        checkAndLogMsg ("DisServiceScheduler::sendMessages", Logger::L_SevereError,
                                        "Call to reliableUnicastDisServiceControllerMsg failed\n");
                        _m.unlock();
                        return -1;
                    }
                }
                else {
                    int retVal = _pDisService->broadcastDisServiceMsg((DisServiceCtrlMsg *) pMsgWrapper->getMessage(), pMsgWrapper->getPurpose());
                    if(retVal != 0) {
                        checkAndLogMsg ("DisServiceScheduler::sendMessages", Logger::L_SevereError,
                                        "Call to reliableUnicastDisServiceControllerMsg failed\n");
                        _m.unlock();
                        return -1;
                    }
                }
                _pOutgoingCtrlQueue->remove(pMsgWrapper);
            }
        }
    }
    if(_dataQueueCount > 0) {
        for(int i = 0; (i < _DATA_MESSAGES_TO_SEND) && (_dataQueueCount); i++, _dataQueueCount--) {
            MsgWrap * pMsgWrapper = _pOutgoingDataQueue->getFirst();
            if (pMsgWrapper != NULL) {
                int retVal = _pDisService->fragmentAndBroadcastDisServiceDataMsg((DisServiceDataMsg*)pMsgWrapper->getMessage(), pMsgWrapper->getPurpose());
                if(retVal != 0) {
                    checkAndLogMsg ("DisServiceScheduler::sendMessages", Logger::L_SevereError,
                                    "Call to broadcastDisServiceMsg failed\n");
                    _m.unlock();
                    return -1;
                }
                _pOutgoingDataQueue->remove(pMsgWrapper);
            }
        }
    }
    _m.unlock();
    return 0;
}

#endif
