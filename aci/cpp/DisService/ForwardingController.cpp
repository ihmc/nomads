/*
 * ForwardingController.cpp
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

#include "ForwardingController.h"

#include "DisseminationService.h"
#include "PeerState.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const bool ForwardingController::REQUIRE_ACKNOWLEDGMENT = false;

ForwardingController::ForwardingController (Type type, DisseminationService *pDisService)
    : MessagingService (pDisService)
{
    _type = type;
    _pDisService = pDisService;
    _pDCCtlr = new DataCacheService (_pDisService);
    _pConfigManager = _pDisService->_pCfgMgr;
}

ForwardingController::~ForwardingController()
{
}

int ForwardingController::setDefaultForwardingcontroller (ForwardingController *pDefFwdCtrl)
{
    if (pDefFwdCtrl == NULL) {
        return -1;
    }
    _pDefFwdCtrl = pDefFwdCtrl;
    return 0;
}

//------------------------------------------------------------------
// ForwardingController -> DisService
//------------------------------------------------------------------

int ForwardingController::handleMessageWithDefaults (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                                     const char *pszIncomingInterface)
{
    _pDefFwdCtrl->newIncomingMessage (pMsgMetaData, ui16MsgMetaDataLen, pDisServiceMsg,
                                      ui32SourceIPAddress, pszIncomingInterface);
    return 0;
}

PeerState * ForwardingController::lockAndGetWorldState()
{
    PeerState *pPS = _pDisService->getPeerState();
    if (pPS != NULL) {
        pPS->lock();
    }
    return pPS;
}

void ForwardingController::releaseWorldState (PeerState *pPeerState)
{
    pPeerState->unlock();
}


