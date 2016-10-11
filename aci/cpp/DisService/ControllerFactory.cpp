/*
 * ControllerFactory.cpp
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

#include "ControllerFactory.h"

#include "AckController.h"
#include "BandwidthSensitiveController.h"
#include "DefaultDataCacheExpirationController.h"
#include "DefaultDataCacheReplicationController.h"
#include "DefaultForwardingController.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "TargetBasedReplicationController.h"
#include "PullReplicationController.h"
#include "PushReplicationController.h"
#include "PushToConvoyDataCacheReplicationController.h"
#include "WorldStateForwardingController.h"
#include "SubscriptionForwardingController.h"
#include "TopologyForwardingController.h"

#include "ConfigManager.h"
#include "FTypes.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

static ConfigManager *pCfgMgr = NULL;
static DisseminationService *pDisseminationService = NULL;

void ControllerFactory::init (DisseminationService *pDisService, ConfigManager *pConfigManager)
{
    pCfgMgr = pConfigManager;
    pDisseminationService =  pDisService;
}

AckController * ControllerFactory::getAckControllerAndRegisterListeners()
{
    const char *pszMethodName = "ControllerFactory::getAckControllerAndRegisterListeners";

    uint32 ui32TimeOut = pCfgMgr->hasValue ("aci.disService.replication.ack.timeout") ? pCfgMgr->getValueAsUInt32 ("aci.disService.replication.ack.timeout")
                                                                                      : AckController::DEFAULT_RETRANSMISSION_TIMEOUT;
    uint8 ui8TransmissionWindow = AckController::DEFAULT_TRANSMISSION_WINDOW;

    AckController *pCtrler = new AckController (pDisseminationService, ui32TimeOut, ui8TransmissionWindow);

    if (pCtrler == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Could not instantiate controller.  Quitting.\n");
        exit (-1);
    }

    unsigned int uiIndex;
    if (pDisseminationService->registerMessageListener (pCtrler, uiIndex) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "MessageListener");
        exit (-1);
    }
    if (pDisseminationService->registerPeerStateListener (pCtrler, uiIndex) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "PeerStateListener");
        exit (-1);
    }
    
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "Acknowledgment enabled with timeout %u and ui8TransmissionWindow %d.\n",
                    ui32TimeOut, ui8TransmissionWindow);
    return pCtrler;
}

DataCacheExpirationController * ControllerFactory::getExpControllerAndRegisterListeners()
{
    const char *pszMethodName = "ControllerFactory::getDataCacheExpirationController";

    DataCacheExpirationController *pCtrler = new DefaultDataCacheExpirationController (pDisseminationService);

    if (pCtrler == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Could not instantiate controller.  Quitting.\n");
        exit (-1);
    }

    unsigned int uiIndex;
    if (pDisseminationService->registerDataCacheListener (pCtrler, uiIndex) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "DataCacheListener");
        exit (-1);
    }

    return pCtrler;
}

DataCacheReplicationController * ControllerFactory::getRepControllerAndRegisterListeners()
{
    const char *pszMethodName = "ControllerFactory::getDataCacheReplicationController";

    bool bAck = pCfgMgr->getValueAsBool ("aci.disService.replication.ack", false);

    DataCacheReplicationController *pCtrler;
    String value;
    if (pCfgMgr->hasValue ("aci.disService.replication.mode")) {
        value = pCfgMgr->getValue ("aci.disService.replication.mode");
        if (value == "PULL") {
             pCtrler = bAck ? new PullReplicationController (pDisseminationService, bAck) :
                              new PullReplicationController (pDisseminationService);
        }
        else if (value == "PUSH") {
            pCtrler = bAck ? new PushReplicationController (pDisseminationService, bAck) :
                             new PushReplicationController (pDisseminationService);
        }
        else if (value == "PUSH_CONVOY") {
            pCtrler = bAck ? new PushToConvoyDataCacheReplicationController (pDisseminationService, bAck) :
                             new PushToConvoyDataCacheReplicationController (pDisseminationService);
        }
        else if (value == "BANDWIDTH_SENSITIVE") {
            pCtrler = bAck ? new BandwidthSensitiveController (pDisseminationService, bAck) :
                             new BandwidthSensitiveController (pDisseminationService);
            unsigned int index;
            if (pDisseminationService->registerNetworkStateListener ((BandwidthSensitiveController*)pCtrler, index) < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Could not register controller.  Quitting.\n");
                exit (-1);
            }
            
        }
        else if (value == "TargetBased") {
            pCtrler = new TargetBasedReplicationController (pDisseminationService, pCfgMgr);
        }
        else {
            pCtrler = bAck ? new DefaultDataCacheReplicationController (pDisseminationService, bAck) :
                             new DefaultDataCacheReplicationController (pDisseminationService);
        } 
    }
    else {
        pCtrler = bAck ? new DefaultDataCacheReplicationController (pDisseminationService, bAck) :
                         new DefaultDataCacheReplicationController (pDisseminationService);
    }

    if (pCtrler == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Could not instantiate controller.  Quitting.\n");
        exit (-1);
    }

    unsigned int index;
    if (pDisseminationService->registerDataCacheListener (pCtrler, index) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "DataCacheListener");
        exit (-1);
    }
    if (pDisseminationService->registerMessageListener (pCtrler, index) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "MessageListener");
        exit (-1);
    }
    if (pDisseminationService->registerPeerStateListener (pCtrler, index) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "PeerStateListener");
        exit (-1);
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "%s data cache replication controller loaded.\n",
                    ((const char *) value ? (const char *) value : "DEFAULT"));

    return pCtrler;
}

ForwardingController * ControllerFactory::getForwControllerAndRegisterListeners()
{
    const char *pszMethodName = "ControllerFactory::getForwardingControllerInstance";

    ForwardingController *pCtrler = NULL;
    int DEF_FORWARDING_CTRL = 300;   // Any number that ensures that the default
                                     // case will be selected
    switch (pCfgMgr->getValueAsInt ("aci.disService.forwarding.mode", DEF_FORWARDING_CTRL)) {
        case 0: {
            pCtrler = new WorldStateForwardingController (pDisseminationService);
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "Loading WorldStateForwardingController\n");
            break;
        }

        case 1: {
            float fProb = (float)(pCfgMgr->getValueAsInt ("aci.disService.forwarding.probability", 50));
            pCtrler = new DefaultForwardingController (pDisseminationService, fProb);
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Loading DefaultForwardingController "
                            "(purely probabilistic) with probability of %.2f\n", fProb);
            break;
        }

        case 3: {
            pCtrler = new TopologyForwardingController (pDisseminationService);
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "Loading TopologyForwardingController\n");
            break;
        }

        default: {
            pCtrler = new DefaultForwardingController (pDisseminationService);
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "loaded the stateful forwarding controller\n");
            break;
        }
    }

    if (pCtrler == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Could not instantiate controller.  Quitting.\n");
        exit (-1);
    }

    unsigned int index;
    if (pDisseminationService->registerMessageListener (pCtrler, index) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "MessageListener");
        exit (-1);
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "registered the message listener\n");

    if (pDisseminationService->registerPeerStateListener (pCtrler, index) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "PeerStateListener");
        exit (-1);
    }

    return pCtrler;
}

SubscriptionForwardingController * ControllerFactory::getSubForwControllerAndRegisterListeners()
{
    const char *pszMethodName = "ControllerFactory::SubscriptionForwardingControllerInstance";
    
    SubscriptionForwardingController *pCtrler = NULL;
    String sValue = pCfgMgr->getValue ("aci.disService.forwarding.subscriptionForwarding");
    
    if (sValue != NULL && sValue == "ON") {    
        pCtrler = new SubscriptionForwardingController (pDisseminationService);
        if (pCtrler == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Could not instantiate Subscription Forwarding Controller.  Quitting.\n");
            exit (-1);
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "loaded the Subscription Forwarding Controller\n");

        unsigned int index;
        if (pDisseminationService->registerMessageListener (pCtrler, index) < 0) {
            checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "MessageListener");
            exit (-1); 
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "registered the message listener\n");

        if (pDisseminationService->registerPeerStateListener (pCtrler, index) < 0) {
            checkAndLogMsg (pszMethodName, listerRegistrationFailed, "pCtrler", "PeerStateListener");
            exit (-1);
        }
        
        if (pCfgMgr->hasValue ("aci.disService.forwarding.subscriptionForwarding.advThreshold")) {
            uint16 ui16AdvThreshold = (uint16) pCfgMgr->getValueAsInt ("aci.disService.forwarding.subscriptionForwarding.advThreshold");
            pCtrler->setAdvertisementThreshold (ui16AdvThreshold);
        }
        
        if (pCfgMgr->hasValue ("aci.disService.forwarding.subscriptionForwarding.advPeriod")) {
            int64 i64AdvPeriod = (int64) pCfgMgr->getValueAsInt64 ("aci.disService.forwarding.subscriptionForwarding.advPeriod", 5000);
            pCtrler->setAdvertisementPeriod (i64AdvPeriod);
        }
        
        int64 i64LifeTimeUpdateTh = (int64) pCfgMgr->getValueAsInt64 ("aci.disService.forwarding.subscriptionForwarding.lifetimeUpdateThreshold", 60000);
        int64 i64SeqNoLifetimeExpirationThreshold = (int64) pCfgMgr->getValueAsInt64 ("aci.disService.forwarding.subscriptionForwarding.seqNumLifetimeExpirationThreshold", 600000);
        pCtrler->configureTable (i64LifeTimeUpdateTh, i64SeqNoLifetimeExpirationThreshold);
        
    }
    
    return pCtrler;
}

