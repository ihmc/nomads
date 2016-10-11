/*
 * Controller.cpp
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

#include "Controller.h"

#include "DataStore.h"
#include "Defs.h"
#include "DSPro.h"
#include "DSProImpl.h"
#include "DSSFLib.h"
#include "InformationPull.h"
#include "InformationPush.h"
#include "InformationStore.h"
#include "Instrumentator.h"
#include "MessageProperties.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"
#include "NodeContextManager.h"
#include "NodePath.h"
#include "Pedigree.h"
#include "Publisher.h"
#include "Scheduler.h"
#include "Targets.h"
#include "Topology.h"
#include "WaypointMessageHelper.h"

#include "Searches.h"
#include "TransmissionHistoryInterface.h"

#include "BufferReader.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "SearchProperties.h"
#include "MessageForwardingPolicy.h"
#include "Rank.h"
#include "PreviousMessageIds.h"

#define misconfiguredController Logger::L_MildError, "_pDSPro has not been initialized\n"
#define misconfiguredAdaptors Logger::L_MildError, "received message from adaptor %u, but it does not exist\n"
#define checkDSPro(methodName) if (_pDSPro == NULL) { checkAndLogMsg (methodName, Logger::L_Warning, "_pDSPro is null\n"); return -1; }
#define checkAdaptorId(methodName,uiAdaptorId) if (!_pDSPro->getCommAdaptorManager()->hasAdaptor (uiAdaptorId)) { checkAndLogMsg (methodName, misconfiguredAdaptors, uiAdaptorId); return -2; }

using namespace IHMC_ACI;
using namespace NOMADSUtil;

char ** toNullTerminatedList (const char *pszString);

namespace IHMC_ACI
{
    bool addPeerToTopology (DSProImpl *pDSPro, AdaptorId uiAdaptorId, const char *pszSenderNodeId, const char *pszPublisherNodeId)
    {
        if (strcmp (pszPublisherNodeId, pszSenderNodeId) != 0) {
            AdaptorType adaptorType = UNKNOWN;
            adaptorType = pDSPro->getCommAdaptorManager ()->getAdaptorType (uiAdaptorId);
            pDSPro->getTopology ()->addLink (pszSenderNodeId, pszPublisherNodeId,
                                             Topology::DEFAULT_INTERFACE, uiAdaptorId,
                                             adaptorType);
        }
        bool bNewPeer = false;
        PeerNodeContext *pNodeContext = pDSPro->getNodeContextManager()->getPeerNodeContext (pszPublisherNodeId);
        if (pNodeContext == NULL) {    
            pDSPro->getNodeContextManager()->newPeer (pszPublisherNodeId);
            bNewPeer = true;
        }
        pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        return bNewPeer;
    }
}

Controller::Controller (DSProImpl *pDSPro, LocalNodeContext *pLocalNodeCtxt, Scheduler *pScheduler,
                        InformationStore *pInfoStore, Topology *pTopology,
                        TransmissionHistoryInterface *pTrHistory)
    : _bPreStagingEnabled (true),
      _bContextForwardingEnabled (true),
      _pDSPro (pDSPro),
      _pInfoPull (NULL),
      _pInfoPush (NULL),
      _pScheduler (pScheduler),
      _pTrHistory (pTrHistory),
      _m (MutexId::Controller_m, LOG_MUTEX),
      _nodeId (pDSPro->getNodeId()),
      _fwdCtrl (_pDSPro->getNodeId(), _pDSPro->getCommAdaptorManager(), _pDSPro->getDataStore(), pTopology, true),
      _deliveredMsgs (true,  // bCaseSensitiveKeys
                      true,  // bCloneKeys
                      true), //bDeleteKeys
      _msgFwdPolicy (pDSPro->getNodeId()),
      _rankerLocalConf (pDSPro->getNodeId(), pLocalNodeCtxt),
      _matchmakingHelper (pInfoStore, pTopology, pTrHistory)
{
}

Controller::~Controller (void)
{
}

int Controller::init (NOMADSUtil::ConfigManager *pCfgMgr,
                      InformationPushPolicy *pInfoPushPolicy, Scheduler *pScheduler)
{
    const char *pszMethodName = "Controller::init";

    if ((pInfoPushPolicy == NULL) || (pScheduler == NULL)) {
        return -1;
    }
    if (_pDSPro == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "pDSPro is NULL\n");
        return -1;
    }

    if (pCfgMgr == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "the config manager is NULL\n");
        return -2;
    }

    _bPreStagingEnabled = pCfgMgr->getValueAsBool ("aci.dspro.dsprorepctrl.prestaging.enabled", true);
    _bContextForwardingEnabled = pCfgMgr->getValueAsBool ("aci.dspro.dsprorepctrl.contextForwarding.enabled", true);
    _rankerLocalConf.init (pCfgMgr);

    _pInfoPush = new InformationPush (_nodeId, &_rankerLocalConf,
                                      _pDSPro->getNodeContextManager(), pInfoPushPolicy,
                                      pScheduler);

    const float fThreshold = (pCfgMgr->hasValue ("aci.dspro.informationPull.rankThreshold") ?
                              (float) atof (pCfgMgr->getValue("aci.dspro.informationPull.rankThreshold")) :
                              InformationPull::INFO_PULL_RANK_THRESHOLD);
    _pInfoPull = new InformationPull (_nodeId, &_rankerLocalConf,
                                      _pDSPro->getNodeContextManager(), _pDSPro->getInformationStore(),
                                      fThreshold);
    if (_pInfoPull->configure (pCfgMgr) < 0) {
        return -5;
    }

    if (_fwdCtrl.init (pCfgMgr) < 0) {
        return -6;
    }

    return 0;
}

int Controller::contextUpdateMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                             const void *pBuf, uint32 ui32Len)
{
    // TODO: implement this
    return 0;
}

int Controller::contextVersionMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                              const void *pBuf, uint32 ui32Len)
{
    // TODO: implement this
    return 0;
}

int Controller::dataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks)
{
    if (pAdaptorProperties == NULL || pProperties == NULL || pProperties->_pszPublisherNodeId == NULL ||
        pProperties->_pszMsgId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    checkDSPro ("Controller::dataArrived")
    checkAdaptorId ("Controller::dataArrived",pAdaptorProperties->uiAdaptorId)

    const char *pszMethodName = "Controller::dataArrived";

    // Since multiple chunks of a certain message will have the same pszId,
    // generate a unique ID for the chunk
    String chunkKey (pProperties->_pszMsgId);
    chunkKey += ":";
    chunkKey += static_cast<uint32>(ui8NChunks);

    _pDSPro->removeAsynchronousRequestMessage (pProperties->_pszMsgId);

    _m.lock (1000);
    if (_deliveredMsgs.containsKey (chunkKey.c_str())) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "duplicate %s arrived: %s (%s) - discarding it\n",
                        (ui8TotNChunks > 1 ? "chunk" : "data"), pProperties->_pszMsgId, chunkKey.c_str());
        _m.unlock (1000);
        return -3;
    }

    _deliveredMsgs.put (chunkKey.c_str());

    _m.unlock (1000);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "%s arrived %s (%s) (%d/%d). Lenght: %u.\n",
                    (ui8TotNChunks > 1 ? "chunk" : "data"), pProperties->_pszMsgId,
                    chunkKey.c_str(), ui8NChunks, ui8TotNChunks, ui32Len);

    if (!pAdaptorProperties->bSupportsCaching) {
        // If the message arrived from an adaptor that does not support caching, the
        // message needs to be cached
        int rc = _pDSPro->getPublisher()->addData (pProperties->_pszMsgId, pProperties->_pszObjectId,
                                                   pProperties->_pszInstanceId, pProperties->_pszAnnotatedObjMsgId,
                                                   pProperties->_pAnnotationMetadata, pProperties->_ui32AnnotationLen,
                                                   pProperties->_pszMimeType, pProperties->_pszChecksum, pBuf, ui32Len,
                                                   pProperties->_i64ExpirationTime, ui8NChunks, ui8TotNChunks);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not store "
                            "data %s. Return code: %d\n", pProperties->_pszMsgId, rc);
        }
    }

    if (pProperties->_pszAnnotatedObjMsgId == NULL) {
        // Annotations never have a corresponding metadata, therefore it should not be searched
        MetadataList *pMetadataList = _pDSPro->getInformationStore()->getMetadataForData (pProperties->_pszMsgId);
        if ((pMetadataList == NULL) || (pMetadataList->getFirst () == NULL)) {
            // No metadata referring to data was found - search it!
            // (unless I published the data... Then I should should wait for
            // the application to publish it...)
            if (0 != (strcmp (pProperties->_pszPublisherNodeId, _nodeId))) {
                String query (MetaData::REFERS_TO);
                query += " = '";
                query += pProperties->_pszMsgId;
                query += "'";
                const String grpName (extractGroupFromKey (pProperties->_pszMsgId));
                if (grpName.length () > 0) {
                    char *pszQueryId;
                    SearchProperties prop;
                    prop.pszGroupName = grpName;
                    prop.pszQueryType = DSPro::SQL_QUERY_TYPE;
                    prop.pQuery = query.c_str();
                    prop.uiQueryLen = query.length();
                    prop.i64TimeoutInMillis = 0;
                    _pDSPro->search (prop, &pszQueryId);
                    if (pszQueryId != NULL) {
                        free (pszQueryId);
                    }
                    // TODO: deallocate pMessages
                }
            }
        }
        else if (pMetadataList != NULL) {
            MetadataInterface *pCurr, *pNext;
            pNext = pMetadataList->getFirst ();
            while ((pCurr = pNext) != NULL) {
                pNext = pMetadataList->getNext ();
                delete pMetadataList->search (pCurr);
            }
            delete pMetadataList;
        }
    }

    const String sSubGrpName (MessageIdGenerator::extractSubgroupFromMsgId (pProperties->_pszMsgId));
    int rc = _pDSPro->dataArrived (pProperties->_pszMsgId, sSubGrpName, pProperties->_pszObjectId,
                                   pProperties->_pszInstanceId, pProperties->_pszAnnotatedObjMsgId,
                                   pProperties->_pszMimeType, pBuf, ui32Len, ui8NChunks, ui8TotNChunks,
                                   pProperties->_pszQueryId);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not notify data "
                        "arrived %s. Return code: %d\n", pProperties->_pszMsgId, rc);
    }

    return (rc < 0 ? -4 : 0);
}

int Controller::metadataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                                 const void *pBuf, uint32 ui32Len, const char *pszReferredDataId)
{
    const char *pszMethodName = "Controller::metadataArrived";

    if (pAdaptorProperties == NULL || pProperties == NULL ||
        pProperties->_pszPublisherNodeId == NULL ||
        pProperties->_pszMsgId == NULL ||  pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    checkDSPro (pszMethodName)
    checkAdaptorId (pszMethodName,pAdaptorProperties->uiAdaptorId)
    const String logMsg (pProperties->toString());

    _pDSPro->removeAsynchronousRequestMessage (pProperties->_pszMsgId);

    _m.lock (1001);
    if (_deliveredMsgs.containsKey (pProperties->_pszMsgId)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "duplicate metadata arrived %s - discarding\n", logMsg.c_str());
        _m.unlock (1001);
        return -3;
    }
    _deliveredMsgs.put (pProperties->_pszMsgId);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "metadata arrived %s\n", logMsg.c_str());

    _m.unlock (1001);

    // Parse pBuf into a Metadata object
    MetaData *pMetadata = _pDSPro->getMetadataConf()->createNewMetadataFromBuffer (pBuf, ui32Len);
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, dataDeserializationError, pProperties->_pszMsgId);
        return -4;
    }

    String sReferredIdTmp;
    if ((pMetadata->getFieldValue (MetaData::REFERS_TO, sReferredIdTmp) < 0) || sReferredIdTmp.length() <= 0) {
        sReferredIdTmp = pszReferredDataId;
    }
    else if ((pszReferredDataId != NULL) && (sReferredIdTmp != pszReferredDataId)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "referred object id in "
                        "metadata differs from one passed as parameter: <%s> <%s>.\n",
                        pszReferredDataId, sReferredIdTmp.c_str());
        assert (sReferredIdTmp == pszReferredDataId);
        return -5;
    }

    // Store the message if it has not already been stored by the comm adaptor
    if (!pAdaptorProperties->bSupportsCaching) {
        int rc = _pDSPro->getPublisher()->addMetadata (pProperties->_pszMsgId, pProperties->_pszObjectId,
                                                       pProperties->_pszInstanceId, pProperties->_pszMimeType,
                                                       pProperties->_pszChecksum, pszReferredDataId, pBuf,
                                                       ui32Len, pProperties->_i64ExpirationTime);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "could not store metadata. Return code: %d\n", rc);
        }
    }

    // Check whether the node is the source of the message. If this is the case,
    // it does not need to be stored again
    String sSource;
    if ((0 == pMetadata->getFieldValue (MetaData::SOURCE, sSource)) && (sSource.length() > 0)) {
        if (sSource == _nodeId) {
            // The node is the publisher of the data, I do not need store a new
            // version
            delete pMetadata;
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "the source of the "
                            "message is the node itself\n");
            return 0;
        }
    }

    // Check whether the node is in the pedigree of the message. If this is the
    // case, it does not need to be stored again
    Pedigree pedigree (sSource); // Pedigree makes a copy of pszSource
    if (0 == pMetadata->getFieldValue (pedigree)) {
        if (pedigree.containsNodeID (_nodeId, true)) {
            delete pMetadata;
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "the node is in the "
                            "pedigree of the message\n");
            return 0;
        }
    }

    // Check whether the node has already received the metadata message with a
    // shorter pedigree
    //_pDSPro->_pInfoStore->getMetadataForData()
    
    // addAnnotation is the external API's function to add an annotation,
    // therefore it takes the group of the annotation and appends it to the
    // "root" DSPro group. It is  therefore necessary to extract the specific
    // group
    const String sSubGrpName (MessageIdGenerator::extractSubgroupFromMsgId (pProperties->_pszMsgId));
    if (sSubGrpName.length() <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not extract group message id\n");
    }

    MetaData *pNewMetadata = pMetadata->clone();
    if (pNewMetadata == NULL) {
        delete pMetadata;
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return -8;
    }

    // Create pedigree for the new message
    Pedigree newMetadataPedigree (_nodeId, sSource);
    newMetadataPedigree += pedigree.toString();

    pNewMetadata->setFieldValue (MetaData::PEDIGREE, newMetadataPedigree.toString());
    pNewMetadata->resetFieldValue (MetaData::PREV_MSG_ID);
    String sNewMetadataId;
    bool bMetadataAddedInfoStore = true;
    // addAnnotationNoPrestage() modifies pMetadata - therefore I pass a copy
    Publisher::PublicationInfo pub (sSubGrpName, pProperties->_pszObjectId, pProperties->_pszInstanceId);
    pub.i64ExpirationTime = 0;
    if (_pDSPro->setAndAddMetadata (pub, pNewMetadata, sNewMetadataId, true) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "could not insert metadata %s to info store\n",
                        logMsg.c_str());
        bMetadataAddedInfoStore = false;
    }
    else {
        char *pszNewMsgId = NULL;
        int rc = pNewMetadata->getFieldValue (MetaData::MESSAGE_ID, &pszNewMsgId);
        if (rc != 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "could not extract message "
                            "id from pNewMetadata for message %s. Return code: %d\n", 
                            logMsg.c_str(), rc);
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "metadata for %s inserted "
                        "to info store with id %s\n", logMsg.c_str(), pszNewMsgId);
        free (pszNewMsgId);
    }

    previousMetadataPull (pMetadata);
    dataPull (pProperties->_pszMsgId, sReferredIdTmp, pMetadata);

    // Trigger matchmaking
    if (_bPreStagingEnabled && bMetadataAddedInfoStore) {
        // Evaluate data push
        int rc;
        if ((rc = metadataPush (sNewMetadataId, pNewMetadata, sSource) < 0)) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Can not evaluate data"
                            "push for metadata with id <%s> referring to data with id <%s>. Returned code %d\n",
                            sNewMetadataId.c_str(), pszReferredDataId, rc);
        }
    }

    delete pNewMetadata;
    pNewMetadata = NULL;

    // Notify DSPro
    int rc = _pDSPro->metadataArrived (pProperties->_pszMsgId, sSubGrpName,
                                       pProperties->_pszObjectId, pProperties->_pszInstanceId,
                                       pMetadata, sReferredIdTmp, pProperties->_pszQueryId);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not notify metadata "
                        "arrived %s. Return code: %d\n", logMsg.c_str(), rc);
    }

    delete pMetadata;
    pMetadata = NULL;

    return (rc < 0 ? -10 : 0);
}

int Controller::messageRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              const char *pszPublisherNodeId, const char *pszMsgId)
{
    if (pszSenderNodeId == NULL || pszPublisherNodeId == NULL || pszMsgId == NULL) {
        return -1;
    }

    int rc = _pScheduler->addMessageRequest (pszPublisherNodeId, pszMsgId, NULL);

    // Add remote peer to the topology - if necessary
    addPeerToTopology (_pDSPro, uiAdaptorId, pszSenderNodeId, pszPublisherNodeId);

    _fwdCtrl.messageRequestMessageArrived (uiAdaptorId, pszSenderNodeId,
                                           pszPublisherNodeId, pszMsgId);

    return rc == 0 ? 0 : -2;
}

int Controller::chunkRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                            const char *pszPublisherNodeId, const char *pszMsgId,
                                            DArray<uint8> *pCachedChunks)
{
    if (pszSenderNodeId == NULL || pszPublisherNodeId == NULL || pszMsgId == NULL) {
        return -1;
    }

    int rc = _pScheduler->addMessageRequest (pszPublisherNodeId, pszMsgId, pCachedChunks);
    if (rc != 0) {
        checkAndLogMsg ("Controller::chunkRequestMessageArrived", Logger::L_Warning,
                        "could not add message request. Return code: %d\n", rc);
    }

    // Add remote peer to the topology - if necessary
    addPeerToTopology (_pDSPro, uiAdaptorId, pszSenderNodeId, pszPublisherNodeId);

    _fwdCtrl.chunkRequestMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId,
                                         pszMsgId, pCachedChunks);

    return rc == 0 ? 0 : -2;
}

int Controller::metadataPush (const char *pszId, MetaData *pMetadata,
                              const char *pszPreviousHop)
{
    if (pszId == NULL || pMetadata == NULL) {
        return -1;
    }
    if (!_bPreStagingEnabled) {
        return 0;
    }
    const char **ppszTargetList = _pTrHistory->getTargetList (pszId);
    bool bDeallocatePeerList = false;
    PeerNodeContextList *pPeerNodeContextList;
    if (ppszTargetList == NULL) {
        pPeerNodeContextList = _pDSPro->getNodeContextManager()->getPeerNodeContextList ();
    }
    else {
        pPeerNodeContextList = _pDSPro->getNodeContextManager()->getPeerNodeContextList (ppszTargetList);
        bDeallocatePeerList = true;
    }

    if (pPeerNodeContextList == NULL) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        return 0;
    }

    if (_pDSPro->isTopologyExchangedEnabled()) {
        PeerNodeContextList *pPeerNodeContextListTmp = _msgFwdPolicy.getNodesToMatch (pPeerNodeContextList, pszPreviousHop,
                                                                                      _pDSPro->getTopology(), pszId, pMetadata);
        if (bDeallocatePeerList) {
            delete pPeerNodeContextList;
        }
        pPeerNodeContextList = pPeerNodeContextListTmp;
        bDeallocatePeerList = true;
    }

    // call push mode
    Instrumentations *pPushInstrumentations = NULL;
    if (pPeerNodeContextList != NULL) {
        _m.lock (1012);
        pPushInstrumentations = _pInfoPush->dataArrived (pMetadata, pPeerNodeContextList);
        _m.unlock (1012);
        if (bDeallocatePeerList) {
            delete pPeerNodeContextList;
            pPeerNodeContextList = NULL;
        }
    }
    _pDSPro->getNodeContextManager()->releasePeerNodeContextList();

    _pTrHistory->releaseList (ppszTargetList);
    Instrumentator::notifyAndRelease (pPushInstrumentations);

    return 0;
}

int Controller::dataPull (const char *pszMetadataId, const String &referredObjId, MetaData *pMetadata)
{
    const char *pszMethodName = "Controller::dataPull";
    MatchmakingIntrumentation *pPullInstrumentation = NULL;
    if ((referredObjId == MetaData::NO_REFERRED_OBJECT) ||
        MetadataUtils::refersToDataFromSource (pMetadata, _nodeId)) {
        NodeIdSet matchingNode (_nodeId);
        RankByTargetMap rhdRankByTarget;
        Rank *pRank = new Rank (pszMetadataId, matchingNode, rhdRankByTarget, false);
        pRank->_loggingInfo._comment = "The referred data was published by the node itself or "
                                       "it's metadata only";
        pPullInstrumentation = new MatchmakingIntrumentation (false, _nodeId, pRank);
    }
    else {
        // call pull mode
        int rc = _pDSPro->sendAsynchronousRequestMessage (referredObjId);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "can not "
                            "request referred message %s. Returned code %d\n",
                            referredObjId.c_str(), rc);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "requested "
                            "referred message %s\n", referredObjId.c_str());
        }
    }

    if (pPullInstrumentation != NULL) {
        pInstrumentator->notify (pPullInstrumentation);
        delete pPullInstrumentation;
        pPullInstrumentation = NULL;
    }

    return 0;
}

int Controller::previousMetadataPull (MetaData *pMetadata)
{
    const char *pszMethodName = "Controller::previousMetadataPull";
    String sPreviousMsg;
    PreviousMessageIds prevMsgIds;
    if (pMetadata->getFieldValue (prevMsgIds) == 0) {
        sPreviousMsg = prevMsgIds.getPreviousMessageIdForPeer (_nodeId);
    }
    if (sPreviousMsg.length () > 0) {
        _m.lock (1001);
        if (!_deliveredMsgs.containsKey (sPreviousMsg)) {
            _m.unlock (1001);
            // TODO: should I search only on the adaptor from which the metadata
            //       was received?  Or on all of them?
            int rc = _pDSPro->sendAsynchronousRequestMessage (sPreviousMsg);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "can not request previous "
                                "message %s. Returned code %d\n", sPreviousMsg.c_str (), rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "requested previous message %s\n",
                                sPreviousMsg.c_str());
            }
        }
        else {
            _m.unlock (1001);
        }
    }
    return 0;
}

int Controller::positionMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                        const void *pBuf, uint32 ui32Len)
{
    // TODO: implement this
    return 0;
}

int Controller::searchMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                      SearchProperties *pSearchProperties)
{
    if (pSearchProperties == NULL) {
        return -1;
    }

    uint16 ui16ClientId = 0xFFF;
    Searches::getSearches()->addSearchInfo (pSearchProperties->pszQueryId, pSearchProperties->pszQueryType,
                                            pSearchProperties->pszQuerier, ui16ClientId);

    // Notify the search listeners
    _pDSPro->getCallbackHandler()->searchArrived (pSearchProperties);

    _fwdCtrl.searchMessageArrived (uiAdaptorId, pszSenderNodeId, pSearchProperties);

    return 0;
}

int Controller::searchReplyMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                           const char *pszQueryId, const char **ppszMatchingMsgIds,
                                           const char *pszTarget, const char *pszMatchingNodeId)
{
   /* if (pszTarget != NULL && strcmp (pszTarget, _nodeId) == 0) {
        return 0;
    }*/

    Searches::getSearches()->addQueryReply (pszQueryId, pszMatchingNodeId);

    // Notify the search listeners
    _pDSPro->getCallbackHandler()->searchReplyArrived (pszQueryId, ppszMatchingMsgIds, pszMatchingNodeId);

    if (_fwdCtrl.searchReplyMessageArrived (adaptorId, pszSenderNodeId, pszQueryId,
                                            ppszMatchingMsgIds, pszTarget, pszMatchingNodeId) == 0) {

        // Because the message reply was forwarded I can assume the requestor
        // will request the matching messages, therefore I can go ahead and
        // proactively retrieve those messages, if they are not already cached locally.
        // (TODO: ideally only messages that will are requested by the
        // requestor should be retrieved).
        if (ppszMatchingMsgIds != NULL) {
            for (unsigned int i = 0; ppszMatchingMsgIds[i] != NULL; i++) {
                if (!_pDSPro->getDataStore()->hasData (ppszMatchingMsgIds[i])) {
                    _pDSPro->sendAsynchronousRequestMessage (ppszMatchingMsgIds[i]);
                }
            }
        }
    }

    return 0;
}

int Controller::searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                           const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                           const char *pszTarget, const char *pszMatchingNodeId)
{
    Searches::getSearches()->addQueryReply (pszQueryId, pszMatchingNodeId);

    // Notify the search listeners
    _pDSPro->getCallbackHandler()->volatileSearchReplyArrived (pszQueryId, pReply, ui16ReplyLen, pszMatchingNodeId);

    // TODO: work on forwarding
    return 0;
}

int Controller::topologyReplyMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                             const void *pBuf, uint32 ui32Len)
{
    if (pszSenderNodeId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }

    logTopology ("Controller::topologyReplyMessageArrived", Logger::L_Info,
                 "received topology message from node %s\n", pszSenderNodeId);

    PtrLList<String> neighbors;
    BufferReader br (pBuf, ui32Len);
    if (_pDSPro->getTopology()->read (&br, neighbors) == 0 && neighbors.getFirst() != NULL) {
        _pDSPro->getTopology()->replaceAllLinksForPeer (pszSenderNodeId, adaptorId, neighbors);
    }

    _fwdCtrl.topologyReplyMessageArrived (adaptorId, pszSenderNodeId, pBuf, ui32Len);

    return 0;
}

int Controller::topologyRequestMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                               const void *pBuf, uint32 ui32Len)
{
    // TODO: implement this
    return 0;
}

int Controller::updateMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                      const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    const char *pszMethodName = "Controller::updateMessageArrived";
    if (pszSenderNodeId == NULL || pszPublisherNodeId ==  NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    if (_nodeId == pszPublisherNodeId) {
        return 0;
    }

    logTopology (pszMethodName, Logger::L_Info, "%s --> %s\n", pszPublisherNodeId, pszSenderNodeId);

    addPeerToTopology (_pDSPro, uiAdaptorId, pszSenderNodeId, pszPublisherNodeId);

    PeerNodeContext *pNodeContext = _pDSPro->getNodeContextManager()->getPeerNodeContext (pszPublisherNodeId);
    const NodeContext::Versions oldVersions (pNodeContext->getVersions());
    bool bContextUnsynchronized = false;
    int retValue = _pDSPro->getNodeContextManager()->updatesMessageArrived (pBuf, ui32Len,
                                                                            pszPublisherNodeId,
                                                                            bContextUnsynchronized);
    if (retValue < 0) {
        checkAndLogMsg (pszMethodName, ctrlDeserializationError, "update", pszSenderNodeId);
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        return -2;
    }
    const NodeContext::Versions newVersions (pNodeContext->getVersions());
    const bool bContextUpdated = oldVersions.lessThan (newVersions, false);

    if (_bContextForwardingEnabled) {
        if (bContextUpdated) {
            // At least a version number has incremented - the update was useful,
            // therefore it needs to be forwarded
            Targets **ppTargets = _pDSPro->getTopology()->getForwardingTargets (_nodeId, pszSenderNodeId);
            int rc = _pDSPro->getCommAdaptorManager()->sendUpdateMessage (pBuf, ui32Len, pszPublisherNodeId, ppTargets);
            Targets::deallocateTargets (ppTargets);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to forward update message "
                                "from %s and originated from %s. Return code %d.\n", pszSenderNodeId,
                                pszPublisherNodeId, rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "update message from %s and "
                                "originated from %s was forwarded.\n", pszSenderNodeId, pszPublisherNodeId);
                logTopology ("Controller::updateMessageArrived", Logger::L_Info, "%s --> %s\n",
                             pszPublisherNodeId, pszSenderNodeId);
            }
        }
    }

    if (!bContextUpdated) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        return 0;
    }

    // Path arrived - Notify the listeners
    NodePath *pNodePath = pNodeContext->getPath();
    if ((pNodePath != NULL) && (pNodePath->getPathLength() > 0)) {
        _pDSPro->getCallbackHandler()->pathRegistered (pNodeContext->getPath (),
                                                       pNodeContext->getNodeId(),
                                                       pNodeContext->getTeamId(),
                                                       pNodeContext->getMissionId());
    }

    if (!_bPreStagingEnabled) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        return 0;
    }

    // Get the messages to be matched against the new node context
    MetadataList *pMetadataList = _matchmakingHelper.getMetadataToMatchmake (pNodeContext, pszSenderNodeId,
                                                                             _pDSPro->isTopologyExchangedEnabled());
    if (pMetadataList == NULL || pMetadataList->getFirst() == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Metadata list for peer %s "
                        "contains 0 elements.\n", pszPublisherNodeId);
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        return -5;
    }

    // Call push mode
    // Prepare Instrumentation
    _m.lock (1003);
    Instrumentations *pInstrumentations = _pInfoPush->nodeContextChanged (pMetadataList, pNodeContext);
    _m.unlock (1003);

    _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
    MatchmakingHelper::deallocateMetadataToMatchmake (pMetadataList);
    pMetadataList = NULL;

    // Notify Instrumenter and release memory
    Instrumentator::notifyAndRelease (pInstrumentations);

    return 0;
}

int Controller::versionMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                       const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    const char *pszMethodName = "Controller::versionMessageArrived";
    if (pszSenderNodeId == NULL || pszPublisherNodeId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    if (_nodeId == pszPublisherNodeId) {
        return 0;
    }

    addPeerToTopology (_pDSPro, uiAdaptorId, pszSenderNodeId, pszPublisherNodeId);

    char *pszNodeToForwardTo = NULL;
    int rc = _pDSPro->getNodeContextManager()->versionsMessageArrived (pBuf, ui32Len,
                                                                       pszPublisherNodeId,
                                                                       &pszNodeToForwardTo);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, ctrlDeserializationError, "version", pszSenderNodeId);
        if (pszNodeToForwardTo != NULL) {
            free (pszNodeToForwardTo);
        }
        return -2;
    }

    logTopology (pszMethodName, Logger::L_Info, "%s --> %s\n", pszPublisherNodeId, pszSenderNodeId);

    if (_bContextForwardingEnabled && (pszNodeToForwardTo != NULL)) {

        Targets *pNextHop = _pDSPro->getTopology()->getNextHopAsTarget (pszNodeToForwardTo);
        if (pNextHop != NULL) {
            TargetPtr targetPtrs[2] = {pNextHop, NULL};
            rc = _pDSPro->getCommAdaptorManager()->sendVersionMessage (pBuf, ui32Len, pszPublisherNodeId, targetPtrs);
            delete pNextHop;
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to forward version message from "
                                "%s to %s and originated from %s. Return code %d.\n", pszSenderNodeId,
                                pszNodeToForwardTo, pszPublisherNodeId, rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "version message from %s to %s and originated "
                                "from %s was forwarded.\n", pszSenderNodeId, pszNodeToForwardTo, pszPublisherNodeId);
            }
        }
    }

    if (pszNodeToForwardTo != NULL) {
        free (pszNodeToForwardTo);
    }
    return 0;
}

int Controller::waypointMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                        const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    const char *pszMethodName = "Controller::waypointMessageArrived";
    if (pszSenderNodeId == NULL || pszPublisherNodeId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    if (_nodeId == pszPublisherNodeId) {
        return 0;
    }

    bool bPositionHasChanged = true;

    BufferReader br (pBuf, ui32Len);
    uint32 ui32WaypointMsgPayloadLen = 0;
    PreviousMessageIds previouMessagesSentToTargets;
    int rc = WaypointMessageHelper::readWaypointMessageForTarget (pBuf, 0, ui32Len, previouMessagesSentToTargets,
                                                                  ui32WaypointMsgPayloadLen);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not read waypoint "
                        "message from peer %s\n", pszSenderNodeId);
        return -2;
    }
    const String sLatestMessageSentToTarget (previouMessagesSentToTargets.getPreviousMessageIdForPeer (_nodeId));

    logTopology (pszMethodName, Logger::L_Info, "%s --> %s\n", pszPublisherNodeId, pszSenderNodeId);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "the latest message that was pushed from %s was %s\n",
                    pszSenderNodeId, sLatestMessageSentToTarget.length() > 0 ? sLatestMessageSentToTarget.c_str() : "NULL");

    // Request previous message (DisseminationService::request checks whether the
    // message is already in the cache before requesting it)
    if (sLatestMessageSentToTarget.length() > 0) {
        _m.lock (1004);
        if (!_deliveredMsgs.containsKey (sLatestMessageSentToTarget)) {
            // TODO: should I search only on the adaptor from which the metadata
            //       was received?  Or on all of them?
            int rc = _pDSPro->sendAsynchronousRequestMessage (sLatestMessageSentToTarget);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "can not request message %s. Error code: %d.\n",
                                sLatestMessageSentToTarget.c_str(), rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "requested message %s\n", sLatestMessageSentToTarget.c_str());
            }
        }
        _m.unlock (1004);
    }

    if (addPeerToTopology (_pDSPro, uiAdaptorId, pszSenderNodeId, pszPublisherNodeId)) {
        return 0;
    }

    char *pWaypointMsgPayload = (char *) pBuf;
    pWaypointMsgPayload = pWaypointMsgPayload + (ui32Len - ui32WaypointMsgPayloadLen);

    rc = _pDSPro->getNodeContextManager()->wayPointMessageArrived (pWaypointMsgPayload, ui32WaypointMsgPayloadLen,
                                                                   pszPublisherNodeId, bPositionHasChanged);
    if (rc < 0) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        checkAndLogMsg (pszMethodName, ctrlDeserializationError, "waypoint", pszSenderNodeId);
        return -2;
    }

    if (!bPositionHasChanged) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "The way point for %s has not changed.\n",
                        pszPublisherNodeId);
        return -3;
    }

    // Forward message
    _fwdCtrl.waypointMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId,
                                     pWaypointMsgPayload, ui32WaypointMsgPayloadLen);

    // New position for peer - Notify the listeners
    float fLatitude, fLongitude, fAltitude;
    PeerNodeContext *pNodeContext = _pDSPro->getNodeContextManager()->getPeerNodeContext (pszPublisherNodeId);
    const bool bNotifyDSPro = (pNodeContext->getCurrentPosition (fLatitude, fLongitude, fAltitude) == 0);

    if (!_bPreStagingEnabled) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Pre-staging is not enabled.\n", pszSenderNodeId);
        if (bNotifyDSPro) {
            _pDSPro->getCallbackHandler()->positionUpdated (fLatitude, fLongitude, fAltitude, pszPublisherNodeId);
        }
        return -5;
    }

    // Get the messages to be matched against the new node context
    MetadataList *pMetadataList = _matchmakingHelper.getMetadataToMatchmake (pNodeContext, pszSenderNodeId,
                                                                             _pDSPro->isTopologyExchangedEnabled ());
    if (pMetadataList == NULL || pMetadataList->getFirst() == NULL) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Metadata list for peer %s "
                        "contains 0 elements.\n", pszPublisherNodeId);
        if (bNotifyDSPro) {
            _pDSPro->getCallbackHandler()->positionUpdated (fLatitude, fLongitude, fAltitude, pszPublisherNodeId);
        }
        return -6;
    }

    // Call push mode
    _m.lock (1005);
    PtrLList<MatchmakingIntrumentation> *pInstrumentations = _pInfoPush->nodeContextChanged (pMetadataList, pNodeContext);
    _m.unlock (1005);

    _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
    MatchmakingHelper::deallocateMetadataToMatchmake (pMetadataList);

    // Notify Instrumenter and release memory
    Instrumentator::notifyAndRelease (pInstrumentations);

    if (bNotifyDSPro) {
        _pDSPro->getCallbackHandler()->positionUpdated (fLatitude, fLongitude, fAltitude, pszPublisherNodeId);
    }

    return 0;
}

int Controller::wholeMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                     const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    if (pszSenderNodeId == NULL || pszPublisherNodeId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    if (_nodeId == pszPublisherNodeId) {
        return 0;
    }

    addPeerToTopology (_pDSPro, uiAdaptorId, pszSenderNodeId, pszPublisherNodeId);
    PeerNodeContext *pNodeContext = _pDSPro->getNodeContextManager()->getPeerNodeContext (pszPublisherNodeId);

    logTopology ("Controller::wholeMessageArrived", Logger::L_Info, "%s --> %s\n",
                 pszPublisherNodeId, pszSenderNodeId);

    char *pszNodeToForwardTo = NULL;
    const int iNodeCtxtMgrRC = _pDSPro->getNodeContextManager()->wholeMessageArrived (pBuf, ui32Len, pszPublisherNodeId, &pszNodeToForwardTo);
    if (iNodeCtxtMgrRC < 0) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        checkAndLogMsg ("Controller::wholeMsgArrived", ctrlDeserializationError,
                        "whole", pszSenderNodeId);
        return -3;
    }
    if (iNodeCtxtMgrRC == 1 && pszNodeToForwardTo == NULL) {
        // Old whole message
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        return 0;
    }

    // if (_bContextForwardingEnabled && (pszNodeToForwardTo != NULL) && (strcmp (pszNodeToForwardTo, _nodeId) != 0)) {
    if (_bContextForwardingEnabled) {
        // At least a version number has incremented - the whole message was useful,
        // therefore it needs to be forwarded
        int iSendRC = 0;
        if (iNodeCtxtMgrRC == 1) {
            // if the whole message is old, only send it to the node that requested it
            TargetPtr targetPtrs[2];
            targetPtrs[0] = _pDSPro->getTopology()->getNextHopAsTarget (pszNodeToForwardTo);
            targetPtrs[1] = NULL;
            if (targetPtrs[0] != NULL) {
                iSendRC = _pDSPro->getCommAdaptorManager()->sendWholeMessage (pBuf, ui32Len, pszPublisherNodeId, targetPtrs);
                delete targetPtrs[0];
            }
        }
        else {
            // otherwise, disseminate it
            Targets **ppTargets = _pDSPro->getTopology()->getForwardingTargets (_nodeId, pszSenderNodeId);
            if (ppTargets != NULL && ppTargets[0] != NULL) {
                iSendRC = _pDSPro->getCommAdaptorManager()->sendWholeMessage (pBuf, ui32Len, pszPublisherNodeId, ppTargets);
            }
            Targets::deallocateTargets (ppTargets);
        }

        if (iSendRC < 0) {
            checkAndLogMsg ("Controller::wholeMessageArrived", Logger::L_Warning,
                            "failed to forward update message from %s and originated from %s. Return code %d.\n",
                            pszSenderNodeId, pszPublisherNodeId, iSendRC);
        }
        else {
            checkAndLogMsg ("Controller::wholeMessageArrived", Logger::L_Info,
                            "update message from %s and originated from %s was forwarded.\n",
                            pszSenderNodeId, pszPublisherNodeId);
        }
    }

    if (iNodeCtxtMgrRC == 1) {
        // Old whole message
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList ();
        return 0;
    }

    NodePath *pNodePath = pNodeContext->getPath();
    if ((pNodePath != NULL) && (pNodePath->getPathLength() > 0)) {
        _pDSPro->getCallbackHandler()->pathRegistered (pNodeContext->getPath(),
                                                       pNodeContext->getNodeId(),
                                                       pNodeContext->getTeamId(),
                                                       pNodeContext->getMissionId());
    }

    if (!_bPreStagingEnabled) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        return 0;
    }

    // Get the messages to be matched against the new node context
    MetadataList *pMetadataList = _matchmakingHelper.getMetadataToMatchmake (pNodeContext, pszSenderNodeId,
                                                                             _pDSPro->isTopologyExchangedEnabled());
    if (pMetadataList == NULL || pMetadataList->getFirst() == NULL) {
        _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
        checkAndLogMsg ("Controller::wholeMsgArrived", Logger::L_Info,
                        "Metadata list for peer %s contains 0 elements.\n", pszPublisherNodeId);
        return 0;
    }

    // Call push mode
    _m.lock (1006);
    PtrLList<MatchmakingIntrumentation> *pInstrumentations = _pInfoPush->nodeContextChanged (pMetadataList, pNodeContext);
    _m.unlock (1006);

    _pDSPro->getNodeContextManager()->releasePeerNodeContextList();
    MatchmakingHelper::deallocateMetadataToMatchmake (pMetadataList);
    pMetadataList = NULL;

    // Notify Instrumentator and release memory
    Instrumentator::notifyAndRelease (pInstrumentations);

    return 0;
}

void Controller::newPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId,
                          const char *pszPeerRemoteAddress, const char *pszIncomingInterface)
{
    if (pAdaptorProperties == NULL || pszDstPeerId == NULL || pszIncomingInterface == NULL) {
        return;
    }

    checkAndLogMsg ("Controller::newPeer", Logger::L_Info,
                    "New peer %s (%s) from interface %s managed by adaptor of type %d.\n",
                    pszDstPeerId, pszPeerRemoteAddress, pszIncomingInterface, pAdaptorProperties->uiAdaptorType);

    if (pAdaptorProperties->uiAdaptorType == UNKNOWN) {
        return;
    }
    int rc = _pDSPro->getTopology()->addLink (pszDstPeerId, pszIncomingInterface,
                                             pAdaptorProperties->uiAdaptorId, pAdaptorProperties->uiAdaptorType);
    if (rc < 0) {
        checkAndLogMsg ("Controller::newPeer", Logger::L_SevereError,
                        "Error when adding link in topology for peer %s from interface %s.\n",
                        pszDstPeerId, pszIncomingInterface);
    }
    else {
        checkAndLogMsg ("Controller::newPeer", Logger::L_SevereError,
                        "Adding link in topology for peer %s from interface %s.\n",
                        pszDstPeerId, pszIncomingInterface);
    }

    _pDSPro->newPeer (pszDstPeerId);
}

void Controller::deadPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId)
{
    if (pAdaptorProperties == NULL || pszDstPeerId == NULL) {
        return;
    }

    int rc = _pDSPro->getTopology()->removeLink (pszDstPeerId, pAdaptorProperties->uiAdaptorId);
    if (rc < 0) {
        checkAndLogMsg ("Controller::deadPeer", Logger::L_SevereError,
                        "Error when removing link from topology for peer %s.\n",
                        pszDstPeerId);
    }
    else {
        checkAndLogMsg ("Controller::deadPeer", Logger::L_SevereError,
                        "Removing link from topology for peer %s.\n",
                        pszDstPeerId);
    }

    _pDSPro->deadPeer (pszDstPeerId);
}

void Controller::newLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId,
                                const char *pszPeerRemoteAddress, const char *pszIncomingInterface)
{
    if (pszDstPeerId == NULL || pszIncomingInterface == NULL || pAdaptorProperties == NULL) {
        return;
    }

    checkAndLogMsg ("Controller::newLinkToPeer", Logger::L_Info,
                    "New link to peer %s (%s) from interface %s managed by adaptor of type %d.\n",
                    pszDstPeerId, pszPeerRemoteAddress, pszIncomingInterface, pAdaptorProperties->uiAdaptorType);

    if (pAdaptorProperties->uiAdaptorType == UNKNOWN) {
        return;
    }
    int rc = _pDSPro->getTopology()->addLink (pszDstPeerId, pszIncomingInterface,
                                              pAdaptorProperties->uiAdaptorId,
                                              pAdaptorProperties->uiAdaptorType);
    if (rc < 0) {
        checkAndLogMsg ("Controller::newLinkToPeer", Logger::L_SevereError,
                        "Error when adding link in topology for peer %s from interface %s.\n",
                        pszDstPeerId, pszIncomingInterface);
    }
    else {
        checkAndLogMsg ("Controller::newLinkToPeer", Logger::L_SevereError,
                        "Adding link in topology for peer %s from interface %s.\n",
                        pszDstPeerId, pszIncomingInterface);
    }

    _pDSPro->newPeer (pszDstPeerId);
}

void Controller::droppedLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId,
                                    const char *pszPeerRemoteAddress)
{
    if (pszDstPeerId == NULL || pAdaptorProperties == NULL) {
        return;
    }

    // FIX THIS! I should not remove the node from the topology, only the link
    //           through the interface!
    int rc = _pDSPro->getTopology()->removeLink (pszDstPeerId, pszPeerRemoteAddress, pAdaptorProperties->uiAdaptorId);
    if (rc < 0) {
        checkAndLogMsg ("Controller::droppedLinkToPeer", Logger::L_SevereError,
                        "Error when removing link from topology for peer %s.\n",
                        pszDstPeerId);
    }
    else {
        checkAndLogMsg ("Controller::droppedLinkToPeer", Logger::L_SevereError,
                        "Removing link %s from topology for peer %s.\n",
                        pszPeerRemoteAddress, pszDstPeerId);
    }

    // _pDSPro->deadPeer (pszDstPeerId);
}

char ** toNullTerminatedList (const char *pszString)
{
    char **ppszList = (char **) calloc (2, sizeof (char*));
    if (ppszList != NULL) {
        ppszList[0] = (char*) pszString;
    }
    return ppszList;
}

