/* 
 * DisServiceAdaptor.cpp
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

#include "DisServiceAdaptor.h"

#include "Defs.h"
#include "DSProMessage.h"

#include "DataCacheInterface.h"
#include "DataCacheReplicationController.h"
#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "DisServiceMsgHelper.h"
#include "DSSFLib.h"
#include "Message.h"
#include "MessageProperties.h"

#include "ConfigManager.h"
#include "StrClass.h"
#include "SearchProperties.h"

#define couldNotCreateId Logger::L_Warning, "could not create disservice message id from fields: %s %s %s"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * DisServiceAdaptor::DSPRO_GROUP_NAME = "DSPro";
const char * DisServiceAdaptor::DSPRO_CTRL_TO_CTRL_GROUP_NAME = "DSProCtrl";
const uint16 DisServiceAdaptor::DSPRO_CLIENT_ID = 1;
const uint8  DisServiceAdaptor::DSPRO_SUBSCRIPTION_PRIORITY = 3;

DisServiceAdaptor * DisServiceAdaptor::_pDisServiceAdaptor = NULL;

namespace IHMC_ACI
{
    bool disServiceAndDSProSessionIDsMatch (const char *pszSessionId, const char *pszDisServiceSessionId)
    {
        if (pszDisServiceSessionId == NULL) {
            return (pszSessionId == NULL);
        }
        else if (pszSessionId == NULL) {
            return false;
        }
        return (strcmp (pszSessionId, pszDisServiceSessionId) == 0);
    }
}

DisServiceAdaptor::DisServiceAdaptor (unsigned int uiId, CommAdaptorListener *pListener,
                                      const char *pszNodeId, const char *pszSessionId,
                                      DisseminationService *pDisService)
    : CommAdaptor (uiId, DISSERVICE, true, false, pszNodeId, pszSessionId, pListener),
      DataCacheService (pDisService),
      MessagingService (pDisService),
      _pDisService (pDisService),
      _pPropertyStore (getDataCacheInterface()->getStorageInterface()->getPropertyStore()),
      _sessionIdChecker (pszSessionId),
      _periodicCtrlMessages (6U) 
{            
    _periodicCtrlMessages.put (MessageHeaders::CtxtUpdates_V1);
    _periodicCtrlMessages.put (MessageHeaders::CtxtUpdates_V2);
    _periodicCtrlMessages.put (MessageHeaders::CtxtVersions_V1);
    _periodicCtrlMessages.put (MessageHeaders::CtxtVersions_V2);
    _periodicCtrlMessages.put (MessageHeaders::WayPoint);
    _periodicCtrlMessages.put (MessageHeaders::CtxtWhole_V1);
}

DisServiceAdaptor::~DisServiceAdaptor()
{
    if (_pDisService != NULL) {
        if (_pDisService->isRunning()) {
            _pDisService->requestTerminationAndWait();
        }
        delete _pDisService;
        _pDisService = NULL;
    }
}

bool DisServiceAdaptor::checkGroupName (const char *pszIncomingGroupName, const char *pszExpectedGroupName)
{
    if (pszIncomingGroupName == NULL || pszExpectedGroupName == NULL) {
        return false;
    }
    const String incomingGroupName (pszIncomingGroupName);
    // incomingGroupName could be "pszExpectedGroupName", or "pszExpectedGroupName.someSubgroup",
    // or "pszExpectedGroupName.someSubgroup.[od]", therefore I need startsWith()
    return (incomingGroupName.startsWith (pszExpectedGroupName) == 1);
}

DisServiceAdaptor * DisServiceAdaptor::getDisServiceAdaptor (unsigned int uiId, const char *pszNodeId, const char *pszSessionId,
                                                             CommAdaptorListener *pListener, ConfigManager *pCfgMgr)
{
    if (_pDisServiceAdaptor != NULL) {
        return _pDisServiceAdaptor;
    }
    if ((pszNodeId == NULL) || (pListener == NULL)) {
        return NULL;
    }

    const char *pszMethodName = "DisServiceAdaptor::getDisServiceAdaptor";

    const String dsProNodeId (pszNodeId);
    const char *pszDisServiceNodeId = NULL;
    const char *pszDisServiceNodeIdProperty = "aci.disService.nodeUUID";
    if (!pCfgMgr->hasValue (pszDisServiceNodeIdProperty)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "%s does not exist. Setting it to %s\n",
                        pszDisServiceNodeIdProperty, dsProNodeId.c_str());
        pCfgMgr->setValue (pszDisServiceNodeIdProperty, dsProNodeId);
    }
    else if (((pszDisServiceNodeId = pCfgMgr->getValue (pszDisServiceNodeIdProperty)) == NULL) || (dsProNodeId != pszDisServiceNodeId)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "%s is set to %s, while the dspro node id is set to %s. "
                        "Overriding the DisService node id to %s\n", pszDisServiceNodeIdProperty, pszDisServiceNodeId,
                        dsProNodeId.c_str(), dsProNodeId.c_str());
    }

    // Avoid setting any search controller, DSPro will register its own
    if (pCfgMgr->getValue ("aci.disService.searchController") != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "forcing aci.disService.searchController value to NONE\n");
    }
    pCfgMgr->setValue ("aci.disService.searchController", "NONE");

    DisseminationService *pDisService = new DisseminationService (pCfgMgr);
    if (pDisService == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return NULL;
    }

    if (pDisService->init() < 0) {
        delete pDisService;
        return NULL;
    }

    if (!disServiceAndDSProSessionIDsMatch (pszSessionId, pDisService->getSessionId())) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "DisService sessionKey (%s), "
                        "and dspro sessionKey (%s) do not match.\n",
                        (pszSessionId == NULL ? "NULL" : pszSessionId),
                        (pDisService->getSessionId() == NULL ? "NULL" : pDisService->getSessionId()));
        return NULL;
    }

    String dataSubscription (DSPRO_GROUP_NAME);
    dataSubscription += ".*";
    int rc = pDisService->subscribe (DSPRO_CLIENT_ID, dataSubscription,
                                     DSPRO_SUBSCRIPTION_PRIORITY,
                                     false,     // Group reliable
                                     true,      // Message reliable
                                     false);    // Sequential
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to subscribe to %s group."
                        "DisService returned code %d\n", dataSubscription.c_str(), rc);
        return NULL;
    }

    rc = pDisService->subscribe (DSPRO_CLIENT_ID, DSPRO_CTRL_TO_CTRL_GROUP_NAME,
                                 DSPRO_SUBSCRIPTION_PRIORITY,
                                 false,    // Group reliable
                                 true,     // Message reliable
                                 true);    // Sequential
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to subscribe to %s group."
                        "DisService returned code %d\n", DSPRO_CTRL_TO_CTRL_GROUP_NAME, rc);
        return NULL;
    }

    _pDisServiceAdaptor = new DisServiceAdaptor (uiId, pListener, dsProNodeId,
                                                 pszSessionId, pDisService);
    if (_pDisServiceAdaptor == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return NULL;
    }

    rc = pDisService->registerDisseminationServiceListener (DSPRO_CLIENT_ID, _pDisServiceAdaptor);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to register to DisseminationServiceListener. Return code: %d\n", rc);
        return NULL;
    }

    unsigned int uiIndex = 0;
    rc = pDisService->registerMessageListener (_pDisServiceAdaptor, uiIndex);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to register to MessageListener. Return code: %d\n", rc);
        return NULL;
    }

    rc = pDisService->registerPeerStateListener (_pDisServiceAdaptor, uiIndex);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to register to MessageListener. Return code: %d\n", rc);
        return NULL;
    }

    return _pDisServiceAdaptor;
}

DisseminationService * DisServiceAdaptor::getDisseminationService (void)
{
    return _pDisService;
}

int DisServiceAdaptor::init (ConfigManager *pConfMgr)
{
    // const char *pszMethodName = "DisServiceAdaptor::init";

    return 0;
}

int DisServiceAdaptor::startAdaptor (void)
{
    if (_pDisService == NULL) {
        return -1;
    }
    _pDisService->start();
    return 0;
}

int DisServiceAdaptor::stopAdaptor (void)
{
    if (_pDisService == NULL) {
        return -1;
    }
    _pDisService->requestTerminationAndWait();
    return 0;
}

void DisServiceAdaptor::newIncomingMessage (const void *, uint16,
                                            DisServiceMsg *pDisServiceMsg,
                                            uint32, const char *pszIncomingInterface)
{
    if (pDisServiceMsg == NULL) {
        return;
    }

    if (!_sessionIdChecker.checkSessionId (pDisServiceMsg->getSessionId())) {
        checkAndLogMsg ("DisServiceAdaptor::newIncomingMessage", Logger::L_Info,
                        "received message from peer with wrong session key: %s\n",
                        pDisServiceMsg->getSessionId());
        return;
    }

    bool bTargetSpecified = false;
    const bool bIsTarget = DisServiceMsgHelper::isTarget (_pDisService->getNodeId(), pDisServiceMsg, bTargetSpecified);

    switch (pDisServiceMsg->getType()) {

        case DisServiceMsg::DSMT_Data:
            if (!bTargetSpecified || bIsTarget) {
                handleDataMsg (static_cast<DisServiceDataMsg*>(pDisServiceMsg), pszIncomingInterface);
            }
            break;

        case DisServiceMsg::CRMT_Query:
            handleChunkRetrievalMsg (static_cast<ChunkRetrievalMsgQuery*>(pDisServiceMsg), pszIncomingInterface);
            break;

        case DisServiceMsg::CRMT_QueryHits:
            break;

        case DisServiceMsg::DSMT_CtrlToCtrlMessage:
            handleCtrlToCtrlMsg (static_cast<ControllerToControllerMsg*>(pDisServiceMsg), pszIncomingInterface);
            break;

        case DisServiceMsg::DSMT_SearchMsg:
            handleSearchMsg (static_cast<SearchMsg*>(pDisServiceMsg), pszIncomingInterface);
            break;

        case DisServiceMsg::DSMT_SearchMsgReply:
            handleSearchReplyMsg (static_cast<SearchReplyMsg*>(pDisServiceMsg), pszIncomingInterface);
            break;

        case DisServiceMsg::DSMT_VolatileSearchMsgReply:
            handleSearchReplyMsg (static_cast<VolatileSearchReplyMsg*>(pDisServiceMsg), pszIncomingInterface);
            break;

        default:
            return;
    }
}

void DisServiceAdaptor::handleDataMsg (DisServiceDataMsg *pDisServiceMsg, const char *pszIncomingInterface)
{
    MessageHeader *pMH = pDisServiceMsg->getMessageHeader();
    if (pMH == NULL) {
        return;
    }
    if (!pMH->isCompleteMessage()) {
        return;
    }
    if (strcmp (pDisServiceMsg->getMessageHeader()->getGroupName(), DSPRO_CTRL_TO_CTRL_GROUP_NAME) != 0) {
        return;
    }

    uint32 ui32MsgLen = pDisServiceMsg->getMessageHeader()->getFragmentLength();
    BufferReader br (pDisServiceMsg->getPayLoad(), ui32MsgLen);
    ControllerToControllerMsg ctrlMsg;
    ctrlMsg.read (&br, ui32MsgLen);

    handleCtrlToCtrlMsg (&ctrlMsg, pszIncomingInterface);
}

void DisServiceAdaptor::handleChunkRetrievalMsg (ChunkRetrievalMsgQuery *pChunkRetrievalQueryMsg,
                                                 const char *pszIncomingInterface)
{
    _pListener->chunkRequestMessageArrived (getAdaptorId(), pChunkRetrievalQueryMsg->getSenderNodeId(),
                                            pChunkRetrievalQueryMsg->getSenderNodeId(),
                                            pChunkRetrievalQueryMsg->getQueryId(),
                                            pChunkRetrievalQueryMsg->getLocallyCachedChunks());
}

void DisServiceAdaptor::handleCtrlToCtrlMsg (ControllerToControllerMsg *pCtrlMsg,
                                             const char *pszIncomingInterface)
{
    if (pCtrlMsg == NULL) {
        return;
    }
    if ((pCtrlMsg->getControllerType() != DisseminationService::DCReplicationCtrl) ||
        (pCtrlMsg->getControllerVersion() != DataCacheReplicationController::DCRC_DSPro)) {
        return;
    }
    
    uint8 *pType = (uint8 *) pCtrlMsg->getMetaData();
    if (pType == NULL) {
        checkAndLogMsg ("DisServiceAdaptor::newIncomingMessage", Logger::L_Warning,
                        "controller to controller message of NULL type.\n");
        return;
    }

    void *pBuf = pCtrlMsg->getData();
    if (pBuf == NULL || pCtrlMsg->getDataLength() == 0) {
        checkAndLogMsg ("DisServiceAdaptor::newIncomingMessage", Logger::L_Warning,
                        "controller to controller message contains NULL payload.\n");
    }

    char *pszPublisherNodeId = NULL;
    uint32 ui32BytesReadForPublisherNodeId = 0;
    uint32 ui32RemainingBytes = pCtrlMsg->getDataLength();
    const char *pDataBuf = (const char *) pBuf;
    switch (*pType) {
        case MessageHeaders::TopoReply:
            break;

        case MessageHeaders::CtxtUpdates_V1:
        case MessageHeaders::CtxtVersions_V1:
        case MessageHeaders::WayPoint:
        case MessageHeaders::CtxtWhole_V1:
            pszPublisherNodeId = DSProMessage::readMessagePublisher (pBuf, pCtrlMsg->getDataLength(),
                                                                     ui32BytesReadForPublisherNodeId);
            if (pszPublisherNodeId == NULL) {
                return;
            }

            ui32RemainingBytes -= ui32BytesReadForPublisherNodeId;
            if (ui32RemainingBytes == 0) {
                free (pszPublisherNodeId);
                return;
            }

            pDataBuf = pDataBuf + ui32BytesReadForPublisherNodeId;

            break;

        default:
            checkAndLogMsg ("DisServiceAdaptor::newIncomingMessage",
                            Logger::L_Warning, "controller to controller message of unknown type: %d\n",
                            *pType);
            return;
    }

    switch (*pType) {

        case MessageHeaders::TopoReply:
            _pListener->topologyReplyMessageArrived (getAdaptorId(), pCtrlMsg->getSenderNodeId(),
                                                     pDataBuf, ui32RemainingBytes);
            break;

        case MessageHeaders::CtxtUpdates_V1:
            _pListener->updateMessageArrived (getAdaptorId(), pCtrlMsg->getSenderNodeId(),
                                              pszPublisherNodeId, pDataBuf, ui32RemainingBytes);
            break;

        case MessageHeaders::CtxtVersions_V1:
            _pListener->versionMessageArrived (getAdaptorId(), pCtrlMsg->getSenderNodeId(),
                                               pszPublisherNodeId, pDataBuf, ui32RemainingBytes);
            break;

        case MessageHeaders::WayPoint:
            _pListener->waypointMessageArrived (getAdaptorId(), pCtrlMsg->getSenderNodeId(),
                                                pszPublisherNodeId, pDataBuf, ui32RemainingBytes);
            break;

        case MessageHeaders::CtxtWhole_V1:
            _pListener->wholeMessageArrived (getAdaptorId(), pCtrlMsg->getSenderNodeId(),
                                             pszPublisherNodeId, pDataBuf, ui32RemainingBytes);
            break;

        default:
            checkAndLogMsg ("DisServiceAdaptor::newIncomingMessage",
                            Logger::L_Warning, "controller to controller message of unknown type: %d\n",
                            *pType);
    }

    free (pszPublisherNodeId);
}

void DisServiceAdaptor::handleSearchMsg (SearchMsg *pSearchMsg, const char *)
{
    if (pSearchMsg == NULL) {
        return;
    }
    SearchProperties searchProp;
    searchProp.pszGroupName = pSearchMsg->getGroupName();
    searchProp.pszQueryId = pSearchMsg->getQueryId();
    searchProp.pszQuerier = pSearchMsg->getQuerier();
    searchProp.pszQueryQualifiers = pSearchMsg->getQueryQualifier();
    searchProp.pszQueryType = pSearchMsg->getQueryType();
    unsigned int uiQueryLen = 0;
    searchProp.pQuery = pSearchMsg->getQuery (uiQueryLen);
    searchProp.uiQueryLen = uiQueryLen;
    _pListener->searchMessageArrived (getAdaptorId(), pSearchMsg->getSenderNodeId(), &searchProp);
}

void DisServiceAdaptor::handleSearchReplyMsg (SearchReplyMsg *pSearchReplyMsg, const char *)
{
    if (pSearchReplyMsg == NULL) {
        return;
    }
    _pListener->searchReplyMessageArrived (getAdaptorId(), pSearchReplyMsg->getSenderNodeId(), pSearchReplyMsg->getQueryId(),
                                           pSearchReplyMsg->getMatchingMsgIds(), pSearchReplyMsg->getQuerier(),
                                           pSearchReplyMsg->getMatchingNode());
}

void DisServiceAdaptor::handleSearchReplyMsg (VolatileSearchReplyMsg *pSearchReplyMsg, const char *pszIncomingInterface)
{
    if (pSearchReplyMsg == NULL) {
        return;
    }
    uint16 ui16ReplyLen = 0;
    const void *pReply = pSearchReplyMsg->getReply (ui16ReplyLen);
    _pListener->searchReplyMessageArrived (getAdaptorId(), pSearchReplyMsg->getSenderNodeId(), pSearchReplyMsg->getQueryId(),
                                           pReply, ui16ReplyLen, pSearchReplyMsg->getQuerier(), pSearchReplyMsg->getMatchingNode());
}

void DisServiceAdaptor::newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                     const char *pszIncomingInterface)
{
    _pListener->newPeer (&_adptorProperties, pszNodeUID, pszPeerRemoteAddr, pszIncomingInterface);
}

void DisServiceAdaptor::deadNeighbor (const char *pszNodeUID)
{
    _pListener->deadPeer (&_adptorProperties, pszNodeUID);
}

void DisServiceAdaptor::newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                           const char *pszIncomingInterface)
{
    _pListener->newLinkToPeer (&_adptorProperties, pszNodeUID, pszPeerRemoteAddr,
                               pszIncomingInterface);
}

void DisServiceAdaptor::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
    _pListener->droppedLinkToPeer (&_adptorProperties, pszNodeUID, pszPeerRemoteAddr);
}

void DisServiceAdaptor::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

bool DisServiceAdaptor::dataArrived (uint16, const char *pszSender, const char *pszGroupName,
                                     uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                     const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                     const void *pData, uint32 ui32Length, uint32, uint16, uint8,
                                     const char *pszQueryId)
{
    if (pszSender == NULL || pszGroupName == NULL || pData == NULL || ui32Length == 0) {
        return false;
    }
    if (!checkGroupName (pszGroupName, DSPRO_GROUP_NAME)) {
        return false;
    }
    if (checkGroupName (pszGroupName, DSPRO_CTRL_TO_CTRL_GROUP_NAME)) {
        return false;
    }

    char *pszId = convertFieldToKey (pszGroupName, pszSender, ui32SeqId);
    if (pszId == NULL) {
        checkAndLogMsg ("DisServiceAdaptor::dataArrived", couldNotCreateId,
                        pszGroupName, pszSender, ui32SeqId);
        return false;
    }

    MessageHeaders::MsgType type;
    uint32 ui32NewLen = 0;
    void *pNewData = MessageHeaders::removeDSProHeader (pData, ui32Length, ui32NewLen, type);
    if (pNewData == NULL || ui32NewLen == 0) {
        free (pszId);
        return false;
    }

    uint32 ui32AnnotationMetadataLen = 0U;
    void *pAnnotationMetadataBuf = NULL;
    if (pszAnnotatedObjMsgId != NULL) {
        // Retrieve the annotation metadata
        pAnnotationMetadataBuf = getDataCacheInterface()->getAnnotationMetadata (pszGroupName, pszSender, ui32SeqId, ui32AnnotationMetadataLen);
    }

    char *pszChecksum = NULL;
    MessageProperties msgProp (pszSender, pszId, pszObjectId, pszInstanceId,
                               pszAnnotatedObjMsgId, pAnnotationMetadataBuf,
                               ui32AnnotationMetadataLen, pszMimeType, pszChecksum,
                               pszQueryId, 0);

    switch (type) {
        case MessageHeaders::Data:
            _pListener->dataArrived (&_adptorProperties, &msgProp, pNewData, ui32NewLen, 1, 1);
            break;

        case MessageHeaders::Metadata: {
            const char *pszRefersTo = NULL;
            _pListener->metadataArrived (&_adptorProperties, &msgProp, pNewData, ui32NewLen, pszRefersTo);
            break;
        }

        default:
            checkAndLogMsg ("DisServiceAdaptor::dataArrived", Logger::L_Warning,
                            "unknown dspro data message type: %u", type);
            break;
    }

    free (pNewData);
    free (pszId);
    return true;
}

bool DisServiceAdaptor::chunkArrived (uint16, const char *pszSender, const char *pszGroupName,
                                      uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                      const char *pszMimeType, const void *pChunk, uint32 ui32Length,
                                      uint8 ui8NChunks, uint8 ui8TotNChunks, const char *,
                                      uint16, uint8, const char *pszQueryId)
{
    if (pszSender == NULL || pszGroupName == NULL || pChunk == NULL || ui32Length == 0) {
        return false;
    }
    if (!checkGroupName (pszGroupName, DSPRO_GROUP_NAME)) {
        return false;
    }
    if (checkGroupName (pszGroupName, DSPRO_CTRL_TO_CTRL_GROUP_NAME)) {
        return false;
    }

    char *pszOnDemandGroupName = getOnDemandDataGroupName (pszGroupName);
    if (pszOnDemandGroupName == NULL) {
        checkAndLogMsg ("DisServiceAdaptor::chunkArrived", memoryExhausted);
        return false;
    }

    char *pszId = convertFieldToKey (pszOnDemandGroupName, pszSender, ui32SeqId);
    if (pszId == NULL) {
        checkAndLogMsg ("DisServiceAdaptor::chunkArrived", couldNotCreateId,
                        pszId, pszGroupName, pszSender, ui32SeqId);
        free (pszOnDemandGroupName);
        return false;
    }

    char *pszChecksum = NULL;
    MessageProperties msgProp (pszSender, pszId, pszObjectId, pszInstanceId,
                               NULL, NULL, 0U, pszMimeType, pszChecksum, pszQueryId, 0);

    _pListener->dataArrived (&_adptorProperties, &msgProp, pChunk, ui32Length, ui8NChunks, ui8TotNChunks);

    free (pszOnDemandGroupName);
    free (pszId);
    return true;
}

bool DisServiceAdaptor::metadataArrived (uint16, const char *pszSender, const char *pszGroupName,
                                         uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                         const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                                         bool, uint16, uint8, const char *pszQueryId)
{
    if (pszSender == NULL || pszGroupName == NULL || pMetadata == NULL || ui32MetadataLength <= 1) {
        return false;
    }
    if (!checkGroupName (pszGroupName, DSPRO_GROUP_NAME)) {
        return false;
    }
    if (checkGroupName (pszGroupName, DSPRO_CTRL_TO_CTRL_GROUP_NAME)) {
        return false;
    }

    char *pszId = convertFieldToKey (pszGroupName, pszSender, ui32SeqId);
    if (pszId == NULL) {
        checkAndLogMsg ("DisServiceAdaptor::metadataArrived", couldNotCreateId,
                        pszId, pszGroupName, pszSender, ui32SeqId);
        return false;
    }

    char *pszChecksum = NULL;
    MessageProperties msgProp (pszSender, pszId, pszObjectId, pszInstanceId,
                               NULL, NULL, 0U, pszMimeType, pszChecksum, pszQueryId, 0);

    _pListener->metadataArrived (&_adptorProperties, &msgProp, pMetadata, ui32MetadataLength, NULL);

    free (pszId);
    return true;
}

bool DisServiceAdaptor::dataAvailable (uint16, const char *pszSender, const char *pszGroupName,
                                       uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                       const char *pszMimeType, const char *pszRefObjId, const void *pMetadata,
                                       uint32 ui32MetadataLength, uint16, uint8,
                                       const char *pszQueryId)
{
    if (pszSender == NULL || pszGroupName == NULL ||
        pMetadata == NULL || ui32MetadataLength == 0 || pszRefObjId == NULL) {
        return false;
    }
    if (!checkGroupName (pszGroupName, DSPRO_GROUP_NAME)) {
        return false;
    }
    if (checkGroupName (pszGroupName, DSPRO_CTRL_TO_CTRL_GROUP_NAME)) {
        return false;
    }

    char *pszId = convertFieldToKey (pszGroupName, pszSender, ui32SeqId);
    if (pszId == NULL) {
        checkAndLogMsg ("DisServiceAdaptor::dataAvailable", couldNotCreateId,
                        pszId, pszGroupName, pszSender, ui32SeqId, pszRefObjId);
        return false;
    }

    char *pszChecksum = NULL;
    MessageProperties msgProp (pszSender, pszId, pszObjectId, pszInstanceId,
                               NULL, NULL, 0U, pszMimeType, pszChecksum, pszQueryId, 0);

    _pListener->metadataArrived (&_adptorProperties, &msgProp, pMetadata, ui32MetadataLength, pszRefObjId);

    free (pszId);
    return true;
}

void DisServiceAdaptor::resetTransmissionCounters (void)
{
    // There's nothing I can do for now.  When the exponential backoff of the
    // missing frsgment requests (and possibly hello messages) will be implemented
    // they may be reset here.
}

int DisServiceAdaptor::sendContextUpdateMessage (const void *pBuf, uint32 ui32BufLen,
                                                 const char **ppszRecipientNodeIds,
                                                 const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, NULL, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::CtxtUpdates_V2);
}

int DisServiceAdaptor::sendContextVersionMessage (const void *pBuf, uint32 ui32BufLen,
                                                  const char **ppszRecipientNodeIds,
                                                  const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, NULL, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::CtxtVersions_V2);
}

int DisServiceAdaptor::sendDataMessage (Message *pMsg, const char **ppszRecipientNodeIds,
                                        const char **ppszInterfaces)
{
    if (pMsg == NULL) {
        return -1;
    }

    StringHashset hs (true, false, false);
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        hs.put (ppszRecipientNodeIds[i]);
    }
    String multipleTargets (DisServiceMsgHelper::getMultiNodeTarget (hs));

    // TODO: concatenate recipients!
    DisServiceDataMsg msg (_pDisService->getNodeId(), pMsg, multipleTargets);
    if (multipleTargets.length() <= 0) {
        const char *pszMsgId = NULL;
        MessageHeader *pMH = pMsg->getMessageHeader();
        if (pMH != NULL) {
            pszMsgId = pMH->getMsgId();
        }
        checkAndLogMsg ("DisServiceAdaptor::sendDataMessage", Logger::L_Warning,
                        "can not set target for data message %s\n",
                        pszMsgId != NULL ? pszMsgId : "");
    }

    return broadcastDataMessage (&msg, "DisServiceAdaptor", ppszInterfaces, NULL, NULL);
}

int DisServiceAdaptor::sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces)
{
    if (pMsg == NULL) {
        return -1;
    }

    // TODO: concatenate recipients!
    DisServiceDataMsg msg (_pDisService->getNodeId(), pMsg);
    if (ppszRecipientNodeIds == NULL || ppszRecipientNodeIds[0] == NULL) {
        const char *pszMsgId = NULL;
        MessageHeader *pMH = pMsg->getMessageHeader();
        if (pMH != NULL) {
            pszMsgId = pMH->getMsgId();
        }
        checkAndLogMsg ("DisServiceAdaptor::sendChunkedMessage", Logger::L_Warning,
                        "can not set target for data message %s\n",
                        pszMsgId != NULL ? pszMsgId : "");
    }
    else {
        msg.setTargetNodeId (ppszRecipientNodeIds[0]);
    }
    return broadcastDataMessage (&msg, "DisServiceAdaptor", NULL, NULL);
}

int DisServiceAdaptor::sendMessageRequestMessage (const char *pszMsgId, const char *,
                                                  const char **, const char **)
{
    if (pszMsgId == NULL) {
        return -1;
    }
    const char *pszMethodName = "DisServiceAdaptor::sendMessageRequestMessage";

    int rc = 0;
    const char *pszOperation = NULL;
    if (isOnDemandDataID (pszMsgId) && isAllChunksMessageID (pszMsgId)) {
        // It's pointing to a large object - retrieve whatever is in the
        // cache or search or run discovery for any chunk
        pszOperation = "retrieve";
        void *pBuf = NULL;
        uint32 ui32BufSize = 0;
        rc = _pDisService->retrieve (pszMsgId, &pBuf, &ui32BufSize, 0);
        if (pBuf != NULL && ui32BufSize > 0) {
            // it returned something - rc is the number of bytes that were read:
            // retrieve does not need to be called again, if the application needs
            // more chunks, it will request for them, and requestMoreChunks()
            // will be used instead
            free (pBuf);
            pBuf = NULL;
            ui32BufSize = 0;
            rc = 0;            
        }
    }
    else {
        pszOperation = "request";
        rc = _pDisService->historyRequest (DSPRO_CLIENT_ID,// client ID
                                           pszMsgId,       // pszMessageID
                                           0);             // timeout (very short! because
                                                           // dspro is taking care of re-requesting
                                                           // when necessary)
    }

    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Can not request the data for message with id = <%s>. "
                        "Returned %d\n", pszMsgId, rc);
        return -2;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Called %s for <%s>.\n", pszOperation, pszMsgId);

    return 0;
}

int DisServiceAdaptor::sendChunkRequestMessage (const char *pszMsgId, DArray<uint8> *pCachedChunks,
                                                const char *, const char **, const char **)
{
    if (pszMsgId == NULL) {
        return -1;
    }
    const char *pszMethodName = "DisServiceAdaptor::sendChunkRequestMessage";

    int rc = 0;
    if (isOnDemandDataID (pszMsgId) && isAllChunksMessageID (pszMsgId)) {
        const int64 i64Timeout = (pCachedChunks == NULL ? 1 : (pCachedChunks->size() <= 0 ? 1 : 0));
        rc = _pDisService->requestMoreChunks (DSPRO_CLIENT_ID, pszMsgId, i64Timeout);
    }

    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Can not request more chunks for "
                        "message with id = <%s>. Returned %d\n", pszMsgId, rc);
        return -2;
    }

    if (rc > 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "All the chunks for message "
                        "with id = <%s>. have already been requested\n", pszMsgId);
    }
    else {
        String chunkIds ("");
        if (pCachedChunks != NULL) {
            for (unsigned int i = 0; i < pCachedChunks->size(); i++) {
                chunkIds += ((uint32) (*pCachedChunks)[i]);
                chunkIds += ((char) ' ');
            }
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Requested more chunks for <%s>. "
                        "The already received chunks are: <%s>\n", pszMsgId, chunkIds.c_str());
    }

    return 0;
}
                                         
int DisServiceAdaptor::sendPositionMessage (const void *pBuf, uint32 ui32BufLen,
                                            const char **ppszRecipientNodeIds,
                                            const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, NULL, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::Position);
}

int DisServiceAdaptor::sendSearchMessage (SearchProperties &searchProp,
                                          const char **ppszRecipientNodeIds,
                                          const char **)
{
    if (searchProp.pszGroupName == NULL || searchProp.pszQuerier == NULL ||
        searchProp.pQuery == NULL || searchProp.uiQueryLen == 0) {
        return -1;
    }

    if (searchProp.pszQueryId == NULL) {
        searchProp.pszQueryId = SearchService::getSearchId (searchProp.pszQuerier, searchProp.pszGroupName, _pPropertyStore);
        if (searchProp.pszQueryId == NULL) {
            return -2;
        }
    }

    BufferWriter bw (1024, 1024);
    int rc = 0;
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        SearchMsg srcMsg (_pDisService->getNodeId(), ppszRecipientNodeIds[i]);
        srcMsg.setQuery (searchProp.pQuery, searchProp.uiQueryLen);
        srcMsg.setQuerier (searchProp.pszQuerier);
        srcMsg.setQueryId (searchProp.pszQueryId);
        srcMsg.setGroupName (searchProp.pszGroupName);
        srcMsg.setQueryType (searchProp.pszQueryType);
        srcMsg.setQueryQualifier (searchProp.pszQueryQualifiers);

        bw.reset();
        srcMsg.write (&bw, 0);

        int rcTmp = transmitCtrlMessage (&srcMsg, "dspro sending search message");
        if (rcTmp < 0) {
            checkAndLogMsg ("DisServiceAdaptor::sendSearchMessage", Logger::L_Warning,
                            "could not send search message to node  <%s>. Returned error %d\n",
                            ppszRecipientNodeIds[i], rcTmp);
        }
        if (rcTmp < 0 && rc == 0) {
            rc = rcTmp;
        }
    }

    return (rc == 0 ? 0 : -3);
}

int DisServiceAdaptor::sendSearchReplyMessage (const char *pszQueryId,
                                               const char **ppszMatchingMsgIds,
                                               const char *pszTarget,
                                               const char *pszMatchingNode,
                                               const char **ppszRecipientNodeIds,
                                               const char **)
{
    if (pszQueryId == NULL || ppszMatchingMsgIds == NULL) {
        return -1;
    }

    BufferWriter bw (1024, 1024);
    int rc = 0;
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        SearchReplyMsg srcMsg (_pDisService->getNodeId(), pszTarget);
        srcMsg.setQueryId (pszQueryId);
        srcMsg.setQuerier (pszTarget);
        srcMsg.setMatchingNode (pszMatchingNode);
        srcMsg.setMatchingMsgIds (ppszMatchingMsgIds);

        bw.reset();
        srcMsg.write (&bw, 0);

        int rcTmp = transmitCtrlMessage (&srcMsg, "dspro sending search reply message");
        if (rcTmp < 0) {
            checkAndLogMsg ("DisServiceAdaptor::sendSearchReplyMessage", Logger::L_Warning,
                            "could not send search message to node  <%s>. Returned error %d\n",
                            ppszRecipientNodeIds[i], rcTmp);
        }
        if (rcTmp < 0 && rc == 0) {
            rc = rcTmp;
        }
    }

    return rc == 0 ? 0 : -3;
}

int DisServiceAdaptor::sendVolatileSearchReplyMessage (const char *pszQueryId,
                                                       const void *pReply, uint16 ui16ReplyLen,
                                                       const char *pszTarget,
                                                       const char *pszMatchingNode,
                                                       const char **ppszRecipientNodeIds,
                                                       const char **ppszInterfaces)
{
    if (pszQueryId == NULL || pReply == NULL || ui16ReplyLen == 0) {
        return -1;
    }

    BufferWriter bw (1024, 1024);
    int rc = 0;
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        VolatileSearchReplyMsg srcMsg (_pDisService->getNodeId (), pszTarget);
        srcMsg.setQueryId (pszQueryId);
        srcMsg.setQuerier (pszTarget);
        srcMsg.setMatchingNode (pszMatchingNode);
        srcMsg.setReply (pReply, ui16ReplyLen);

        bw.reset();
        srcMsg.write (&bw, 0);

        int rcTmp = transmitCtrlMessage (&srcMsg, "dspro sending volatile search reply message");
        if (rcTmp < 0) {
            checkAndLogMsg ("DisServiceAdaptor::sendVolatileSearchReplyMessage", Logger::L_Warning,
                            "could not send search message to node  <%s>. Returned error %d\n",
                            ppszRecipientNodeIds[i], rcTmp);
        }
        if (rcTmp < 0 && rc == 0) {
            rc = rcTmp;
        }
    }

    return rc == 0 ? 0 : -3;
}

int DisServiceAdaptor::sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
                                                 const char **ppszRecipientNodeIds,
                                                 const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, NULL, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::TopoReply);
}

int DisServiceAdaptor::sendTopologyRequestMessage (const void *pBuf, uint32 ui32BufLen,
                                                   const char **ppszRecipientNodeIds,
                                                   const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, NULL, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::TopoReq);
}

int DisServiceAdaptor::sendUpdateMessage (const void *pBuf, uint32 ui32BufLen,
                                          const char *pszPublisherNodeId,
                                          const char **ppszRecipientNodeIds,
                                          const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, pszPublisherNodeId, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::CtxtUpdates_V1);
}

int DisServiceAdaptor::sendVersionMessage (const void *pBuf, uint32 ui32BufLen,
                                           const char *pszPublisherNodeId,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, pszPublisherNodeId, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::CtxtVersions_V1);
}

int DisServiceAdaptor::sendWaypointMessage (const void *pBuf, uint32 ui32BufLen,
                                            const char *pszPublisherNodeId,
                                            const char **ppszRecipientNodeIds,
                                            const char **ppszInterfaces)
{
    const char *pszMethodName = "DisServiceAdaptor::sendWaypointMessage";
    uint32 ui32PubLen = pszPublisherNodeId == NULL ? 0
                      : strlen (pszPublisherNodeId);
    BufferWriter bw (4U + ui32PubLen + ui32BufLen, 128U);
    int rc = 0;
    if ((rc = DSProMessageHelper::writesCtrlMsg (&bw, pszPublisherNodeId, ui32PubLen, pBuf, ui32BufLen) < 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not write wapoint message. %d\n", rc);
        return -2;
    }

    StringHashset hs (true, false, false);
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        hs.put (ppszRecipientNodeIds[i]);
    }
    const String multipleTargets (DisServiceMsgHelper::getMultiNodeTarget (hs));

    bool bSuccess = true;
    uint32 ui32BufferLen = bw.getBufferLength();
    void *pBufCpy = malloc (ui32BufferLen);
    memcpy (pBufCpy, bw.getBuffer(), ui32BufferLen);
    DSProMessage *pCtrlMsg = DSProMessage::getDSProMessage (_pDisService->getNodeId(),
                                                            multipleTargets,
                                                            MessageHeaders::WayPoint, pBufCpy, // DSProMessage's destructor
                                                            ui32BufferLen);                    // deletes pBuf! Pass a copy to it
    if (pCtrlMsg != NULL) {
        BufferWriter bwMsg;
        rc  = pCtrlMsg->write (&bwMsg);
        if (rc >= 0) {
            char msgId[256];
            rc = _pDisService->store (DSPRO_CLIENT_ID, DSPRO_CTRL_TO_CTRL_GROUP_NAME, NULL, NULL, NULL, NULL, 0,
                                      bwMsg.getBuffer(), bwMsg.getBufferLength(), 0, 0, 0, 0, msgId, 256);
            if (rc == 0) {
                char logBuf [256];
                snprintf (logBuf, sizeof (logBuf) - 1, "dspro - sending ControllerToControllerMsg message of type %s to peer %s",
                          MessageHeaders::getMetadataAsString ((uint8 *)pCtrlMsg->getMetaData()), pCtrlMsg->getTargetNodeId());
                // TODO: select the proper interfaces!
                rc = broadcastDataMessage (msgId, multipleTargets,
                                           0,     // timeout
                                           0,     // priority
                                           false, //bRequireAck,
                                           logBuf);
            }
            if (rc != 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not store or send "
                                "waypoint message to peer %s. %d\n", multipleTargets.c_str(), rc);
                bSuccess = false;
            }
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not write "
                            "waypoint message. Returned code: %d\n", rc);
        }
        delete pCtrlMsg;
    }

    return bSuccess;
}

int DisServiceAdaptor::sendWholeMessage (const void *pBuf, uint32 ui32BufLen,
                                         const char *pszPublisherNodeId,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces)
{
    return sendAndLogCtrMsg (pBuf, ui32BufLen, pszPublisherNodeId, ppszRecipientNodeIds,
                             ppszInterfaces, MessageHeaders::CtxtWhole_V1);
}

int DisServiceAdaptor::sendAndLogCtrMsg (const void *pBuf, uint32 ui32BufLen,
                                         const char *pszPublisherNodeId,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces,
                                         MessageHeaders::MsgType type)
{
    if (pBuf == NULL || ui32BufLen == 0) {
        return -1;
    }

    const char *pszMethodName = "DisServiceAdaptor::sendAndLogCtrMsg";
    int rc = 0;

    uint32 ui32PubLen = pszPublisherNodeId == NULL ? 0
                      : strlen (pszPublisherNodeId);
    for (int i = 0; ppszRecipientNodeIds[i] != NULL; i++) {
        BufferWriter bw (4U + ui32PubLen + ui32BufLen, 128U);

        if ((rc = DSProMessageHelper::writesCtrlMsg (&bw, pszPublisherNodeId, ui32PubLen, pBuf, ui32BufLen) < 0)) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not write message. %d\n", rc);
            return -2;
        }

        uint32 ui32BufferLen = bw.getBufferLength();
        DSProMessage *pCtrlMsg = DSProMessage::getDSProMessage (_pDisService->getNodeId(),
                                                                ppszRecipientNodeIds[i],
                                                                type, bw.relinquishBuffer(), // DSProMessage's destructor
                                                                ui32BufferLen);              // deletes pBuf! Pass a copy to it!
        if (pCtrlMsg != NULL) {
            char buf [256];
            snprintf (buf, sizeof (buf) - 1, "dspro - sending ControllerToControllerMsg message of type %s to peer %s",
                      MessageHeaders::getMetadataAsString ((uint8 *)pCtrlMsg->getMetaData()), pCtrlMsg->getTargetNodeId());

            int rcTmp = _periodicCtrlMessages.contains (type) ?
                        transmitUnreliableCtrlToCtrlMessage (pCtrlMsg, buf) :
                        transmitCtrlToCtrlMessage (pCtrlMsg, buf);

            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "could not send %s message to replication "
                                "controller on node  <%s>. Returned error %d\n",
                                MessageHeaders::getMetadataAsString ((uint8 *)pCtrlMsg->getMetaData()),
                                pCtrlMsg->getReceiverNodeID(), rcTmp);
            }
            if (rcTmp < 0 && rc == 0) {
                rc = rcTmp;
            }
            delete pCtrlMsg;
        }
    }

    return (rc == 0 ? 0 : -4);
}

