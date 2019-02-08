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

#include "AMTDictator.h"
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
#include "MetadataConfigurationImpl.h"
#include "MetaData.h"
#include "MissionPkg.h"
#include "NetLogger.h"
#include "NodeContextManager.h"
#include "NodePath.h"
#include "PositionUpdater.h"
#include "Searches.h"
#include "Scheduler.h"
#include "WaypointMessageHelper.h"

#include "DSSFLib.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "StrClass.h"
#include "C45Utils.h"
#include "MetadataHelper.h"
#include "Voi.h"

#include "ChunkingManager.h"
#include "Json.h"

#include <memory>

#define nodeCtxMgrNotInitialized(pszMethodName) if (pLogger) pLogger->logMsg (pszMethodName, Logger::L_Warning, "Trying set node context before but NodeContextManager not initialized yet.\n");
#define initErr(pszMethodName, component, rc) if (pLogger) pLogger->logMsg (pszMethodName, Logger::L_SevereError, "%s could not be instantiated or configured. Returned error code: %d.\n", component, rc)
#define initWarn(pszMethodName, component, rc) if (pLogger) pLogger->logMsg (pszMethodName, Logger::L_Warning, "%s could not be instantiated or configured. Returned error code: %d.\n", component, rc)

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;
using namespace IHMC_MISC_MIL_STD_2525;

namespace IHMC_ACI
{
    const char * configureSessionId (ConfigManager *pCfgMgr)
    {
        const char *pszSessionId = pCfgMgr->getValue ("aci.dspro.sessionKey");
        if ((pszSessionId == nullptr) || (strlen (pszSessionId) <= 0)) {
            pszSessionId = pCfgMgr->getValue ("aci.disService.sessionKey");
        }
        else {
            pCfgMgr->setValue ("aci.disService.sessionKey", pszSessionId);
        }
        if ((pszSessionId != nullptr) && (strlen (pszSessionId) <= 0)) {
            pszSessionId = nullptr;
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
        if (pTopology != nullptr) {
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
        if (pPath == nullptr) {
            return -1;
        }
        pPath->read (pCfgMgr, 0);
        if (pPath->getPathLength() > 0 &&
            pPath->getPathId() != nullptr) {
            pDSPro->registerPath (pPath);
            if (pCfgMgr->hasValue ("aci.dspro.nodePath.current") &&
                pCfgMgr->getValueAsBool ("aci.dspro.nodePath.current")) {
                pDSPro->setCurrentPath (pPath->getPathId());
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
            pPath = nullptr;
        }
        return 0;
    }
}

DSProImpl::DSProImpl (const char *pszNodeId, const char *pszVersion)
    : _bEnableLoopbackNotifications (true),
      _bEnableTopologyExchange (true),
      _pMetadataConf (nullptr),
      _pLocalNodeContext (nullptr),
      _pDataStore (nullptr),
      _pInfoStore (nullptr),
      _pController (nullptr),
      _pPublisher (nullptr),
      _pNodeContextMgr (nullptr),
      _pTopology (nullptr),
      _pScheduler (nullptr),
      _pPositionUpdater (nullptr),
      _pVoi (nullptr),
      _pAMTDict (nullptr),
      _nodeId (pszNodeId),
      _version (pszVersion),
      _m (MutexId::DSPro_m, LOG_MUTEX),
      _adaptMgr (pszNodeId)
{
}

DSProImpl::~DSProImpl (void)
{
    if ((_pPositionUpdater != nullptr) && _pPositionUpdater->isRunning()) {
        _pPositionUpdater->requestTerminationAndWait();
    }
    if ((_pScheduler != nullptr) && _pScheduler->isRunning()) {
        _pScheduler->requestTerminationAndWait();
    }
    delete _pPositionUpdater;
    _pPositionUpdater = nullptr;
    delete _pPositionUpdater;
    _pPositionUpdater = nullptr;
    delete _pVoi;
    _pVoi = nullptr;
}

int DSProImpl::init (ConfigManager *pCfgMgr, MetadataConfigurationImpl *pMetadataConf)
{
    const char *pszMethodName = "DSProImpl::init";
    String tag = "$Name$";

    if ((pCfgMgr == nullptr) || (pMetadataConf == nullptr)) {
        return -1;
    }
    if (_chunkingConf.init (pCfgMgr) < 0) {
        return -2;
    }
    _pMetadataConf = pMetadataConf;
    if (_nodeId.length() <= 0) {
        return -3;
    }
    if (_cbackHandler.init (pCfgMgr) < 0) {
        return -4;
    }
    _pAMTDict = new AMTDictator (this);
    if (_pAMTDict == nullptr) {
        return -5;
    }

    int rc = -1;
    _bEnableTopologyExchange = pCfgMgr->getValueAsBool (DSPro::ENABLE_TOPOPLOGY_EXCHANGE, false);
    const String sessionId (configureSessionId (pCfgMgr));

    // Instantiate Local Node Context
    _pLocalNodeContext = LocalNodeContext::getInstance (_nodeId, pCfgMgr, _pMetadataConf);
    if ((_pLocalNodeContext == nullptr) || ((rc = _pLocalNodeContext->configure (pCfgMgr)) < 0)) {
        initErr (pszMethodName, "LocalNodeContext", rc);
        return -6;
    }

    // Instantiate Data Store (to store the messages in binary format)
    _pDataStore = DataStore::getDataStore (pCfgMgr, sessionId);
    if (_pDataStore == nullptr) {
        initErr (pszMethodName, "DataStore", rc);
        return -7;
    }

    // Instantiate Information Store (to store metadata attributes)
    _pInfoStore = new InformationStore (_pDataStore, DisServiceAdaptor::DSPRO_GROUP_NAME);
    if ((_pInfoStore == nullptr) || ((rc = _pInfoStore->init (_pMetadataConf)) != 0)) {
        initErr (pszMethodName, "InformationStore", rc);
        return -8;
    }

    // Instantiate Publisher
    _pPublisher = new Publisher (_nodeId, DisServiceAdaptor::DSPRO_GROUP_NAME, _pInfoStore, _pDataStore);
    if ((_pPublisher == nullptr) || ((rc = _pPublisher->init (pCfgMgr)) < 0)) {
        initErr (pszMethodName, "Publisher", rc);
        return -9;
    }

    // Instantiate Node Context Manager
    _pNodeContextMgr = new NodeContextManager (_nodeId, _pLocalNodeContext);
    if (_pNodeContextMgr == nullptr) {
        initErr (pszMethodName, "NodeContextManager", rc);
        return -10;
    }

    // Instantiate Topology
    _pTopology = instantiateTopology (pCfgMgr, _nodeId, &_adaptMgr, _pNodeContextMgr);
    if (_pTopology == nullptr) {
        initErr (pszMethodName, "Topology", rc);
        return -11;
    }

    if ((rc = _pNodeContextMgr->configure (&_adaptMgr, _pTopology, _pMetadataConf)) < 0) {
        initErr (pszMethodName, "NodeContextMgr", rc);
        return -12;
    }

    // Instantiate or connect to VoI
    _pVoi = new Voi (sessionId);
    if (_pVoi->init() < 0) {
        return -13;
    }

    // Instantiate Scheduler
    _pScheduler = Scheduler::getScheduler (pCfgMgr, this, &_adaptMgr, _pDataStore, _pNodeContextMgr, _pInfoStore, _pTopology, _pVoi);
    if (_pScheduler == nullptr) {
        initErr (pszMethodName, "Scheduler", rc);
        return -14;
    }

    InformationPushPolicy *pInfoPushPolicy = instantiateInformationPushPolicy (pCfgMgr, _pInfoStore);
    if (pInfoPushPolicy == nullptr) {
        initErr (pszMethodName, "InformationPushPolicy", rc);
        return -15;
    }

    _pController = new Controller (this, _pLocalNodeContext, _pScheduler, _pInfoStore, _pTopology);
    if ((_pController == nullptr) || ((rc = _pController->init (pCfgMgr, pInfoPushPolicy, _pScheduler, _pVoi)) < 0)) {
        initErr (pszMethodName, "Controller", rc);
        return -16;
    }

    if ((rc = _adaptMgr.init (pCfgMgr, sessionId, _pController, _pDataStore->getPropertyStore())) < 0) {
        return -17;
    }

    _pPositionUpdater = new PositionUpdater (_pNodeContextMgr, this);
    if (_pPositionUpdater == nullptr) {
        return -18;
    }

    // Load query controllers
    QueryController *pQueryCtrlr = new DSProQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == nullptr || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "DSProQueryController", rc);
        return -19;
    }
    _controllers.prepend (pQueryCtrlr);

    pQueryCtrlr = new ApplicationQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == nullptr || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "ApplicationQueryController", rc);
        return -20;
    }
    _controllers.prepend (pQueryCtrlr);

    pQueryCtrlr = new DisServiceQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == nullptr || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "DisServiceQueryController", rc);
        return -21;
    }
    _controllers.prepend (pQueryCtrlr);

    pQueryCtrlr = new ChunkQueryController (this, _pDataStore, _pInfoStore);
    if (pQueryCtrlr == nullptr || (rc = pQueryCtrlr->init (_pMetadataConf, &_adaptMgr)) < 0) {
        initErr (pszMethodName, "ChunkQueryController", rc);
        return -22;
    }
    _controllers.prepend (pQueryCtrlr);

    // Start threads
    rc = _adaptMgr.startAdaptors();
    if (rc < 0) {
        initWarn (pszMethodName, "CommAdaptorManager", rc);
        return -23;
    }
    _pPositionUpdater->start();

    if (pCfgMgr->getValueAsBool (NetLogger::ENABLE_PROPERTY, false)) {
        pNetLogger = new NetLogger (_nodeId);
        if (pNetLogger != nullptr) {
            if (pNetLogger->init (&_adaptMgr) < 0) {
                initWarn (pszMethodName, "NetLogger", rc);
            }
            else if (pNetLogger->notify (pCfgMgr) < 0) {
                initWarn (pszMethodName, "NetLogger", rc);
            }
        }
    }

    // Load path if it is configured
    if (loadPath (pCfgMgr, this) < 0) {
        return -24;
    }

    return 0;
}

int DSProImpl::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    return _adaptMgr.changeEncryptionKey (pchKey, ui32Len);
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

MetadataConfigurationImpl * DSProImpl::getMetadataConf (void)
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
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pMetadata == nullptr) {
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
    char *pszDataId = nullptr;
    if ((pData != nullptr) && (ui32DataLen > 0)) {
        const uint8 ui8NChunks = _chunkingConf.getNumberofChunks (ui32DataLen);
        int rc = _pPublisher->chunkAndAddData (pub, nullptr, nullptr, 0U, pData, ui32DataLen, nullptr,
                                               &pszDataId, ui8NChunks, false); // annotation can't be fragmented
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
            _m.unlock (2016);
            return -3;
        }
        assert (pszDataId != nullptr);
    }

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated

    pub.pszReferredObjectId = pszDataId != nullptr ? pszDataId : MetadataInterface::NO_REFERRED_OBJECT;

    int rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, true, false);
    _m.unlock (2016);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s referring to data %s.\n",
                    sMetadataId.c_str(), (pszDataId == nullptr ? "NULL" : pszDataId));

    if (rc == 0) {
        int rcPush = _pController->metadataPush (sMetadataId, pMetadata);
        if (rcPush < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "could not push metadata. Returned code %d\n", rcPush);
        }
        if (_bEnableLoopbackNotifications) {
            const char *pszMsgIds[2] = { sMetadataId.c_str (), nullptr };
            _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
        }
        if (ppszId != nullptr) {
            *ppszId = sMetadataId.r_str();
        }
    }

    if (pszDataId != nullptr) {
        free (pszDataId);
    }

    return (rc < 0 ? -4 : 0);

}

int DSProImpl::addMessage (const char *pszGroupName, const char *pszObjectId,
                           const char *pszInstanceId, MetaData *pMetadata,
                           const void *pData, uint32 ui32DataLen,
                           int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSProImpl::addMessage";
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pMetadata == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not create metadata.\n");
        return -1;
    }

    String mimeType;
    pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, mimeType);

    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);

    // setAndAddMetadata modifies pMetadata - no problem here, since pMetadata
    // is not used anymore, in fact, it is even deallocated
    _m.lock (2012);
    char *pszDataId = nullptr;
    if ((pData != nullptr) && (ui32DataLen > 0)) {
        int rc = _pPublisher->chunkAndAddData (pub, nullptr, nullptr, 0U, pData, ui32DataLen, mimeType,
                                               &pszDataId, 0, false);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
            _m.unlock (2012);
            return -3;
        }
        assert (pszDataId != nullptr);
    }

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated

    pub.pszReferredObjectId = pszDataId != nullptr ? pszDataId : MetadataInterface::NO_REFERRED_OBJECT;
    int rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, true, false);

    _m.unlock (2012);

    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s "
                        "referring to data %s.\n", sMetadataId.c_str(), pub.pszReferredObjectId);
        int rcPush = _pController->metadataPush (sMetadataId, pMetadata);
        if (rcPush < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not push metadata. Returned code %d\n", rcPush);
        }
        if (_bEnableLoopbackNotifications) {
            const char *pszMsgIds[2] = { sMetadataId.c_str (), nullptr };
            _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
        }
        if (ppszId != nullptr) {
            *ppszId = sMetadataId.r_str();
        }
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "could not add message metadata %s "
                        "referring to data %s.\n", sMetadataId.c_str(), pub.pszReferredObjectId);
    }
    if (pszDataId != nullptr) {
        free (pszDataId);
    }

    return (rc < 0 ? -4 : 0);
}

int DSProImpl::addChunkedMessage (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                  MetaData *pMetadata, PtrLList<IHMC_MISC::Chunker::Fragment> *pChunks,
                                  const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSProImpl::addChunkedMessage";
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || pszDataMimeType == nullptr) {
        return -1;
    }
    if (pMetadata == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not create metadata.\n");
        return -2;
    }
    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);
    char *pszDataId = nullptr;
    _m.lock (2014);
    int rc = _pPublisher->addChunkedData (pub, pChunks, pszDataMimeType, false, pszDataId);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
        _m.unlock (2014);
        return -3;
    }

    assert (pszDataId != nullptr);

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated
    pub.pszReferredObjectId = pszDataId;
    rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, true, false);

    _m.unlock (2014);

    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s "
                        "referring to data %s.\n", sMetadataId.c_str(), pub.pszReferredObjectId);
        int rcPush = _pController->metadataPush (sMetadataId, pMetadata);
        if (rcPush < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not push metadata. Returned code %d\n", rcPush);
        }
        if (_bEnableLoopbackNotifications) {
            const char *pszMsgIds[2] = { sMetadataId.c_str (), nullptr };
            _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
        }
        if (ppszId != nullptr) {
            *ppszId = sMetadataId.r_str();
        }
    }
    if (pszDataId != nullptr) {
        free (pszDataId);
    }

    return (rc < 0 ? -4 : 0);
}

int DSProImpl::addAdditionalChunk (const char *pszId, const char *pszReferredObjectId,
                                   const char *pszObjectId, const char *pszInstanceId,
                                   IHMC_MISC::Chunker::Fragment *pChunk, int64 i64ExpirationTime)
{
    const char *pszMethodName = "DSProImpl::addAdditionalChunk";
    if ((pszId == nullptr) || (pChunk == nullptr) || (pChunk->src_type.length() <= 0)) {
        return -1;
    }
    uint8 ui8NChunks, ui8TotChunks;
    ui8NChunks = ui8TotChunks = 0;
    if (_pDataStore->getNumberOfReceivedChunks (pszReferredObjectId, ui8NChunks, ui8TotChunks) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "error when trying "
                        "to check the number of received chunks for message %s\n", pszId);
        return -2;
    }
    if (ui8NChunks >= ui8TotChunks) {
        return -3;
    }
    pChunk->ui8TotParts = ui8TotChunks;
    Publisher::PublicationInfo pub ("", pszObjectId, pszInstanceId, i64ExpirationTime);
    _m.lock (2014);
    int rc = _pPublisher->addChunkedData (pszReferredObjectId, pub, pChunk, pChunk->src_type);
    _m.unlock (2014);
    return (rc < 0 ? -4 : 0);
}

int DSProImpl::chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                   const char *pszInstanceId, MetaData *pMetadata,
                                   const void *pData, uint32 ui32DataLen,
                                   const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId,
                                   bool bPush)
{
    MissionPackage missionPkg;
    if (missionPkg.isMissionPackage (pszDataMimeType)) {
        return missionPkg.addAsChunkedMissionPkg (this, pszGroupName, pszObjectId,
                                                  pszInstanceId, pMetadata,
                                                  pData, ui32DataLen, pszDataMimeType,
                                                  i64ExpirationTime, ppszId);
    }
    return chunkAndAddMessageInternal (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                       pData, ui32DataLen, pszDataMimeType, i64ExpirationTime, ppszId,
                                       bPush);
}

int DSProImpl::chunkAndAddMessageInternal (const char *pszGroupName, const char *pszObjectId,
                                           const char *pszInstanceId, MetaData *pMetadata,
                                           const void *pData, uint32 ui32DataLen,
                                           const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId,
                                           bool bPush)
{
    const char *pszMethodName = "DSProImpl::chunkAndAddMessage";
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || pData == nullptr || ui32DataLen == 0U || pszDataMimeType == nullptr) {
        return -1;
    }
    if (pMetadata == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not create metadata.\n");
        return -2;
    }

    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);

    // setAndAddMetadata modifies pMetadata - no problem here, since pMetadata
    // is not used anymore, in fact, it is even deallocated
    char *pszDataId = nullptr;

    _m.lock (2014);

    const uint8 ui8NChunks = _chunkingConf.getNumberofChunks (ui32DataLen);
    int rc = _pPublisher->chunkAndAddData (pub, nullptr, nullptr, 0U, pData, ui32DataLen,
                                           pszDataMimeType, &pszDataId, ui8NChunks, false);

    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not store data.\n");
        _m.unlock (2014);
        return -3;
    }
    assert (pszDataId != nullptr);

    String sMetadataId;
    // setReferredObjAddAndPushAnnotation() modifies pMetadata - no problem here,
    // since pMetadata is not used anymore, in fact, it is even deallocated
    pub.pszReferredObjectId = pszDataId;
    rc = _pPublisher->setAndAddMetadata (pub, pMetadata, sMetadataId, bPush, false);

    _m.unlock (2014);

    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added message metadata %s "
                        "referring to data %s.\n", sMetadataId.c_str(), pub.pszReferredObjectId);
        int rcPush = bPush ? _pController->metadataPush (sMetadataId, pMetadata) : 0;
        if (rcPush < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not push metadata. Returned code %d\n", rcPush);
        }
        if (_bEnableLoopbackNotifications) {
            const char *pszMsgIds[2] = { sMetadataId.c_str(), nullptr };
            _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, pszMsgIds);
        }
        if (ppszId != nullptr) {
            *ppszId = sMetadataId.r_str();
        }
    }
    if (pszDataId != nullptr) {
        free (pszDataId);
    }

    return (rc < 0 ? -4 : 0);
}

int DSProImpl::disseminateMessage (const char *pszGroupName, const char *pszObjectId,
                                   const char *pszInstanceId, const void *pData,
                                   uint32 ui32DataLen, int64 i64ExpirationTime, char **ppszId)
{
    const uint8 ui8NChunks = 0;
    const bool bDisseminate = true;
    const char *pszDataMimeType = nullptr;
    const char *pszAnnotatedObjMsgId = nullptr;
    const void *pAnnotationMetadata = nullptr;
    const uint32 ui32AnnotationMetadataLen = 0U;
    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);

    _m.lock (2018);
    int rc = _pPublisher->chunkAndAddData (pub, pszAnnotatedObjMsgId, pAnnotationMetadata, ui32AnnotationMetadataLen,
                                           pData, ui32DataLen, pszDataMimeType, ppszId, ui8NChunks, bDisseminate);
    _m.unlock (2018);

    if ((ppszId != nullptr) && (*ppszId != nullptr)) {
        _pScheduler->addMessageToDisseminated (*ppszId);
        // const char * messageIds[2] = { *ppszId , nullptr };
        // _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, messageIds);
    }

    return rc;
}

int DSProImpl::disseminateMessageMetadata (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                           const void *pMetadata, uint32 ui32MetadataLen,
                                           const void *pData, uint32 ui32DataLen, const char *pszMimeType,
                                           int64 i64ExpirationTime, char **ppszId)
{
    const uint8 ui8NChunks = _chunkingConf.getNumberofChunks (ui32DataLen);
    const void *pAnnotationMetadata = nullptr;
    const uint32 ui32AnnotationMetadataLen = 0U;
    String dataGroup ("ds.data.");
    dataGroup += pszGroupName;
    Publisher::PublicationInfo pub (dataGroup, pszObjectId, pszInstanceId, i64ExpirationTime);

    _m.lock (2018);
    // Publish data
    char *pszDataId = nullptr;
    int rc = _pPublisher->chunkAndAddData (pub, nullptr, pAnnotationMetadata, ui32AnnotationMetadataLen,
                                           pData, ui32DataLen, pszMimeType, &pszDataId, ui8NChunks, true);
    _m.unlock (2018);
    if (pszDataId == nullptr) {
        return -1;
    }

    *ppszId = _pPublisher->assignIdAndAddMetadata (pszGroupName, pszObjectId, pszInstanceId, nullptr, nullptr, pszDataId,
                                                   pMetadata, ui32MetadataLen, i64ExpirationTime, true);
    if ((ppszId != nullptr) && (*ppszId != nullptr)) {
        _pScheduler->addMessageToDisseminated (*ppszId);
        // const char * messageIds[2] = { *ppszId , nullptr };
        // _pPositionUpdater->addMetadataToNotify (CallbackHandler::LOOPBACK_NOTIFICATION, messageIds);
    }

    return rc;
}

int DSProImpl::subscribe (CommAdaptor::Subscription &sub)
{
    return _adaptMgr.subscribe (sub);
}

int DSProImpl::addAnnotationNoPrestage (const char *pszGroupName, const char *pszObjectId,
                                        const char *pszInstanceId, MetadataInterface *pMetadata,
                                        const char *pszReferredObject, int64 i64ExpirationTime,
                                        char **ppszId)
{
    if ((pszGroupName == nullptr) || (pMetadata == nullptr) || (ppszId == nullptr)) {
        return -1;
    }

    _m.lock (2018);
    String msgId;
    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);
    pub.pszReferredObjectId = pszReferredObject;
    int rc = _pPublisher->setAndAddMetadata (pub, pMetadata, msgId, false, false);
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
    if ((pszGroupName == nullptr) || (pData == nullptr) || (ui32DataLen == 0U) || (ppszId == nullptr)) {
        return -1;
    }
    Publisher::PublicationInfo pub (pszGroupName, pszObjectId, pszInstanceId, i64ExpirationTime);
    _m.lock (2018);
    int rc = _pPublisher->chunkAndAddData (pub, pszAnnotatedObjMsgId, pszAnnotationMetadata,
                                           ui32AnnotationMetdataLen, pData, ui32DataLen,
                                           pszDataMimeType, ppszId, 0, false);
    _m.unlock (2018);
    return rc;
}

int DSProImpl::setAndAddMetadata (Publisher::PublicationInfo &pubInfo, MetadataInterface *pMetadata,
                                  String &msgId, bool bStoreInInfoStore)
{
    _m.lock (2015);
    int rc = _pPublisher->setAndAddMetadata (pubInfo, pMetadata, msgId, bStoreInInfoStore, false);
    _m.unlock (2015);
    return rc;
}

int DSProImpl::addPeer (AdaptorType adaptorType, const char *pszNetworkInterface,
                        const char *pszRemoteAddress, uint16 ui16Port)
{
    // TODO: change _adaptMgr's code to also take and use pszNetworkInterface
    if (/*pszNetworkInterface == nullptr ||*/ pszRemoteAddress == nullptr) {
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
    if (pszId == nullptr) {
        return -1;
    }
    _userReqs.put (pszId, pszQueryId);
    return 0;
}

int DSProImpl::addUserId (const char *pszUserName)
{
    const char *pszMethodName = "DSProImpl::addUserId";
    if (pszUserName == nullptr) {
        return -1;
    }
    _m.lock (2001);
    if (_pNodeContextMgr == nullptr) {
        _m.unlock (2001);
        nodeCtxMgrNotInitialized (pszMethodName);
        return -2;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    if (pLocalNodeContext == nullptr) {
        _m.unlock (2001);
        return -3;
    }
    int rc = pLocalNodeContext->addUserId (pszUserName);
    _pNodeContextMgr->releaseLocalNodeContext();

    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added user id %s.\n", pszUserName);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not add user id %s. "
                        "Return code %d.\n", pszUserName, rc);
    }

    _m.unlock (2001);
    return rc;
}

int DSProImpl::addAreaOfInterest (const char *pszAreaName, BoundingBox &bb, int64 i64StatTime, int64 i64EndTime)
{
    const char *pszMethodName = "DSProImpl::addAreaOfInterest";
    if (pszAreaName == nullptr) {
        return -1;
    }
    _m.lock (2001);
    if (_pNodeContextMgr == nullptr) {
        _m.unlock (2001);
        nodeCtxMgrNotInitialized (pszMethodName);
        return -2;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    if (pLocalNodeContext == nullptr) {
        _m.unlock (2001);
        return -3;
    }
    int rc = pLocalNodeContext->addAreaOfInterest (pszAreaName, bb, i64StatTime, i64EndTime);
    _pNodeContextMgr->releaseLocalNodeContext();

    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added area of interest %s.\n", pszAreaName);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not add area of interest %s. "
            "Return code %d.\n", pszAreaName, rc);
    }

    _m.unlock (2001);
    return rc;
}

int DSProImpl::addCustomPolicy (CustomPolicyImpl *pPolicy)
{
    const char *pszMethodName = "DSProImpl::addCustomPolicy";
    if (pPolicy == nullptr) {
        return -1;
    }
    _m.lock (2001);
    if (_pNodeContextMgr == nullptr) {
        _m.unlock (2001);
        nodeCtxMgrNotInitialized (pszMethodName);
        return -2;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    if (pLocalNodeContext == nullptr) {
        _m.unlock (2001);
        return -3;
    }
    int rc = pLocalNodeContext->addCustomPolicy (pPolicy);

    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added custom policy of type %d.\n",
                        pPolicy->getType());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not add custom policy of type %d. Return code: %d.\n",
                        pPolicy->getType(), rc);
    }

    _m.unlock (2001);
    return rc;
}

void DSProImpl::asynchronouslyNotifyMatchingMetadata (const char *pszQueryId, const char **ppszMsgIds)
{
    if (pszQueryId == nullptr && ppszMsgIds != nullptr && ppszMsgIds[0] != nullptr) {
        return;
    }

    _pPositionUpdater->addMetadataToNotify (pszQueryId, ppszMsgIds);
}

void DSProImpl::asynchronouslyNotifyMatchingSearch (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen)
{
    if (pszQueryId == nullptr && pReply != nullptr && ui16ReplyLen == 0) {
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

    const String id (pszId);

    if ((id.length() <= 0) || (ppData == nullptr) || ((id == MetaData::NO_REFERRED_OBJECT) == 1)) {
        return -1;
    }

    *ppData = nullptr;
    const String grpName (extractGroupFromKey (id));
    if (grpName.length() <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Could not extract group from message id %s.\n", id.c_str());
        return -2;
    }

    if (_pDataStore->getData (id, ppData, ui32DataLen) < 0) {
        // The data is in the cache, but could not be read correctly
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Could not read the data from the datacache.\n");
        return -3;
    }

    _m.lock (2008);
    if ((*ppData == nullptr) || (ui32DataLen == 0)) {
        // the data has not arrived yet, register the data request
        if (_userReqs.put (id, pszCallbackParameter) == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "data message "
                            "%s not found. Added it to _userReqs\n", id.c_str());
        }

        // Hack for missiong packages
        if (grpName.endsWith ("part") || grpName.endsWith ("part.[od]") || grpName.endsWith ("manifest")) {
            int rc = sendAsynchronousRequestMessage (id);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "can not "
                                "request referred message %s. Returned code %d\n",
                                id.c_str(), rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "requested "
                                "referred message %s\n", id.c_str());
            }
        }
    }

    updateUsage (id);

    if (*ppData == nullptr) {
        _m.unlock (2008);
        return 1;
    }

    // Copy the returned object
    void *pDataCpy = nullptr;
    if (isOnDemandGroupName (grpName)) {
        pDataCpy = *ppData;
    }
    else {
        uint32 ui32NewLen = 0;
        MessageHeaders::MsgType type;
        pDataCpy = MessageHeaders::removeDSProHeader (*ppData, ui32DataLen, ui32NewLen, type);
        if (pDataCpy == nullptr) {
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
    if (_pDataStore->getNumberOfReceivedChunks (id, ui8NChunks, ui8TotChunks) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "error when trying "
                        "to check the number of received chunks for message %s\n", id.c_str());
        free (*ppData);
        return -6;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "getNumberOfReceivedChunks() "
                    "says that %d chunks out of %d have been received\n", ui8NChunks, ui8TotChunks);

    bHasMoreChunks = (ui8TotChunks > ui8NChunks);
    return 0;
}

char ** DSProImpl::getMatchingMetadataAsJson (AVList *pAVQueryList, int64 i64BeginArrivalTimestamp, int64 i64EndArrivalTimestamp)
{
    MetadataList *pMetadataList = _pInfoStore->getAllMetadata (pAVQueryList, i64BeginArrivalTimestamp, i64EndArrivalTimestamp);
    if (pMetadataList == nullptr || pMetadataList->getFirst() == nullptr) {
        return nullptr;
    }
    int iCount = pMetadataList->getCount();
    if (iCount <= 0) {
        return nullptr;
    }
    char **ppszMetadataAsXML = static_cast<char **>(calloc (iCount + 1, sizeof (char *)));
    if (ppszMetadataAsXML != nullptr) {
        // Convert the fields to XML
        MetadataInterface *pCurr, *pNext;
        pNext = pMetadataList->getFirst();
        for (unsigned int i = 0; ((pCurr = pNext) != nullptr) && (i < ((unsigned int) iCount)); i++) {
            pNext = pMetadataList->getNext();
            std::unique_ptr<JsonObject> pJson (pCurr->toJson());
            String json (pJson->toString (true));
            ppszMetadataAsXML[i] = json.r_str();
            delete pMetadataList->remove (pCurr);
        }
    }

    return ppszMetadataAsXML;
}

const char * DSProImpl::getNodeId (void) const
{
    return _nodeId.c_str();
}

DArray2<String> * DSProImpl::getPeerList (void)
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

int DSProImpl::notUseful (const char *pszMessageID)
{
    if (pszMessageID == nullptr) {
        return -1;
    }
    _m.lock (2009);
    int rc = updateLearning (pszMessageID, Classification::NotUseful);
    _m.unlock (2009);
    return (rc < 0 ? -2 : 0);
}

void DSProImpl::sendWaypointMessage (const void *pBuf, uint32 ui32BufLen)
{
    if (pBuf == nullptr || ui32BufLen == 0) {
        return;
    }

    PtrLList<String> *pNeighborList = _pTopology->getNeighbors();
    if (pNeighborList == nullptr) {
        return;
    }
    if (pNeighborList->getFirst() == nullptr) {
        delete pNeighborList;
        return;
    }

    NodeIdSet nodeIdSet;
    PreviousMessageIds previousMessageIds;
    String *pszNextPeerId = pNeighborList->getFirst();
    for (String *pszCurrPeerId; (pszCurrPeerId = pszNextPeerId) != nullptr;) {
        pszNextPeerId = pNeighborList->getNext();
        const String latestMsg (_pScheduler->getLatestMessageReplicatedToPeer (pszCurrPeerId->c_str()));
        previousMessageIds.add (pszCurrPeerId->c_str(), latestMsg);
        nodeIdSet.add (pszCurrPeerId->c_str());
        delete pNeighborList->remove (pszCurrPeerId);
    }
    delete pNeighborList;

    const String latestResetMsg (_pScheduler->getLatestResetMessage());
    if (latestResetMsg.length() <= 0) {
        previousMessageIds.add ("*", latestResetMsg);
    }

    uint32 ui32TotalLen = 0;
    void *pData = WaypointMessageHelper::writeWaypointMessageForTarget (previousMessageIds, pBuf, ui32BufLen, ui32TotalLen);
    Targets **ppTargets = _pTopology->getNextHopsAsTarget (nodeIdSet);
    if ((ppTargets != nullptr) && (ppTargets[0] != nullptr)) {
        // Send the waypoint message on each available interface that reaches the recipients
        int rc = _adaptMgr.sendWaypointMessage (pData, ui32TotalLen, _nodeId, ppTargets);
        String sLatestMsgs (previousMessageIds);
        String sPeers (nodeIdSet);
        checkAndLogMsg ("DSPro::sendWaypointMessage", Logger::L_Info, "sending waypoint message "
                        "to %s (%s); last message pushed to this node was %s.\n", sPeers.c_str(),
                        (rc == 0 ? "succeeded" : "failed"), sLatestMsgs.c_str());
    }
    Targets::deallocateTargets (ppTargets);
    free (pData);
}

int DSProImpl::updateUsage (const char *pszMessageId)
{
    const char *pszMethodName = "DSProImpl::updateUsage";
    if (pszMessageId == nullptr) {
        return -1;
    }

    MetadataList *pMetadataList = _pInfoStore->getMetadataForData (pszMessageId);
    if (pMetadataList == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "metadata not "
                        "found for data message with id %s.\n", pszMessageId);
        return -2;
    }
    else {
        for (MetadataInterface *pMetadata = pMetadataList->getFirst(); pMetadata != nullptr;
             pMetadata = pMetadataList->getNext()) {
            char *pszMetadataId = nullptr;
            if (0 == pMetadata->getFieldValue (pMetadata->MESSAGE_ID, &pszMetadataId) &&
                pszMetadataId != nullptr) {
                updateLearning (pszMetadataId, Classification::Useful);
                free (pszMetadataId);
            }
            delete pMetadata;
        }
        delete pMetadataList;
    }

    return 0;
}

int DSProImpl::updateLearning (const char *pszMessageId, uint8 ui8Usage)
{
    const char *pszMethodName = "DSPro::updateLearning";
    if (pszMessageId == nullptr) {
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
    if (pMetadataList != nullptr) {
        MetadataInterface *pCurr, *pNext;
        pNext = pMetadataList->getFirst ();
        for (unsigned int i = 0; ((pCurr = pNext) != nullptr); i++) {
            pNext = pMetadataList->getNext ();

            IHMC_C45::C45AVList *pDataset = C45Utils::getMetadataAsDataset (pCurr, _pMetadataConf);
            if (pDataset == nullptr) {
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
    if (ppTargets != nullptr) {
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

    if (searchProp.pszQueryId == nullptr) {
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
    if (pszId == nullptr) {
        return -1;
    }

    _pPositionUpdater->requestMessage (pszId);
    return 0;
}

int DSProImpl::setMissionId (const char *pszMissionName)
{
    if (pszMissionName == nullptr) {
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setMissionId (pszMissionName);
    _pNodeContextMgr->releaseLocalNodeContext();
    checkAndLogMsg ("DSProImpl::setMissionId", Logger::L_Info, "added mission id %s.\n", pszMissionName);
    return 0;
}

int DSProImpl::setRole (const char *pszRole)
{
    if (pszRole == nullptr) {
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setRole (pszRole);
    _pNodeContextMgr->releaseLocalNodeContext();
    checkAndLogMsg ("DSProImpl::setRole", Logger::L_Info, "added role %s.\n", pszRole);
    return 0;
}

int DSProImpl::setTeamId (const char *pszTeamId)
{
    if (pszTeamId == nullptr) {
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setTeam (pszTeamId);
    _pNodeContextMgr->releaseLocalNodeContext();
    checkAndLogMsg ("DSProImpl::setTeamId", Logger::L_Info, "added team %s.\n", pszTeamId);
    return 0;
}

int DSProImpl::setNodeType (const char *pszType)
{
    if (pszType == nullptr) {
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setNodeType (pszType);
    _pNodeContextMgr->releaseLocalNodeContext();
    checkAndLogMsg ("DSProImpl::setNodeType", Logger::L_Info, "added node type %s.\n", pszType);
    return 0;
}

int DSProImpl::setCurrentPath (const char *pszPathID)
{
    _m.lock (2002);
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized ("DSProImpl::setCurrentPath");
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
    const char *pszMethodName = "DSProImpl::setCurrentPosition";
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized (pszMethodName);
        return -1;
    }
    if (_pController == nullptr) {
        return -2;
    }

    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    bool bPositionChanged = pLocalNodeContext->setCurrentPosition (fLatitude, fLongitude,
                                                                   fAltitude, pszLocation, pszNote,
                                                                   getTimeInMilliseconds());
    _pNodeContextMgr->releaseLocalNodeContext();
    if (!bPositionChanged) {
        return 0;
    }

    BufferWriter bw (1024, 128);
    int rc = _pNodeContextMgr->updatePosition (&bw);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "\n");
        return -3;
    }
    sendWaypointMessage (bw.getBuffer(), bw.getBufferLength());

    _pPositionUpdater->positionUpdated();

    return 0;
}

int DSProImpl::setMatchingThreshold (float fMatchmakingThreshold)
{
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized ("DSProImpl::setMatchingThreshold");
        return -1;
    }
    if ((fMatchmakingThreshold < 0.0f) || (fMatchmakingThreshold > 10.0f)) {
        return -2;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setMatchmakingThreshold (fMatchmakingThreshold);
    _pNodeContextMgr->releaseLocalNodeContext();
    return 0;
}

int DSProImpl::setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeOfInfluenceInMeters)
{
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized ("DSProImpl::setDefaultUsefulDistance (1)");
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setRangeOfInfluence (pszNodeType, ui32RangeOfInfluenceInMeters);
    _pNodeContextMgr->releaseLocalNodeContext();
    return 0;
}

int DSProImpl::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized ("DSProImpl::setDefaultUsefulDistance (1)");
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setDefaultUsefulDistance (ui32UsefulDistanceInMeters);
    _pNodeContextMgr->releaseLocalNodeContext();
    return 0;
}

int DSProImpl::setUsefulDistance (const char *pszMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized ("DSProImpl::setDefaultUsefulDistance (2)");
        return -1;
    }
    LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
    pLocalNodeContext->setUsefulDistance (pszMIMEType, ui32UsefulDistanceInMeters);
    _pNodeContextMgr->releaseLocalNodeContext();
    return 0;
}

int DSProImpl::registerPath (NodePath *pPath)
{
    if (pPath == nullptr) {
        return -1;
    }
    if (pPath->getPathLength() <= 0) {
        checkAndLogMsg ("DSProImpl::registerPath", Logger::L_Warning,
            "trying to register path with no waypoints.\n");
        return -2;
    }
    _m.lock (2001);
    if (_pNodeContextMgr == nullptr) {
        nodeCtxMgrNotInitialized ("DSProImpl::registerPath");
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
    if (pszId == nullptr) {
        return -1;
    }

    _pPositionUpdater->removeMessageRequest (pszId);
    return 0;
}

int DSProImpl::requestMoreChunks (const char *pszChunkedMsgId, const char *pszCallbackParameter)
{
    const char *pszMethodName = "DSProImpl::requestMoreChunks";
    if (pszChunkedMsgId == nullptr) {
        return -1;
    }

    _m.lock (2010);
    if (_userReqs.put (pszChunkedMsgId, pszCallbackParameter) == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "new chunk requested "
                        "and added it to _userReqs\n", pszChunkedMsgId);
    }

    Targets **ppTargets = _pTopology->getNeighborsAsTargets();
    if (ppTargets == nullptr) {
        _m.unlock (2010);
        return 0;
    }
    if (ppTargets[0] == nullptr) {
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
    if (pCachedChunkIds != nullptr) {
        for (unsigned int i = 0; i < pCachedChunkIds->size(); i++) {
            chunkIds += ((uint32) (*pCachedChunkIds)[i]);
            chunkIds += " ";
        }
        delete pCachedChunkIds;
        pCachedChunkIds = nullptr;
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

int DSProImpl::registerChunkFragmenter (const char *pszMimeType, IHMC_MISC::ChunkerInterface *pChunker)
{
    return _pDataStore->getChunkingManager()->registerChunker (pszMimeType, pChunker);
}

int DSProImpl::registerChunkReassembler (const char *pszMimeType, IHMC_MISC::ChunkReassemblerInterface *pReassembler)
{
    return _pDataStore->getChunkingManager()->registerReassembler (pszMimeType, pReassembler);
}

int DSProImpl::deregisterChunkFragmenter (const char *pszMimeType)
{
    return _pDataStore->getChunkingManager()->deregisterChunker (pszMimeType);
}

int DSProImpl::deregisterChunkReassembler (const char *pszMimeType)
{
    return _pDataStore->getChunkingManager()->deregisterReassembler (pszMimeType);
}

int DSProImpl::dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                            const void *pBuf, uint32 ui32Len, uint8 ui8ChunkIndex, uint8 ui8TotNChunks,
                            const char *pszQueryId)
{
    const char *pszMethodName = "DSProImpl::dataArrived";
    String dsproQueryId;
    const String group (pszGroupName);
    if (group.startsWith (DisServiceAdaptor::DSPRO_GROUP_NAME) && !_userReqs.contains (pszId, dsproQueryId)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s of length %u arrived. "
                        "It was not requested, therefore it will not be delivered "
                        "to the applications\n", pszId, ui32Len);
        return 0;
    }
    if ((pszQueryId == nullptr) && (dsproQueryId.length() > 0)) {
        pszQueryId = dsproQueryId.c_str();
    }

    // Notify DSPro internal clients
    _pAMTDict->messageArrived (pBuf, ui32Len, pszMimeType);

    // Notify DSPro external clients
    return _cbackHandler.dataArrived (pszId, pszGroupName, pszObjectId,
                                      pszInstanceId, pszAnnotatedObjMsgId, pszMimeType,
                                      pBuf, ui32Len, ui8ChunkIndex, ui8TotNChunks, pszQueryId);
}

int DSProImpl::metadataArrived (const char *pszId, const char *pszGroupName,
                                const char *pszObjectId, const char *pszInstanceId,
                                const void *pBuf, uint32 ui32Len, const char *pszReferredDataId,
                                const char *pszQueryId)
{
    const char *pszMethodName = "DSProImpl::metadataArrived";
    if (pszId == nullptr || pBuf == nullptr || ui32Len == 0 || pszReferredDataId == nullptr) {
        return -1;
    }

    MetaData *pMetadata = toMetadata (pBuf, ui32Len);
    if (pMetadata == nullptr) {
        checkAndLogMsg (pszMethodName, dataDeserializationError, pszId);
        return -2;
    }

    String dsproQueryId;
    if (pszQueryId == nullptr) {
        if (_userReqs.contains (pszId, dsproQueryId) && (dsproQueryId.length() > 0)) {
            pszQueryId = dsproQueryId.c_str();
        }
    }

    int rc = metadataArrived (pszId, pszGroupName, pszObjectId, pszInstanceId,
                              pMetadata, pszReferredDataId, pszQueryId);
    delete pMetadata;
    pMetadata = nullptr;

    return (int) rc;
}

int DSProImpl::metadataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                const char *pszInstanceId, const MetaData *pMetadata, const char *pszReferredDataId,
                                const char *pszQueryId)
{
    String metaDataTarget;
    if (0 != pMetadata->getFieldValue (MetaData::TARGET_ID, metaDataTarget)) {
        metaDataTarget = nullptr;
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
    if (metaDataTarget.length() > 0 && _pNodeContextMgr != nullptr) {
        LocalNodeContext *pLocalNodeContext = _pNodeContextMgr->getLocalNodeContext();
        if (pLocalNodeContext != nullptr) {
            bIsTarget = MatchMakingPolicies::isNodeOrUserTarget (metaDataTarget, pLocalNodeContext);
            _pNodeContextMgr->releaseLocalNodeContext();
        }
    }

    String dsproQueryId;
    if (pszQueryId == nullptr) {
        if (_userReqs.contains (pszId, dsproQueryId) && (dsproQueryId.length() > 0)) {
            pszQueryId = dsproQueryId.c_str();
        }
    }

    // Notify DSPro internal clients
    String appMetadata, appMetadataMIMEType;
    if ((pMetadata->getFieldValue (MetaData::APPLICATION_METADATA, appMetadata) == 0) &&
        (pMetadata->getFieldValue (MetaData::APPLICATION_METADATA_FORMAT, appMetadataMIMEType) == 0)) {
        _pAMTDict->messageArrived (appMetadata, appMetadata.length(), appMetadataMIMEType);
    }

    // Notify DSPro external clients
    std::unique_ptr<JsonObject> pJson (pMetadata->toJson());
    String json (pJson->toString (true));
    int rc = _cbackHandler.metadataArrived (pszId, pszGroupName, refObjectId, refInstanceId,
                                            json, pszReferredDataId, pszQueryId, bIsTarget);

    return rc;
}

int DSProImpl::dataAvailable (const char *pszId, const char *pszGroupName,
                              const char *pszObjectId, const char *pszInstanceId,
                              const char *pszRefObjId, const char *pszMimeType,
                              const void *pMetadata, uint32 ui32MetadataLength,
                              const char *pszQueryId)
{
    String dsproQueryId;
    if (pszQueryId == nullptr) {
        if (_userReqs.contains (pszId, dsproQueryId) && (dsproQueryId.length () > 0)) {
            pszQueryId = dsproQueryId.c_str();
        }
    }

    // Notify DSPro external clients
    int rc = _cbackHandler.dataAvailable (pszId, pszGroupName, pszObjectId, pszInstanceId,
                                          pszMimeType, pszRefObjId, pMetadata, ui32MetadataLength,
                                          pszQueryId);

    return rc;
}

int DSProImpl::newPeer (const char *pszNewPeerId)
{
    if (pszNewPeerId == nullptr) {
        return -1;
    }
    _pPositionUpdater->topologyHasChanged();
    return _cbackHandler.newPeer (pszNewPeerId);
}

int DSProImpl::deadPeer (const char *pszDeadPeerId)
{
    if (pszDeadPeerId == nullptr) {
        return -1;
    }
    _pPositionUpdater->topologyHasChanged();
    return _cbackHandler.deadPeer (pszDeadPeerId);
}
