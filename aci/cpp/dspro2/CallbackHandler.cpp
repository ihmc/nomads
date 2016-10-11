/*
 * DSProImpl.cpp
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "CallbackHandler.h"

#include "ControlMessageNotifier.h"
#include "Defs.h"
#include "DSProListener.h"
#include "Instrumentator.h"
#include "Listener.h"
#include "NodePath.h"
#include "SearchProperties.h"

#include "ConfigManager.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * CallbackHandler::LOOPBACK_NOTIFICATION = "loopback";

CallbackHandler::CallbackHandler (void)
    : _mCallback (MutexId::DSProCback_m, LOG_MUTEX)
{    
}

CallbackHandler::~CallbackHandler (void)
{
}

int CallbackHandler::init (NOMADSUtil::ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    if (pCfgMgr->getValueAsBool ("aci.dspro.instrument", true)) {
        pInstrumentator = new Instrumentator (&_mCallback);
    }
    if (pCfgMgr->getValueAsBool ("aci.dspro.ctrlMsgs.notify", true)) {
        pCtrlMsgNotifier = new ControlMessageNotifier (&_mCallback);
    }
    return 0;
}

int CallbackHandler::dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                  const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                  const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                                  const char *pszQueryId)
{
    const char *pszMethodName = "CallbackHandler::dataArrived";
    // pszGroupName and pszMimeType can be null
    if (pszId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }

    _mCallback.lock (2025);
    for (unsigned int i = 0; i < _clientsPro.size(); i++) {
        if (_clientsPro.used (i)) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "notifying application %u "
                            "with message %s (%d/%d)(%s) of length %u\n", i, pszId, ui8NChunks, ui8TotNChunks,
                            (pszQueryId == NULL ? "" : pszQueryId), ui32Len);
            _clientsPro[i].pListener->dataArrived (pszId, pszGroupName, pszObjectId, pszInstanceId,
                                                   pszAnnotatedObjMsgId, pszMimeType, pBuf, ui32Len,
                                                   ui8NChunks, ui8TotNChunks, pszQueryId);
        }
    }
    _mCallback.unlock (2025);

    return 0;
}

int CallbackHandler::metadataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                      const char *pszInstanceId, const char *pszXMLMetadata, const char *pszReferredDataId,
                                      const char *pszQueryId, bool bIsTarget)
{
    const char *pszMethodName = "CallbackHandler::metadataArrived";
    if (pszId == NULL || pszXMLMetadata == NULL || pszReferredDataId == NULL) {
        return -1;
    }

    _mCallback.lock (2026);
    for (unsigned int i = 0; i < _clientsPro.size (); i++) {
        if (_clientsPro.used (i)) {
            if (bIsTarget || (pszQueryId != NULL)) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "notifying application %u with message %s (%s)\n",
                                i, pszId, (pszQueryId == NULL ? "" : pszQueryId));
                _clientsPro[i].pListener->metadataArrived (pszId, pszGroupName, pszObjectId,
                                                           pszInstanceId, pszXMLMetadata,
                                                           pszReferredDataId, pszQueryId);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "not notifying application %u with message %s (%s) because "
                                "the node or user is not the target of the message\n", i, pszId, (pszQueryId == NULL ? "" : pszQueryId));
            }
        }
    }
    _mCallback.unlock (2026);
    return 0;
}


int CallbackHandler::newPeer (const char *pszNewPeerId)
{
    _mCallback.lock (2027);
    for (unsigned int i = 0; i < _clientsPro.size(); i++) {
        if (_clientsPro.used (i)) {
            _clientsPro[i].pListener->newPeer (pszNewPeerId);
        }
    }
    _mCallback.unlock (2027);

    return 0;
}

int CallbackHandler::deadPeer (const char *pszDeadPeerId)
{
    _mCallback.lock (2028);
    for (unsigned int i = 0; i < _clientsPro.size(); i++) {
        if (_clientsPro.used (i)) {
            _clientsPro[i].pListener->deadPeer (pszDeadPeerId);
        }
    }
    _mCallback.unlock (2028);

    return 0;
}

void CallbackHandler::pathRegistered (NodePath *pPath, const char *pszNodeId,
                                      const char *pszTeam, const char *pszMission)
{
    const char *pszMethodName = "CallbackHandler::pathRegistered";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "calling path registered for "
                    "node %s, with path of length %d\n", pszNodeId, pPath->getPathLength());
    _mCallback.lock (2023);
    for (unsigned int i = 0; i < _clientsPro.size(); i++) {
        if (_clientsPro.used (i)) {
            if (_clientsPro[i].pListener != NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "calling path registered for node %s, "
                                "with path of length %d to client %d\n", pszNodeId, pPath->getPathLength(), i);
                _clientsPro[i].pListener->pathRegistered (pPath, pszNodeId, pszTeam, pszMission);
            }
        }
    }
    _mCallback.unlock (2023);
}

void CallbackHandler::positionUpdated (float latitude, float longitude, float altitude, const char *pszNodeId)
{
    _mCallback.lock (2024);
    for (unsigned int i = 0; i < _clientsPro.size (); i++) {
        if (_clientsPro.used (i)) {
            if (_clientsPro[i].pListener != NULL) {
                _clientsPro[i].pListener->positionUpdated (latitude, longitude, altitude, pszNodeId);
            }
        }
    }
    _mCallback.unlock (2024);
}

void CallbackHandler::searchArrived (SearchProperties *pSearchProperties)
{
    _mCallback.lock (2032);
    for (unsigned int i = 0; i < _searchListners.size (); i++) {
        if (_searchListners.used (i) && _searchListners[i].pListener != NULL) {
            _searchListners[i].pListener->searchArrived (pSearchProperties->pszQueryId, pSearchProperties->pszGroupName,
                pSearchProperties->pszQuerier, pSearchProperties->pszQueryType,
                pSearchProperties->pszQueryQualifiers, pSearchProperties->pQuery,
                pSearchProperties->uiQueryLen);
        }
    }
    _mCallback.unlock (2032);
}

void CallbackHandler::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    _mCallback.lock (2032);
    for (unsigned int i = 0; i < _searchListners.size (); i++) {
        if (_searchListners.used (i) && _searchListners[i].pListener != NULL) {
            _searchListners[i].pListener->searchReplyArrived (pszQueryId, ppszMatchingMessageIds, pszMatchingNodeId);
        }
    }
    _mCallback.unlock (2032);
}

void CallbackHandler::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen, const char *pszMatchingNodeId)
{
    _mCallback.lock (2032);
    for (unsigned int i = 0; i < _searchListners.size (); i++) {
        if (_searchListners.used (i) && _searchListners[i].pListener != NULL) {
            _searchListners[i].pListener->volatileSearchReplyArrived (pszQueryId, pReply, ui16ReplyLen, pszMatchingNodeId);
        }
    }
    _mCallback.unlock (2032);
}

int CallbackHandler::registerDSProListener (uint16 ui16ClientId, DSProListener *pListener, uint16 &ui16AssignedClientId)
{
    _mCallback.lock (2019);
    if (_clientsPro.used (ui16ClientId)) {
        ui16AssignedClientId = _clientsPro.firstFree();
    }
    else {
        ui16AssignedClientId = ui16ClientId;
    }
    _clientsPro[ui16AssignedClientId].pListener = pListener;
    _mCallback.unlock (2019);
    return 0;
}

int CallbackHandler::deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener)
{
    _mCallback.lock (2020);
    int rc = _clientsPro.clear (ui16ClientId);
    _mCallback.unlock (2020);
    return rc;
}

int CallbackHandler::registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener, uint16 &ui16AssignedClientId)
{
    _mCallback.lock (2021);
    int rc = -1;
    if (pInstrumentator != NULL) {
        rc = pInstrumentator->registerAndEnableMatchmakingLogListener (ui16ClientId, pListener, ui16AssignedClientId);
    }
    _mCallback.unlock (2021);
    return rc;
}

int CallbackHandler::deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *)
{
    _mCallback.lock (2022);
    int rc = -1;
    if (pInstrumentator != NULL) {
        rc = pInstrumentator->deregisterAndDisableMatchmakingLogListener (ui16ClientId);
    }
    _mCallback.unlock (2022);
    return rc;
}

int CallbackHandler::registerControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener, uint16 &ui16AssignedClientId)
{
    _mCallback.lock (2021);
    int rc = -1;
    if (pCtrlMsgNotifier != NULL) {
        rc = pCtrlMsgNotifier->registerAndEnableControllerMessageListener (ui16ClientId, pListener, ui16AssignedClientId);
    }
    _mCallback.unlock (2021);
    return rc;
}

int CallbackHandler::deregisterControlMessageListener (uint16 ui16ClientId, ControlMessageListener *)
{
    _mCallback.lock (2022);
    int rc = -1;
    if (pCtrlMsgNotifier != NULL) {
        rc = pCtrlMsgNotifier->deregisterAndDisableControllerMessageListener (ui16ClientId);
    }
    _mCallback.unlock (2022);
    return rc;
}

int CallbackHandler::registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId)
{
    _mCallback.lock (2029);
    if (_searchListners.used (ui16ClientId) && _searchListners[ui16ClientId].pListener != NULL) {
        checkAndLogMsg ("CallbackHandler::registerSearchListener", Logger::L_SevereError,
                        "Client ID %d in use.  Register client using a different ui16ClientId.\n",
            ui16ClientId);
        ui16ClientId = _searchListners.firstFree ();
    }
    _searchListners[ui16ClientId].pListener = pListener;
    ui16AssignedClientId = ui16ClientId;
    _mCallback.unlock (2029);
    return 0;
}

int CallbackHandler::deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener)
{
    _mCallback.lock (2031);
    int rc = _searchListners.clear (ui16ClientId);
    _mCallback.unlock (2031);
    return rc;
}

//==============================================================================
// DisServicePro::ClientInfoPro
//==============================================================================

CallbackHandler::ClientInfoPro::ClientInfoPro (void)
    : pListener (NULL)
{
}

CallbackHandler::ClientInfoPro::~ClientInfoPro (void)
{
    pListener = NULL;
}

//==============================================================================
// DisServicePro::SearchInfo
//==============================================================================

CallbackHandler::SearchInfo::SearchInfo (void)
    : pListener (NULL)
{
}

CallbackHandler::SearchInfo::~SearchInfo (void)
{
}

