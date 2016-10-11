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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "DSProImpl.h"

#include "ApplicationQueryController.h"
#include "C45AVList.h"
#include "ChunkQueryController.h"
#include "Classification.h"
#include "Controller.h"
#include "DataStore.h"
#include "Defs.h"
#include "DisServiceAdaptor.h"
#include "DisServiceQueryController.h"
#include "DSPro.h"
#include "DSProQueryController.h"
#include "InformationPushPolicy.h"
#include "InformationStore.h"
#include "LocalNodeContext.h"
#include "MatchMakingPolicies.h"
#include "MetadataConfiguration.h"
#include "MetaData.h"
#include "NodeContextManager.h"
#include "NodePath.h"
#include "PositionUpdater.h"
#include "Searches.h"
#include "Scheduler.h"
#include "TransmissionHistoryInterface.h"
#include "WaypointMessageHelper.h"

#include "DSSFLib.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "StrClass.h"

#define initErr(pszMethodName, component, rc) if (pLogger) pLogger->logMsg (pszMethodName, Logger::L_SevereError, "%s could not be instantiated or configured. Returned error code: %d.\n", component, rc)

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    const char * configureSessionId (ConfigManager *pCfgMgr)
    {
        const char *pszSessionId = pCfgMgr->getValue ("aci.dspro.sessionKey");
        if ((pszSessionId == NULL) || (strlen (pszSessionId) <= 0)) {
            pszSessionId = pCfgMgr->getValue ("aci.disService.sessionKey");
        }
        else {
            pCfgMgr->setValue ("aci.disService.sessionKey", pszSessionId);
        }
        if ((pszSessionId != NULL) && (strlen (pszSessionId) <= 0)) {
            pszSessionId = NULL;
        }
        return pszSessionId;
    }

    InformationPushPolicy * instantiateInformationPushPolicy (ConfigManager *pCfgMgr, InformationStore *pInfoStore)
    {
        if (pCfgMgr->hasValue ("aci.dspro.replication.manager.mode")) {
            const char *pszType = pCfgMgr->getValue ("aci.dspro.replication.manager.mode");
            return InformationPushPolicy::getPolicy (pszType, pInfoStore);
        }
        return new ReactivePush (pInfoStore);
    }

    Topology * instantiateTopology (ConfigManager *pCfgMgr, const String nodeId, CommAdaptorManager *pAdptrMgr, NodeContextManager *pNodeContextMgr)
    {
        bool bUseStaticTopology = pCfgMgr->getValueAsBool (
            StaticTopology::USE_STATIC_TOPOLOGY_PROPERTY,
            StaticTopology::USE_STATIC_TOPOLOGY_DEFAULT);
        Topology *pTopology = (bUseStaticTopology ? new StaticTopology (nodeId) : new Topology (nodeId));
        if (pTopology != NULL) {
            if (bUseStaticTopology) {
                static_cast<StaticTopology *>(pTopology)->configure (pAdptrMgr, pNodeContextMgr, pCfgMgr);
            }
            else {
                pTopology->configure (pAdptrMgr, pNodeContextMgr);
            }
        }
        return pTopology;
    }

    int loadPath (ConfigManager *pCfgMgr, DSProImpl *pDSPro)
    {
        NodePath *pPath = new NodePath ();
        if (pPath == NULL) {
            return -1;
        }
        pPath->read (pCfgMgr, 0);
        if (pPath->getPathLength () > 0 &&
            pPath->getPathID () != NULL) {
            pDSPro->registerPath (pPath);
            if (pCfgMgr->hasValue ("aci.dspro.nodePath.current") &&
                pCfgMgr->getValueAsBool ("aci.dspro.nodePath.current")) {
                pDSPro->setCurrentPath (pPath->getPathID ());
                if (pCfgMgr->hasValue ("aci.dspro.nodePath.setFirstWaypoint") &&
                    pCfgMgr->getValueAsBool ("aci.dspro.nodePath.setFirstWaypoint")) {
                    pDSPro->setCurrentPosition (pPath->getLatitude (0), pPath->getLongitude (0),
                        pPath->getAltitude (0), pPath->getLocation (0),
                        pPath->getNote (0));
                }
            }
            pPath->display();
        }
        else {
            delete pPath;
            pPath = NULL;
        }
        return 0;
    }
}

DSProImpl::DSProImpl (const char *pszNodeId, const char *pszVersion)
    : _bEnableLoopbackNotifications (true),
      _bEnableTopologyExchange (true),
      _pMetadataConf (NULL),
      _pLocalNodeContext (NULL),
      _pDataStore (NULL),
      _pInfoStore (NULL),
      _pController (NULL),
      _pPublisher (NULL),
      _pNodeContextMgr (NULL),
      _pTopology (NULL),
      _pScheduler (NULL),
      _pPositionUpdater (NULL),    
      _nodeId (pszNodeId),
      _version (pszVersion),
      _m (MutexId::DSPro_m, LOG_MUTEX),
      _adaptMgr (pszNodeId)
{
}

DSProImpl::~DSProImpl (void)
{
    if ((_pPositionUpdater != NULL) && (_pPositionUpdater->isRunning())) {
        _pPositionUpdater->requestTerminationAndWait ();
    }
    if ((_pScheduler != NULL) && (_pScheduler->isRunning())) {
        _pScheduler->requestTerminationAndWait();
    }
    delete _pPositionUpdater;
    _pPositionUpdater = NULL;
    delete _pPositionUpdater;
    _pPositionUpdater = NULL;
}

int DSProImpl::init (ConfigManager *pCfgMgr, MetadataConfiguration *pMetadataConf)
{
    const char *pszMethodName = "DSProImpl::init";
    String tag = "$Name: BeforeNodeCtxtSerializationRefactoring-20160927 $";

    if ((pCfgMgr == NULL) || (pMetadataConf == NULL)) {
        return -1;
    }
    _pMetadataConf = pMetadataConf;
    if (_nodeId.length() <= 0) {
        return -2;
    }
    if (_cbackHandler.init (pCfgMgr) < 0) {
        return -3;
    }

    int rc = -1;
    _bEnableTopologyExchange = pCfgMgr->getValueAsBool (DSPro::ENABLE_TOPOPLOGY_EXCHANGE, false);
    const String sessionId (configureSessionId (pCfgMgr));

    // Instantiate Local Node Context
    _pLocalNodeContext = LocalNodeContext::getInstance (_nodeId, pCfgMgr, _pMetadataConf);
    if ((_pLocalNodeContext == NULL) || ((rc = _pLocalNodeContext->configure (pCfgMgr)) < 0)) {
        initErr (pszMethodName, "LocalNodeContext", rc);
    }

    // Instantiate Data Store (to store the messages in binary format)
    _pDataStore = DataStore::getDataStore (pCfgMgr, sessionId);
    if (_pDataStore == NULL) {
        initErr (pszMethodName, "DataStore", rc);
        return -4;
    }

    // Instantiate Information Store (to store metadata attributes)
    _pInfoStore = new InformationStore (_pDataStore, DisServiceAdaptor::DSPRO_GROUP_NAME);
    if ((_pInfoStore == NULL) || ((rc = _pInfoStore->init (_pMetadataConf)) < 0)) {
        initErr (pszMethodName, "InformationStore", rc);
        return -5;
    }

    // Instantiate Publisher
    _pPublisher = new Publisher (_nodeId, DisServiceAdaptor::DSPRO_GROUP_NAME, _pInfoStore, _pDataStore);
    if ((_pPublisher == NULL) || ((rc = _pPublisher->init (pCfgMgr)) < 0)) {
        initErr (pszMethodName, "Publisher", rc);
        return -6;
    }

    // Instantiate Node Context Manager
    _pNodeContextMgr = new NodeContextManager (_nodeId, _pLocalNodeContext);
    if (_pNodeContextMgr == NULL) {
        initErr (pszMethodName, "NodeContextManager", rc);
        return -7;
    }

    // Instantiate Topology
    _pTopology = instantiateTopology (pCfgMgr, _nodeId, &_adaptMgr, _pNodeContextMgr);
    if (_pTopology == NULL) {
        initErr (pszMethodName, "Topology", rc);
        return -8;
    }

    if ((rc = _pNodeContextMgr->configure (&_adaptMgr, _pTopology, _pMetadataConf)) < 0) {
        initErr (pszMethodName, "NodeContextMgr", rc);
        return -9;
    }

    // Instantiate Scheduler
    _pScheduler = Scheduler::getScheduler (pCfgMgr, this, &_adaptMgr, _pDataStore, _pNodeContextMgr, _pInfoStore, _pTopology);
    if (_pScheduler == NULL) {
        initErr (pszMethodName, "Scheduler", rc);
        return -10;
    }

    InformationPushPolicy *pInfoPushPolicy = instantiateInformationPushPolicy (pCfgMgr, _pInfoStore);
    if (pInfoPushPolicy == NULL) {
        initErr (pszMethodName, "InformationPushPolicy", rc);
        return -11;
    }

    _pController = new Controller (this, _pLocalNodeContext, _pScheduler, _pInfoStore, _pTopology, TransmissionHistoryInterface::getTransmissionHistory());
    if ((_pController == NULL) || ((rc = _pController->init (pCfgMgr, pInfoPushPolicy, _pScheduler)) < 0)) {
        initErr (pszMethodName, "Controller", rc);
        return -12;
    }

    if ((rc = _adaptMgr.init (pCfgMgr, sessionId, _pController, _pDataStore->getPropertyStore())) < 0) {
        return -13;
    }

    _pPositionUpdater = new PositionUpdater (_pNodeContextMgr, this);
    if (_pPositionUpdater == NULL) {
        return -14;
    }

    // Load query controllers
    QueryController *pQueryCtrlr = new DSProQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == NULL || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "DSProQueryController", rc);
        return -15;
    }
    _controllers.prepend (pQueryCtrlr);

    pQueryCtrlr = new ApplicationQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == NULL || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "ApplicationQueryController", rc);
        return -16;
    }
    _controllers.prepend (pQueryCtrlr);

    pQueryCtrlr = new DisServiceQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == NULL || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "DisServiceQueryController", rc);
        return -17;
    }
    _controllers.prepend (pQueryCtrlr);

    pQueryCtrlr = new ChunkQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == NULL || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "ChunkQueryController", rc);
        return -18;
    }
    _controllers.prepend (pQueryCtrlr);

    // Start threads
    rc = _adaptMgr.startAdaptors();
    if (rc < 0) {
        return -19;
    }
    _pPositionUpdater->start();

    // Load path if it is configured
    if (loadPath (pCfgMgr, this) < 0) {
        return -20;
    }

    return 0;
}

CallbackHandler * DSProImpl::getCallbackHandler (void)
{
    return &_cbackHandler;
}

CommAdaptorManager * DSProImpl::getCommAdaptorManager (void)
{
    return &_adaptMgr;
}

DataStore * DSProImpl::getDataStore (void)
{
    return _pDataStore;
}

InformationStore * DSProImpl::getInformationStore (void)
{
    return _pInfoStore;
}

MetadataConfiguration * DSProImpl::getMetadataConf (void)
{
    return _pMetadataConf;
}

NodeContextManager * DSProImpl::getNodeContextManager (void)
{
    return _pNodeContextMgr;
}

ThreadSafePublisher * DSProImpl::getPublisher (void)
{
    return _pPublisher;
}

Topology * DSProImpl::getTopology (void)
{
    return _pTopology;
}

int DSProImpl::addAnnotation (const char *pszGroupName, const char *pszObjectId,
                              const char *pszInstanceId, const char *pszAnnotationTargetObjId,
                              MetaData *pMetadata, const void *pData, uint32 ui32DataLen,
                              int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSProImpl::addAnnotation";
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not create metadata.\n");
        return -1;
    }

    String annotationTargetObjId;
    pMetadata->getFieldValue (MetadataInterface::ANNOTATION_TARGET_OBJ_ID, annotationTargetObjId);
    if (annotationTargetObjId.length() > 0) {
        if (annotationTargetObjId != pszAnnotationTargetObjId) {
            return -2;
        }
    }
    else if (pMetadata->setFieldValue (MetadataInterface::ANNOTATION_TARGET_OBJ_ID, pszAnnotationTargetObjId) < 0) {
        return -3;
    }

    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);

    _m.lock (2016);
    char *pszDataId = NULL;
    if ((pData != NULL) && (ui32DataLen > 0)) {
        int rc = _pPublisher->chunkAndAddData (pub, NULL, NULL, 0U, pData, ui32DataLen, NULL,
                                               &pszDataId, false); // annotation can't be fragmented
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
            _m.unlock (2016);
            return -3;
        }
        assert (pszDataId != NULL);
    }

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated
    
    pub.pszReferredObjectId = pszDataId != NULL ? pszDataId : MetadataInterface::NO_REFERRED_OBJECT;
    
    int rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, true);
    _m.unlock (2016);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s "
                    "referring to data %s.\n", sMetadataId.c_str(), (pszDataId == NULL ? "NULL" : pszDataId));

    if (rc == 0 && (rc = _pController->metadataPush (sMetadataId, pMetadata)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not push metadata. Returned code %d\n", rc);
    }
    if (_bEnableLoopbackNotifications) {
        const char *pszMsgIds[2] = { sMetadataId.c_str(), NULL };
        _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
    }

    if (pszDataId != NULL) {
        free (pszDataId);
    }
    if (ppszId != NULL) {
        *ppszId = sMetadataId.r_str();
    }

    return (rc < 0 ? -4 : 0);
    
}

int DSProImpl::addMessage (const char *pszGroupName, const char *pszObjectId,
                           const char *pszInstanceId, MetaData *pMetadata,
                           const void *pData, uint32 ui32DataLen,
                           int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSProImpl::addMessage";
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not create metadata.\n");
        return -1;
    }

    String mimeType;
    pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, mimeType);

    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);

    // setAndAddMetadata modifies pMetadata - no problem here, since pMetadata
    // is not used anymore, in fact, it is even deallocated
    _m.lock (2012);
    char *pszDataId = NULL;
    if ((pData != NULL) && (ui32DataLen > 0)) {
        int rc = _pPublisher->chunkAndAddData (pub, NULL, NULL, 0U, pData, ui32DataLen, mimeType,
                                               &pszDataId, true);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
            _m.unlock (2012);
            return -3;
        }
        assert (pszDataId != NULL);
    }

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated
    
    pub.pszReferredObjectId = pszDataId != NULL ? pszDataId : MetadataInterface::NO_REFERRED_OBJECT;
    int rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, true);

    _m.unlock (2012);
    if (pszDataId != NULL) {
        free (pszDataId);
    }

    if (rc == 0 && (rc = _pController->metadataPush (sMetadataId, pMetadata)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not push metadata. Returned code %d\n", rc);
    }
    if (_bEnableLoopbackNotifications) {
        const char *pszMsgIds[2] = { sMetadataId.c_str(), NULL };
        _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s "
                    "referring to data %s.\n", sMetadataId.c_str(), (ppszId == NULL ? "NULL" : *ppszId));

    if (ppszId != NULL) {
        *ppszId = sMetadataId.r_str();
    }
    return (rc < 0 ? -4 : 0);
}

int DSProImpl::chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                   const char *pszInstanceId, MetaData *pMetadata,
                                   const void *pData, uint32 ui32DataLen,
                                   const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSProImpl::chunkAndAddMessage";
    if (ppszId != NULL) {
        *ppszId = NULL;
    }
    if (pszGroupName == NULL || pData == NULL || ui32DataLen == 0U || pszDataMimeType == NULL) {
        return -1;
    }
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not create metadata.\n");
        
        return -2;
    }

    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);

    // setAndAddMetadata modifies pMetadata - no problem here, since pMetadata
    // is not used anymore, in fact, it is even deallocated
    _m.lock (2014);
    char *pszDataId = NULL;

    int rc = _pPublisher->chunkAndAddData (pub, NULL, NULL, 0U, pData, ui32DataLen,
                                           pszDataMimeType, &pszDataId, false);

    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
        _m.unlock (2014);
        return -3;
    }
    assert (pszDataId != NULL);

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated
    pub.pszReferredObjectId = pszDataId;
    rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, true);

    _m.unlock (2014);

    if (rc == 0 && (rc = _pController->metadataPush (sMetadataId, pMetadata)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not push metadata. Returned code %d\n", rc);
    }
    if (_bEnableLoopbackNotifications) {
        const char *pszMsgIds[2] = { sMetadataId.c_str(), NULL };
        _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
    }
    if (pszDataId != NULL) {
        free (pszDataId);
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s referring to data %s.\n",
                    sMetadataId.c_str(), (ppszId == NULL ? "NULL" : *ppszId));
    if (ppszId != NULL) {
        *ppszId = sMetadataId.r_str();
    }

    return (rc < 0 ? -4 : 0);
}

int DSProImpl::addAnnotationNoPrestage (const char *pszGroupName, const char *pszObjectId,
                                        const char *pszInstanceId, MetadataInterface *pMetadata,
                                        const char *pszReferredObject, int64 i64ExpirationTime,
                                        char **ppszId)
{
    if ((pszGroupName == NULL) || (pMetadata == NULL) || (ppszId == NULL)) {
        return -1;
    }

    _m.lock (2018);
    String msgId;
    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);
    pub.pszReferredObjectId = pszReferredObject;
    int rc = _pPublisher->setAndAddMetadata (pub, pMetadata, msgId, false);
    _m.unlock (2018);
    *ppszId = msgId.r_str();
    return rc;
}

int DSProImpl::addData (const char *pszGroupName, const char *pszObjectId,
                        const char *pszInstanceId, const char *pszAnnotatedObjMsgId,
                        const char *pszAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                        const void *pData, uint32 ui32DataLen,
                        const char *pszDataMimeType, int64 i64ExpirationTime,
                        char **ppszId)
{
    if ((pszGroupName == NULL) || (pData == NULL) || (ui32DataLen == 0U) || (ppszId == NULL)) {
        return -1;
    }
    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);
    _m.lock (2018);
    int rc = _pPublisher->chunkAndAddData (pub, pszAnnotatedObjMsgId, pszAnnotationMetadata,
                                           ui32AnnotationMetdataLen, pData, ui32DataLen,
                                           pszDataMimeType, ppszId, true);
    _m.unlock (2018);
    return rc;
}

int DSProImpl::setAndAddMetadata (Publisher::PublicationInfo &pubInfo, MetadataInterface *pMetadata,
                                  String &msgId, bool bStoreInInfoStore)
{
    _m.lock (2015);
    int rc = _pPublisher->setAndAddMetadata (pubInfo, pMetadata, msgId, bStoreInInfoStore);
    _m.unlock (2015);
    return rc;
}

int DSProImpl::addPeer (AdaptorType adaptorType, const char *pszNetworkInterface,
                        const char *pszRemoteAddress, uint16 ui16Port)
{
    // TODO: change _adaptMgr's code to also take and use pszNetworkInterface
    if (/*pszNetworkInterface == NULL ||*/ pszRemoteAddress == NULL) {
        return -1;
    }
    if (adaptorType == DISSERVICE) {
        return -2;
    }

    _m.lock (2006);
    int rc = _adaptMgr.connectToPeer (adaptorType, pszRemoteAddress, ui16Port);
    _m.unlock (2006);
    if (rc < 0) {
        checkAndLogMsg ("DSProImpl::addPeer", Logger::L_Warning, "_adaptMgr.connectToPeer "
                        "returned an error. Return code: %d", rc);
    }
    return 0;
}

int DSProImpl::addRequestedMessageToUserRequests (const char *pszId, const char *pszQueryId)
{
    if (pszId == NULL) {
        return -1;
    }
    _userReqs.put (pszId, pszQueryId);
    return 0;
}

int DSProImpl::addUserId (const char *pszUserName)
{
    if (pszUserName == NULL) {
        return -1;
    }
    _m.lock (2001);
    if (_pNodeContextMgr == NULL) {
        _m.unlock (2001);
        return -2;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext ();
    if (pLocalNodeContext == NULL) {
        _m.unlock (2001);
        return -3;
    }
    int rc = pLocalNodeContext->addUserId (pszUserName);
    _pNodeContextMgr->releaseLocalNodeContext ();

    if (rc == 0) {
        checkAndLogMsg ("DSProImpl::addUserId", Logger::L_Info, "added user id %s.\n",
                        pszUserName);
    }
    else {
        checkAndLogMsg ("DSProImpl::addUserId", Logger::L_MildError, "could not "
                        "add user id %s. Return code %d.\n", pszUserName, rc);
    }

    _m.unlock (2001);
    return rc;
}

void DSProImpl::asynchronouslyNotifyMatchingMetadata (const char *pszQueryId, const char **ppszMsgIds)
{
    if (pszQueryId == NULL && ppszMsgIds != NULL && ppszMsgIds[0] != NULL) {
        return;
    }

    _pPositionUpdater->addMetadataToNotify (pszQueryId, ppszMsgIds);
}

void DSProImpl::asynchronouslyNotifyMatchingSearch (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen)
{
    if (pszQueryId == NULL && pReply != NULL && ui16ReplyLen == 0) {
        return;
    }

    _pPositionUpdater->addSearchToNotify (pszQueryId, pReply, ui16ReplyLen);
}

int DSProImpl::getData (const char *pszId, const char *pszCallbackParameter,
                        void **ppData, uint32 &ui32DataLen, bool &bHasMoreChunks)
{
    const char *pszMethodName = "DSProImpl::getData";
    bHasMoreChunks = false;
    ui32DataLen = 0;

    if (pszId == NULL || ppData == NULL || strcmp (pszId, MetaData::NO_REFERRED_OBJECT) == 0) {
        return -1;
    }

    *ppData = NULL;
    const String grpName (extractGroupFromKey (pszId));
    if (grpName.length() <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Could not extract group from message id %s.\n", pszId);
        return -2;
    }

    const bool bIsDSProGrp = DisServiceAdaptor::checkGroupName (grpName, DisServiceAdaptor::DSPRO_GROUP_NAME);
    const bool bIsLargeObject = isOnDemandGroupName (grpName);
    if (!bIsDSProGrp) {

    }

    if (_pDataStore->getData (pszId, ppData, ui32DataLen) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Could not read the data from the datacache.\n");
        return -3;
    }

    _m.lock (2008);
    if ((*ppData == NULL) || (ui32DataLen == 0)) {
        // the data has not arrived yet, register the data request
        if (_userReqs.put (pszId, pszCallbackParameter) == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "data message "
                            "%s not found. Added it to _userReqs\n", pszId);
        }
    }

    MetadataList *pMetadataList = _pInfoStore->getMetadataForData (pszId);
    if (pMetadataList == NULL) {
        checkAndLogMsg ("DSPro::getData", Logger::L_Warning, "metadata not "
            "found for data message with id %s.\n", pszId);
    }
    else {
        for (MetadataInterface *pMetadata = pMetadataList->getFirst();
            pMetadata != NULL; pMetadata = pMetadataList->getNext()) {

            char *pszMetadataId = NULL;
            if (0 == pMetadata->getFieldValue (pMetadata->MESSAGE_ID, &pszMetadataId) &&
                pszMetadataId != NULL) {
                updateLearning (pszMetadataId, Classification::Useful);
                free (pszMetadataId);
            }
            delete (pMetadata);
        }
        delete pMetadataList;
        pMetadataList = NULL;
    }

    if (*ppData == NULL) {
        _m.unlock (2008);
        return 1;
    }

    // Copy the returned object
    void *pDataCpy = NULL;
    if (bIsLargeObject) {
        pDataCpy = *ppData;
    }
    else {
        uint32 ui32NewLen = 0;
        MessageHeaders::MsgType type;
        pDataCpy = MessageHeaders::removeDSProHeader (*ppData, ui32DataLen, ui32NewLen, type);
        if (pDataCpy == NULL) {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            free (*ppData);
            _m.unlock (2008);
            return -5;
        }
        ui32DataLen = ui32NewLen;
        free (*ppData);
    }

    *ppData = pDataCpy;
    _m.unlock (2008);

    uint8 ui8NChunks, ui8TotChunks;
    ui8NChunks = ui8TotChunks = 0;
    if (_pDataStore->getNumberOfReceivedChunks (pszId, ui8NChunks, ui8TotChunks) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "error when trying "
                        "to check the number of received chunks for message %s\n", pszId);
        return -6;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "getNumberOfReceivedChunks() "
                    "says that %d chunks out of %d have been received\n", ui8NChunks, ui8TotChunks);

    bHasMoreChunks = (ui8TotChunks > ui8NChunks);
    return 0;
}

char ** DSProImpl::getMatchingMetadataAsXML (AVList *pAVQueryList, int64 i64BeginArrivalTimestamp, int64 i64EndArrivalTimestamp)
{
    MetadataList *pMetadataList = _pInfoStore->getAllMetadata (pAVQueryList, i64BeginArrivalTimestamp, i64EndArrivalTimestamp);
    if (pMetadataList == NULL || pMetadataList->getFirst () == NULL) {
        return NULL;
    }
    int iCount = pMetadataList->getCount ();
    if (iCount <= 0) {
        return NULL;
    }
    char **ppszMetadataAsXML = static_cast<char **>(calloc (iCount + 1, sizeof (char *)));
    if (ppszMetadataAsXML != NULL) {
        // Convert the fields to XML
        MetadataInterface *pCurr, *pNext;
        pNext = pMetadataList->getFirst();
        for (unsigned int i = 0; ((pCurr = pNext) != NULL) && (i < ((unsigned int) iCount)); i++) {
            pNext = pMetadataList->getNext ();
            ppszMetadataAsXML[i] = _pMetadataConf->convertMetadataToXML (pCurr);
            delete pMetadataList->remove (pCurr);
        }
    }

    return ppszMetadataAsXML;
}

const char * DSProImpl::getNodeId (void) const
{
    return _nodeId.c_str();
}

char ** DSProImpl::getPeerList (void)
{
    return _pNodeContextMgr->getPeerList (true);
}

const char * DSProImpl::getVersion (void) const
{
    return _version.c_str();
}

bool DSProImpl::isTopologyExchangedEnabled (void)
{
    return _bEnableTopologyExchange;
}

NOMADSUtil::String DSProImpl::getSessionId (void) const
{
    return _adaptMgr.getSessionId();
}

int DSProImpl::notUseful (const char *pszMessageID)
{
    if (pszMessageID == NULL) {
        return -1;
    }
    _m.lock (2009);
    int rc = updateLearning (pszMessageID, Classification::NotUseful);
    _m.unlock (2009);
    return (rc < 0 ? -2 : 0);
}

void DSProImpl::sendWaypointMessage (const void *pBuf, uint32 ui32BufLen)
{
    if (pBuf == NULL || ui32BufLen == 0) {
        return;
    }

    PtrLList<String> *pNeighborList = _pTopology->getNeighbors();
    if (pNeighborList == NULL) {
        return;
    }
    if (pNeighborList->getFirst () == NULL) {
        delete pNeighborList;
        return;
    }

    NodeIdSet nodeIdSet;
    PreviousMessageIds previousMessageIds;
    String *pszNextPeerId = pNeighborList->getFirst ();
    for (String *pszCurrPeerId; (pszCurrPeerId = pszNextPeerId) != NULL;) {
        pszNextPeerId = pNeighborList->getNext ();
        previousMessageIds.add (pszCurrPeerId->c_str (), _pScheduler->getLatestMessageReplicatedToPeer (pszCurrPeerId->c_str ()));
        nodeIdSet.add (pszCurrPeerId->c_str ());
        delete pNeighborList->remove (pszCurrPeerId);
    }
    delete pNeighborList;

    uint32 ui32TotalLen = 0;
    void *pData = WaypointMessageHelper::writeWaypointMessageForTarget (previousMessageIds, pBuf, ui32BufLen, ui32TotalLen);
    Targets **ppTargets = _pTopology->getNextHopsAsTarget (nodeIdSet);
    if ((ppTargets != NULL) && (ppTargets[0] != NULL)) {
        // Send the waypoint message on each available interface that reaches the recipients
        int rc = _adaptMgr.sendWaypointMessage (pData, ui32TotalLen, _nodeId, ppTargets);
        String sLatestMsgs (previousMessageIds);
        String sPeers (nodeIdSet);
        checkAndLogMsg ("DSPro::sendWaypointMessage", Logger::L_Info, "sending waypoint message "
            "to %s (%s); last message pushed to this node was %s.\n", sPeers.c_str (),
            (rc == 0 ? "succeeded" : "failed"), sLatestMsgs.c_str ());
    }
    Targets::deallocateTargets (ppTargets);
    free (pData);
}

int DSProImpl::updateLearning (const char *pszMessageId, uint8 ui8Usage)
{
    const char *pszMethodName = "DSPro::updateLearning";
    if (pszMessageId == NULL) {
        return -1;
    }

    _m.lock (2018);
    if (0 != _pInfoStore->updateUsage (pszMessageId, ui8Usage)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "Unable to update 'usage' "
                        "value for message with id = <%s> \n", pszMessageId);
        _m.unlock (2018);
        return -2;
    }

    MetadataList *pMetadataList = _pInfoStore->getMetadataForData (pszMessageId);
    if (pMetadataList != NULL) {
        MetadataInterface *pCurr, *pNext;
        pNext = pMetadataList->getFirst ();
        for (unsigned int i = 0; ((pCurr = pNext) != NULL); i++) {
            pNext = pMetadataList->getNext ();

            IHMC_C45::C45AVList *pDataset = _pMetadataConf->getMetadataAsDataset (pCurr);
            if (pDataset == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "Unable to convert the metadata "
                                "from message with id = <%s> to C45AVList class.\n", pszMessageId);
            }
            else {
                int rc = _pNodeContextMgr->updateClassifier (pDataset);
                if (rc < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_MildError, "Unable to use the metadata "
                                    "from message with id = <%s> as input for the learning algorithm.\n",
                                    pszMessageId);
                }
                delete pDataset;
            }
            delete pMetadataList->remove (pCurr);
        }
        delete pMetadataList;
    }

    _m.unlock (2018);
    return 0;
}

int DSProImpl::search (SearchProperties &searchProp, char **ppszQueryId)
{
    Targets **ppTargets = _pTopology->getNeighborsAsTargets();
    if (ppTargets != NULL) {
        // Send search
        int rc = _adaptMgr.sendSearchMessage (searchProp, ppTargets);
        Targets::deallocateTargets (ppTargets);
        if (rc < 0) {
            checkAndLogMsg ("DSProImpl::search", Logger::L_Warning, "could not send "
                            "search message. Returned code : %d\n", rc);
        }
        else {
            checkAndLogMsg ("DSProImpl::search", Logger::L_Info, "sent search message "
                            "with id <%s>\n", searchProp.pszQueryId);
        }
    }

    if (searchProp.pszQueryId == NULL) {
        searchProp.pszQueryId = SearchService::getSearchId (searchProp.pszGroupName, getNodeId(), _pDataStore->getPropertyStore());
    }

    // Store reference to the generated query Id
    *ppszQueryId = strDup (searchProp.pszQueryId);

    uint16 ui16ClientId = 0xFFFF;
    Searches::getSearches()->addSearchInfo (*ppszQueryId, searchProp.pszQueryType, getNodeId(), ui16ClientId);
    return 0;
}

int DSProImpl::sendAsynchronousRequestMessage (const char *pszId)
{
    if (pszId == NULL) {
        return -1;
    }

    _pPositionUpdater->requestMessage (pszId);
    return 0;
}

int DSProImpl::setCurrentPath (const char *pszPathID)
{
    _m.lock (2002);
    if (_pNodeContextMgr == NULL) {
        checkAndLogMsg ("DSProImpl::setCurrentPath", Logger::L_Warning, "Trying set node "
                        "context before but NodeContextManager not initialized yet.\n");
        _m.unlock (2002);
        return -1;
    }
    int rc = _pNodeContextMgr->setCurrentPath (pszPathID);

    _m.unlock (2002);
    return rc == 0 ? 0 : -2;
}

int DSProImpl::setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                                   const char *pszLocation, const char *pszNote)
{
    if (_pNodeContextMgr == NULL) {
        checkAndLogMsg ("DSProImpl::setCurrentPosition", Logger::L_Warning, "Trying set node "
                       "context before but NodeContextManager not initialized yet.\n");
        return -1;
    }
    if (_pController == NULL) {
        return -2;
    }

    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    bool bPositionChanged = pLocalNodeContext->setCurrentPosition (fLatitude, fLongitude,
                                                                   fAltitude, pszLocation, pszNote,
                                                                   getTimeInMilliseconds());
    _pNodeContextMgr->releaseLocalNodeContext ();
    if (!bPositionChanged) {
        return 0;
    }

    BufferWriter bw (1024, 128);
    int rc = _pNodeContextMgr->updatePosition (&bw);
    if (rc < 0) {
        checkAndLogMsg ("DSProImpl::setCurrentPosition", Logger::L_Warning, "\n");
        return -3;
    }

    _pPositionUpdater->positionUpdated();
    sendWaypointMessage (bw.getBuffer(), bw.getBufferLength());
    return 0;
}

int DSProImpl::registerPath (NodePath *pPath)
{
    if (pPath == NULL) {
        return -1;
    }
    if (pPath->getPathLength() <= 0) {
        checkAndLogMsg ("DSProImpl::registerPath", Logger::L_Warning,
            "trying to register path with no waypoints.\n");
        return -2;
    }
    _m.lock (2001);
    if (_pNodeContextMgr == NULL) {
        checkAndLogMsg ("DSProImpl::registerPath", Logger::L_Warning, "trying set node "
                        "context before but NodeContextManager not initialized yet.\n");
        _m.unlock (2001);
        return -1;
    }
    int rc = _pNodeContextMgr->registerPath (pPath);
    pPath->display();

    _m.unlock (2001);
    return rc;
}

int DSProImpl::removeAsynchronousRequestMessage (const char *pszId)
{
    if (pszId == NULL) {
        return -1;
    }

    _pPositionUpdater->removeMessageRequest (pszId);
    return 0;
}

int DSProImpl::requestMoreChunks (const char *pszChunkedMsgId, const char *pszCallbackParameter)
{
    const char *pszMethodName = "DSProImpl::requestMoreChunks";
    if (pszChunkedMsgId == NULL) {
        return -1;
    }

    _m.lock (2010);
    if (_userReqs.put (pszChunkedMsgId, pszCallbackParameter) == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "new chunk requested "
                        "and added it to _userReqs\n", pszChunkedMsgId);
    }

    Targets **ppTargets = _pTopology->getNeighborsAsTargets();
    if (ppTargets == NULL) {
        _m.unlock (2010);
        return 0;
    }
    if (ppTargets[0] == NULL) {
        delete ppTargets;
        _m.unlock (2010);
        return 0;
    }

    uint8 ui8TotalNumberOfChunks = 0;
    DArray<uint8> *pCachedChunkIds = _pDataStore->getCachedChunkIDs (pszChunkedMsgId, ui8TotalNumberOfChunks);
    int rc = _adaptMgr.sendChunkRequestMessage (pszChunkedMsgId, pCachedChunkIds,
                                                getNodeId(), ppTargets);
    Targets::deallocateTargets (ppTargets);
    String chunkIds ("");
    if (pCachedChunkIds != NULL) {
        for (unsigned int i = 0; i < pCachedChunkIds->size(); i++) {
            chunkIds += ((uint32) (*pCachedChunkIds)[i]);
            chunkIds += " ";
        }
        delete pCachedChunkIds;
        pCachedChunkIds = NULL;
    }

    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not request more chunks "
                        "for %s. Return code %d. The already received chunks are: <%s>\n",
                        pszChunkedMsgId, rc, chunkIds.c_str());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "requested more chunks for message %s. "
                        "The already received chunks are: <%s>\n", pszChunkedMsgId, chunkIds.c_str());
    }

    _m.unlock (2010);
    return (rc < 0 ? -2 : 0);
}

int DSProImpl::registerCommAdaptorListener (uint16, CommAdaptorListener *pListener, uint16 &ui16AssignedClientId)
{
    unsigned int uiListenerId = 0;
    int rc = _adaptMgr.registerCommAdaptorListener (pListener, uiListenerId);
    if (rc < 0) {
        return -1;
    }
    if (uiListenerId > 0xFFFF) {
        return -2;
    }
    ui16AssignedClientId = (uint16) uiListenerId;
    return rc;
}

int DSProImpl::deregisterCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener)
{
    return _adaptMgr.deregisterCommAdaptorListener (ui16ClientId);
}

int DSProImpl::dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                            const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                            const char *pszQueryId)
{
    const char *pszMethodName = "DSProImpl::dataArrived";
    String dsproQueryId;
    if (!_userReqs.contains (pszId, dsproQueryId)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s of length %u arrived. "
            "It was not requested, therefore it will not be delivered "
            "to the applications\n", pszId, ui32Len);
        return 0;
    }
    if ((pszQueryId == NULL) && (dsproQueryId.length () > 0)) {
        pszQueryId = dsproQueryId.c_str ();
    }
    return _cbackHandler.dataArrived (pszId, pszGroupName, pszObjectId,
                                      pszInstanceId, pszAnnotatedObjMsgId, pszMimeType,
                                      pBuf, ui32Len, ui8NChunks, ui8TotNChunks, pszQueryId);
}

int DSProImpl::metadataArrived (const char *pszId, const char *pszGroupName,
                                const char *pszObjectId, const char *pszInstanceId,
                                const void *pBuf, uint32 ui32Len, const char *pszReferredDataId,
                                const char *pszQueryId)
{
    const char *pszMethodName = "DSProImpl::metadataArrived";
    if (pszId == NULL || pBuf == NULL || ui32Len == 0 || pszReferredDataId == NULL) {
        return -1;
    }

    MetaData *pMetadata = _pMetadataConf->createNewMetadataFromBuffer (pBuf, ui32Len);
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, dataDeserializationError, pszId);
        return -2;
    }

    String dsproQueryId;
    if (pszQueryId == NULL) {
        if (_userReqs.contains (pszId, dsproQueryId) && (dsproQueryId.length () > 0)) {
            pszQueryId = dsproQueryId.c_str();
        }
    }

    int rc = metadataArrived (pszId, pszGroupName, pszObjectId, pszInstanceId,
                              pMetadata, pszReferredDataId, pszQueryId);
    delete pMetadata;
    pMetadata = NULL;

    return (int) rc;
}

int DSProImpl::metadataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                const char *pszInstanceId, MetaData *pMetadata, const char *pszReferredDataId,
                                const char *pszQueryId)
{
    String metaDataTarget;
    if (0 != pMetadata->getFieldValue (MetaData::TARGET_ID, metaDataTarget)) {
        metaDataTarget = NULL;
    }

    String refObjectId (pszObjectId);
    if (refObjectId.length () <= 0) {
        pMetadata->getFieldValue (MetaData::REFERRED_DATA_OBJECT_ID, refObjectId);
    }
    String refInstanceId (pszInstanceId);
    if (refInstanceId.length () <= 0) {
        pMetadata->getFieldValue (MetaData::REFERRED_DATA_INSTANCE_ID, refInstanceId);
    }

    bool bIsTarget = true;
    if (metaDataTarget.length () > 0 && _pNodeContextMgr != NULL) {
        LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
        if (pLocalNodeContext != NULL) {
            bIsTarget = MatchMakingPolicies::isNodeOrUserTarget (metaDataTarget, pLocalNodeContext);
            _pNodeContextMgr->releaseLocalNodeContext ();
        }
    }

    char *pszXMLMetadata = _pMetadataConf->convertMetadataToXML (pMetadata);
    if (pszXMLMetadata == NULL) {
        return -2;
    }

    String dsproQueryId;
    if (pszQueryId == NULL) {
        if (_userReqs.contains (pszId, dsproQueryId) && (dsproQueryId.length () > 0)) {
            pszQueryId = dsproQueryId.c_str();
        }
    }

    int rc = _cbackHandler.metadataArrived (pszId, pszGroupName, refObjectId, refInstanceId,
                                            pszXMLMetadata, pszReferredDataId, pszQueryId, bIsTarget);

    free (pszXMLMetadata);
    return rc;
}

int DSProImpl::newPeer (const char *pszNewPeerId)
{
    if (pszNewPeerId == NULL) {
        return -1;
    }
    _pPositionUpdater->topologyHasChanged();
    return _cbackHandler.newPeer (pszNewPeerId);
}

int DSProImpl::deadPeer (const char *pszDeadPeerId)
{
    if (pszDeadPeerId == NULL) {
        return -1;
    }
    _pPositionUpdater->topologyHasChanged();
    return _cbackHandler.deadPeer (pszDeadPeerId);
}

