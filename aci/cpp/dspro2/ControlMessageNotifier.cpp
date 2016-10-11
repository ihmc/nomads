/*
 * ControlMessageNotifier.cpp
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

#include "ControlMessageNotifier.h"

#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_ACI
{
    ControlMessageNotifier *pCtrlMsgNotifier;
}

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//==============================================================================
// ControlMessageNotifier
//==============================================================================

ControlMessageNotifier::ControlMessageNotifier (NOMADSUtil::LoggingMutex *pmCallback)
    : _ctrlMsgClients(1)
{
    _uiNListeners = 0;
    _pmCallback = pmCallback;
}

ControlMessageNotifier::~ControlMessageNotifier (void)
{
}

int ControlMessageNotifier::registerAndEnableControllerMessageListener (uint16 ui16ClientId, ControlMessageListener *pControllerMessageListener, uint16 &ui16AssignedClientId)
{
    const char *pszMethodName = "ControlMessageNotifier::registerAndEnableMatchmakingLogListener";
    if (_ctrlMsgClients.used (ui16ClientId) && _ctrlMsgClients[ui16ClientId].pControllerMessageListener != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Client ID %d in use.  Register client using a "
                        "different ui16ClientId.\n",  ui16ClientId);
        ui16ClientId = _ctrlMsgClients.firstFree();
    }
    _ctrlMsgClients[ui16ClientId].pControllerMessageListener = pControllerMessageListener;
    _uiNListeners++;
    ui16AssignedClientId = ui16ClientId;
    return 0;
}

int ControlMessageNotifier::deregisterAndDisableControllerMessageListener (uint16 ui16ClientId)
{
    if (_uiNListeners > 0) {
        _uiNListeners--;
    }
    return _ctrlMsgClients.clear (ui16ClientId);
}

bool ControlMessageNotifier::contextUpdateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->contextUpdateMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::contextVersionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->contextVersionMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::messageRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->messageRequestMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::chunkRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->chunkRequestMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::positionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->positionMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::searchMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->searchMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::topologyReplyMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->topologyReplyMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::topologyRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->topologyRequestMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::updateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->updateMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::versionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->versionMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::waypointMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->waypointMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

bool ControlMessageNotifier::wholeMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    for (unsigned int i = 0; i < _ctrlMsgClients.size(); i++) {
        if (_ctrlMsgClients.used (i) && _ctrlMsgClients[i].pControllerMessageListener != NULL) {
            _ctrlMsgClients[i].pControllerMessageListener->wholeMessageArrived (pszSenderNodeId, pszPublisherNodeId);
        }
    }
    return true;
}

//==============================================================================
// ControlMessageNotifier::MatchmakerClientInfoPro
//==============================================================================
ControlMessageNotifier::ControlMessageClientInfoPro::ControlMessageClientInfoPro()
{
    pControllerMessageListener = NULL;
}

ControlMessageNotifier::ControlMessageClientInfoPro::~ControlMessageClientInfoPro()
{
    pControllerMessageListener = NULL;
}

