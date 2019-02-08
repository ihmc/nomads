/*
 * ConfigFileReader.cpp
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

#include "ConfigFileReader.h"

#include "DisseminationService.h"
#include "MessageReassembler.h"
#include "PeerState.h"
#include "TransmissionService.h"

#include "ConfigManager.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"
#include "NICInfo.h"
#include "StringTokenizer.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace IHMC_ACI;

uint8 intToUI8 (const char *pszMethodName, int iValue);

ConfigFileReader::ConfigFileReader (NOMADSUtil::ConfigManager *pCfgMgr)
{
    _pCfgMgr = pCfgMgr;
}

ConfigFileReader::~ConfigFileReader (void)
{
    _pCfgMgr = NULL;
}

void ConfigFileReader::init (ConfigManager *pCfgMgr)
{
    _pCfgMgr = pCfgMgr;
}

char ** ConfigFileReader::getNetIFs (void)
{
    char **ppIfaces = parseNetIFs (_pCfgMgr->getValue ("aci.disService.netIFs"));
    if (ppIfaces == NULL) {
        checkAndLogMsg ("ConfigFileReader::getNetIFs", Logger::L_Info,
                        "no network interface set; all the available interfaces will be used\n");
    }
    return ppIfaces;
}

char ** ConfigFileReader::getIgnoredNetIFs (void)
{
    char **ppIfaces = parseNetIFs (_pCfgMgr->getValue ("aci.disService.netIFs.ignore"));
    if (ppIfaces == NULL) {
        checkAndLogMsg ("ConfigFileReader::getIgnoredNetIFs", Logger::L_Info,
                        "no network interfaces specified to be ignored\n");
    }
    return ppIfaces;
}

char ** ConfigFileReader::getAddedNetIFs (void)
{
    char **ppIfaces = parseNetIFs (_pCfgMgr->getValue ("aci.disService.netIFs.add"));
    if (ppIfaces == NULL) {
        checkAndLogMsg ("ConfigFileReader::getAddedNetIFs", Logger::L_Info,
                        "no network interface specified to be explictly added\n");
    }
    return ppIfaces;
}

uint8 ConfigFileReader::getTransmissionMode (void)
{
    const char *pszValue = _pCfgMgr->getValue ("aci.disService.transmission");
    uint8 ui8Ret = DisseminationService::PUSH_ENABLED;
    if (pszValue) {
        String value = pszValue;
        if (value == "PUSH_ENABLED") {
            ui8Ret = DisseminationService::PUSH_ENABLED;
        }
        else if (value == "PUSH_DISABLED") {
            ui8Ret = DisseminationService::PUSH_DISABLED;
        }
        else {
            checkAndLogMsg ("ConfigFileReader::getTransmissionMode", Logger::L_Warning,
                            "Transmission mode Undefined. Assuming PUSH_ENABLED\n");
            ui8Ret = DisseminationService::PUSH_ENABLED;
        }
    }
    checkAndLogMsg ("ConfigFileReader::getTransmissionMode", Logger::L_Info,
                    "%d transmission mode loaded.\n" , ui8Ret);
    return ui8Ret;
}

uint16 ConfigFileReader::getKeepAliveInterval()
{
    const char *pszPropertyName = "aci.disService.worldstate.keepAlive.interval";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        uint16 ui16Ret = (uint16) _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getKeepAliveInterval", Logger::L_Info,
                        "%u keep alive interval set.\n" , ui16Ret);
        return ui16Ret;
    }
    checkAndLogMsg ("ConfigFileReader::getKeepAliveInterval", Logger::L_Info,
                    "default (%u) keep alive interval set.\n" ,
                    DisseminationService::DEFAULT_KEEP_ALIVE_INTERVAL);
    return DisseminationService::DEFAULT_KEEP_ALIVE_INTERVAL;
}

uint32 ConfigFileReader::getDeadPeerInterval()
{
    const char *pszPropertyName = "aci.disService.worldstate.deadPeer.interval";
    if (_pCfgMgr->hasValue(pszPropertyName)) {
        uint32 ui32Ret = (uint32) _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getDeadPeerInterval", Logger::L_Info,
                        "%lu dead peer interval set.\n", ui32Ret);
        return ui32Ret;
    }
    checkAndLogMsg ("ConfigFileReader::getDeadPeerInterval", Logger::L_Info,
                    "default (%lu) dead peer interval set.\n" ,
                    DisseminationService::DEFAULT_DEAD_PEER_INTERVAL);
    return DisseminationService::DEFAULT_DEAD_PEER_INTERVAL;
}

ConfigFileReader::FragReqGenMod ConfigFileReader::getMissingFragReqProbabilityMode (uint8 ui8Probability)
{
    const char *pszMethodName = "ConfigFileReader::getMissingFragReqProbabilityMode";
    const char *pszPropertyName = "aci.disService.fragReqGenProb.mode";
    String fixed = "FIXED";
    String priorityDependent = "PRIORITY_DEPENDENT";
    String value = _pCfgMgr->getValue(pszPropertyName);
    if (value == fixed) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "MissingFragReqProbabilityMode set to FIXED.");
        return FIXED;
    }
    else if (value == priorityDependent) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "MissingFragReqProbabilityMode set to PRIORITY_DEPENDENT.");
        return PRIORITY_DEPENDENT;
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "MissingFragReqProbabilityMode set to default value: FIXED.");
        return FIXED;
    }
}

float ConfigFileReader::getMissingFragReqProbability()
{
    const char *pszMethodName = "ConfigFileReader::getMissingFragReqProbability";
    const char *pszPropertyName = "aci.disService.fragReqGenProb.priority";
    const char *pszProbValue = _pCfgMgr->getValue (pszPropertyName);
    if (pszProbValue) {
        float fRet = (float)atof (pszProbValue);
        if ((fRet >= 0) && (fRet <= 100)) {
                return fRet;
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "The value of"
                            " probability is out of bound (%f). Setting default"
                            "value %f", fRet, MessageReassembler::DEFAULT_RANGE_REQUEST_PROB);
            exit(-1);
        }
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "The value of "
                        "probability is unset. Setting default value %f",
                        MessageReassembler::DEFAULT_RANGE_REQUEST_PROB);
        exit(-1);
    }
    return (float)MessageReassembler::DEFAULT_RANGE_REQUEST_PROB;
}

float ConfigFileReader::getMissingFragReqProbabilityByPriority (uint8 ui8Probability)
{
    const char *pszMethodName = "ConfigFileReader::getMissingFragReqProbabilityByPriority";
    char *pszProbability = new char[12];
    pszProbability = itoa (pszProbability, ui8Probability);
    if (pszProbability) {
        // Create the property name
        String propertyName = "aci.disService.fragReqGenProb.priorityValue.";
        propertyName += (String) pszProbability;
        delete pszProbability;
        // Retrieve the value associated for the property name
        const char *pszProbValue = _pCfgMgr->getValue (propertyName);
        if (pszProbValue) {
            float fRet = (float)atof (pszProbValue);
            if ((fRet >= 0) && (fRet <= 100)) {
                return fRet;
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError, "The value "
                                "of probability for propery %s is out of bound (%f).",
                                (const char *) propertyName, fRet);
            }
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "The value of"
                            "probability for propery %s is unset",
                            (const char *) propertyName);
        }
        exit (-1);
    }
    return -1;
}

uint16 ConfigFileReader::getIgnoreRequestInterval()
{
    const char *pszMethodName = "ConfigFileReader::getIgnoreRequestInterval";
    const char *pszPropertyName = "aci.disService.transmission.ignoreReqTime";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%lu ignore request time "
                        "set.\n", _pCfgMgr->getValueAsInt(pszPropertyName));
        return (uint16) _pCfgMgr->getValueAsInt (pszPropertyName);
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Default (%lu) ignore request "
                    "time set.\n" , DisseminationService::DEFAULT_IGNORE_REQUEST_TIME);
    return (uint16) DisseminationService::DEFAULT_IGNORE_REQUEST_TIME;
}

int64 ConfigFileReader::getConnectivityHistoryWindowSize()
{
    const char *pszMethodName = "ConfigFileReader::getConnectivityHistoryWindowSize";
    const char *pszPropertyName = "aci.disService.nodeConfiguration.connectivityHistory.windowSize";
    if (_pCfgMgr->hasValue(pszPropertyName)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%lu connectivity history "
                        "window size.\n", _pCfgMgr->getValueAsInt(pszPropertyName));
        return (int64) _pCfgMgr->getValueAsInt (pszPropertyName);
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Default (%lu) connectivity "
                    "history window size set.\n" ,
                    DisseminationService::DEFAULT_CONNECTIVITY_HISTORY_WIN_SIZE);
    return DisseminationService::DEFAULT_CONNECTIVITY_HISTORY_WIN_SIZE;
}

bool ConfigFileReader::getQueryDataCacheEnabled()
{
    const char *pszPropertyName = "aci.disService.antientropy.queryDataCache";
    bool bEnabled;
    if (!_pCfgMgr->hasValue(pszPropertyName)) {
        bEnabled = false;
    }
    else {
        bEnabled = _pCfgMgr->getValueAsBool (pszPropertyName);
    }
    checkAndLogMsg ("ConfigFileReader::getQueryDataCacheEnabled", Logger::L_Info,
                    "%s.\n" , bEnabled ? "enabled" : "disabled");
    return bEnabled;
}

uint8 ConfigFileReader::getQueryDataCacheReply()
{
    const char *pszPropertyName = "aci.disService.antientropy.replyQueryDataCache";
    uint8 ui8Ret = 0;
    if (_pCfgMgr->hasValue(pszPropertyName)) {
        ui8Ret = _pCfgMgr->getValueAsInt(pszPropertyName);
    }
    if (ui8Ret == 1 || ui8Ret == 2) {
        checkAndLogMsg ("ConfigFileReader::getQueryDataCacheReply", Logger::L_Info,
                        "%s.\n", ui8Ret == 1 ? "REPLY_DATA" : "REPLY_MSGID");
        return ui8Ret;
    }
    checkAndLogMsg ("ConfigFileReader::getQueryDataCacheReply",
                    Logger::L_Warning, "DISABLED by default.\n");
    return DisseminationService::DISABLED;
}

bool ConfigFileReader::getSubscriptionStateExchangeEnabled()
{
    const char *pszPropertyName = "aci.disService.worldstate.subscriptionexchange.enabled";
    bool bEnabled;
    if (_pCfgMgr->hasValue(pszPropertyName)) {
        bEnabled = _pCfgMgr->getValueAsBool(pszPropertyName);
    }
    else {
        bEnabled = DisseminationService::DEFAULT_SUBSCRIPTION_STATE_EXCHANGE_ENABLED;
    }
    checkAndLogMsg ("ConfigFileReader::getSubscriptionStateExchangeEnabled",
                    Logger::L_Info, "%s.\n" , bEnabled ? "enabled" : "disabled");
    return bEnabled;
}

uint16 ConfigFileReader::getSubscriptionStatePeriod()
{
    const char *pszPropertyName = "aci.disService.worldstate.subscriptionexchange.period";
    uint32 ui32Ret = 0;
    if (_pCfgMgr->getValueAsBool("aci.disService.worldstate.subscriptionexchange.enabled")) {
        ui32Ret = _pCfgMgr->getValueAsInt (pszPropertyName,
                                               DisseminationService::DEFAULT_SUBSCRIPTION_STATE_PERIOD);
        checkAndLogMsg ("ConfigFileReader::getSubscriptionStatePeriod",
                        Logger::L_Info, "%d.\n" , ui32Ret);
    }
    return ui32Ret;
}

bool ConfigFileReader::getKeepAliveMsgEnabled()
{
    const char *pszPropertyName = "aci.disService.worldstate.keepAlive.enabled";
    bool bEnabled = _pCfgMgr->getValueAsBool (pszPropertyName, true);
    checkAndLogMsg ("ConfigFileReader::getSendHelloMsgEnabled", Logger::L_Info,
                    "%s.\n" , bEnabled ? "enabled" : "disabled");
    return bEnabled;
}

uint8 ConfigFileReader::getMemorySpace()
{
    const char *pszPropertyName = "aci.disService.nodeConfiguration.memorySpace";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        uint32 ui32MemorySpace = _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getMemorySpace", Logger::L_Info,
                        "Memory space available set to <%d>.\n", ui32MemorySpace);
        if (ui32MemorySpace < DisseminationService::DEFAULT_MAX_MEMORY_SPACE) {
            return (uint8) floor ((float)((255 * ui32MemorySpace) / DisseminationService::DEFAULT_MAX_MEMORY_SPACE) / 51);
        }
    }
    checkAndLogMsg ("ConfigFileReader::getMemorySpace", Logger::L_Info,
                    "Memory space available set to <%d> KB.\n" ,
                    DisseminationService::DEFAULT_MAX_MEMORY_SPACE);
    return PeerState::VERY_HIGH;
}

uint32 ConfigFileReader::getBandwidth()
{
    uint32 ui32Bandwidth = _pCfgMgr->getValueAsInt ("aci.disService.nodeConfiguration.bandwidth", 0);
    checkAndLogMsg ("ConfigFileReader::getBandwidth", Logger::L_Info,
                    "Bandwidth available set to <%d> Kbps. (0 means unlimited)\n",
                    ui32Bandwidth);
    return ui32Bandwidth;
}

uint8 ConfigFileReader::getBandwidthIndex (uint32 ui32Bandwidth)
{
    if (ui32Bandwidth > 0 && ui32Bandwidth < DisseminationService::DEFAULT_MAX_BANDWIDTH) {
        // Scale the value of bandwidth to be within the interval [0, 255]
        float fBandwidth = (float) (ui32Bandwidth - DisseminationService::DEFAULT_MIN_BANDWIDTH);
        float fMaxBwidth = DisseminationService::DEFAULT_MAX_BANDWIDTH -
                           DisseminationService::DEFAULT_MIN_BANDWIDTH;
        float fIndex = fBandwidth * 255 / fMaxBwidth;
        // Divide the range in 5 uniform categories, and return
        // the category index
        if (fIndex < (float) 255/5) {
            return PeerState::VERY_LOW;
        }
        else if (fIndex < (float) 255/4) {
            return PeerState::LOW;
        }
        else if (fIndex < (float) 255/3) {
            return PeerState::NORMAL;
        }
        else if (fIndex < (float) 255/2) {
            return PeerState::HIGH;
        }
    }
    return PeerState::VERY_HIGH;
}

uint8 ConfigFileReader::getPolicyID()
{
    const char *pszMethodName = "ConfigFileReader::getPolicyID";
    uint8 ui8RetValue = _pCfgMgr->getValueAsInt (pszMethodName);
    switch (ui8RetValue)
    {
        case 1 :
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Target Replication "
                            "Policy set to ClassifiedTargetReplicationPolicy.\n");
            return ui8RetValue;
        case 2 :
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Target Replication "
                            "Policy set to FirstClassTargetReplicationPolicy.\n");
            return ui8RetValue;
        default :
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Target Replication "
                            "Policy set to RandomizedTargetReplicationPolicy.\n");
            return 0;
    }
}

uint32 ConfigFileReader::getChunkSize()
{
    const char *pszPropertyName = "aci.disService.chunkReplicationPolicy.chunkSize";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        uint32 ui32Ret = (uint32) _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getChunkSize", Logger::L_Info,
                        "Chunk size set to <%d>.\n" , ui32Ret);
        return ui32Ret;
    }
    return (uint32) DisseminationService::DEFAULT_CHUNK_SIZE;
}

uint8 ConfigFileReader::getMemoryThreshold()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.memoryThreshold";
    uint16 ui16RetValue;
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue = _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getMemoryThreshold", Logger::L_Info,
                        "Memory threshold set to <%d>.\n" , iValue);
        ui16RetValue = (uint16) (iValue * 255 / DisseminationService::DEFAULT_MAX_MEMORY_SPACE);
    } else {
        checkAndLogMsg ("ConfigFileReader::getMemoryThreshold", Logger::L_Info,
                        "Memory threshold set to <%d> KB.\n" ,
                        DisseminationService::DEFAULT_MIN_FREE_MEMORY_THRESHOLD);
        ui16RetValue = (uint16) (DisseminationService::DEFAULT_MIN_FREE_MEMORY_THRESHOLD * 255 / DisseminationService::DEFAULT_MAX_MEMORY_SPACE);
    }
    if (ui16RetValue >= 255) {
        return (uint8) 255;
    }
    return (uint8) ui16RetValue;
}

uint8 ConfigFileReader::getBandwidthThreshold()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.bandwidthThreshold";
    uint16 ui16RetValue;
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue = _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getBandwidthThreshold", Logger::L_Info,
                        "Bandwidth threshold set to <%d>.\n", iValue);
        ui16RetValue = (uint16) (iValue * 255 / DisseminationService::DEFAULT_MAX_BANDWIDTH);
    } else {
        checkAndLogMsg ("ConfigFileReader::getBandwidthThreshold", Logger::L_Info,
                        "Bandwidth threshold set to <%d> KB.\n" ,
                        DisseminationService::DEFAULT_MIN_BANDWIDTH_THRESHOLD);
        ui16RetValue = (uint16) (DisseminationService::DEFAULT_MIN_BANDWIDTH_THRESHOLD * 255 / DisseminationService::DEFAULT_MAX_BANDWIDTH);
    }
    if (ui16RetValue >= 255) {
        return (uint8) 255;
    }
    return (uint8) ui16RetValue;
}

uint8 ConfigFileReader::getBandwidthFactor()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.bandwidthFactor";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue = _pCfgMgr->getValueAsInt(pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getBandwidthFactor", Logger::L_Info,
                        "Bandwidth factor set to <%d>.\n", iValue);
        return intToUI8 ("getBandwidthFactor", iValue);
    }
    return DisseminationService::DEFAULT_BANDWIDTH_FACTOR;
}

uint8 ConfigFileReader::getMemoryFactor()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.memoryFactor";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue = _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getMemoryFactor", Logger::L_Info,
                        "Memory factor set to <%d>.\n", iValue);
        return intToUI8 ("getMemoryFactor", iValue);
    }
    return DisseminationService::DEFAULT_MEMORY_FACTOR;
}

uint8 ConfigFileReader::getNumOfClasses()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.numOfClasses";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue = _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getNumOfClasses", Logger::L_Info,
                        "numOfClasses factor set to <%d>.\n" , iValue);
        return intToUI8 ("getNumOfClasses", iValue);
    }
    return DisseminationService::DEFAULT_NUM_OF_CLASSES;
}

uint8 ConfigFileReader::getActiveNeighborsFactor()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.activeNeighborsFactor";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue =  _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getActiveNeighborsFactor", Logger::L_Info,
                        "Active neighbors factor set to <%d>.\n", iValue);
        return intToUI8 ("getActiveNeighborsFactor", iValue);
    }
    return DisseminationService::DEFAULT_ACTIVES_NEIGHBORS_FACTOR;
}

uint8 ConfigFileReader::getNodesInConnectivityHistoryFactor()
{
    const char *pszPropertyName = "aci.disService.targetReplicationPolicy.nodesInConnectivityHistoryFactor";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        int iValue = _pCfgMgr->getValueAsInt (pszPropertyName);
        checkAndLogMsg ("ConfigFileReader::getNodesInConnectivityHistoryFactor",
                        Logger::L_Info, "Nodes in connectivity history factor "
                        "set to <%d>.\n", iValue);
        return intToUI8 ("getNodesInConnectivityHistoryFactor", iValue);
    }
    return DisseminationService::DEFAULT_NODES_IN_CONNECTIVITY_HISTORY_FACTOR;
}

bool ConfigFileReader::isTargetIDFilteringEnabled()
{
    const char *pszPropertyName = "aci.disService.propagation.targetFiltering";
    if (_pCfgMgr->hasValue(pszPropertyName)) {
        return _pCfgMgr->getValueAsBool (pszPropertyName);
    }
    return false;
}

////////////////////////////// WORLD STATE /////////////////////////////////////

bool ConfigFileReader::isDataCacheStateEnabled (void)
{
    return true;
}

bool ConfigFileReader::isSubscriptionStateEnabled (void)
{
    return true;
}

bool ConfigFileReader::isTopologyStateEnabled (void)
{
    return true;
}


////////////////////////////// UTILITY METHODS /////////////////////////////////

uint8 intToUI8 (const char *pszMethodName, int iValue)
{
    String methodName = "ConfigFileReader::";
    methodName += (String) pszMethodName;
    if (iValue < 0) {
        checkAndLogMsg ((const char *) methodName, Logger::L_Warning,
                        "Value %d less than the minimum value allowed. Setting "
                        "it to 0", iValue);
        iValue = 0;
    }
    else if (iValue > 255) {
        checkAndLogMsg ((const char *) methodName, Logger::L_Warning,
                        "Value %d greater than the maximum value allowed. "
                        "Setting it to 255", iValue);
        iValue = 255;
    }
    return (uint8) iValue;
}

char ** ConfigFileReader::parseNetIFs (const char *pszPropertyValue)
{
    if ((pszPropertyValue == NULL) || (strlen (pszPropertyValue) == 0)) {
        return NULL;
    }

    const char *pszMethodName = "ConfigFileReader::parseNetIFs";
    String value (pszPropertyValue);
    char **ppRet = NULL;
    if (value.indexOf (";") > -1) {
        int i = 0;
        DArray<char*> interfaces;
        // There is more than 1 interfaces to be set
        StringTokenizer tokenizer ((const char *)value, ';', ';');
        for (const char *pszInterface = tokenizer.getNextToken(); pszInterface;
             pszInterface = tokenizer.getNextToken()) {
            interfaces[i] = (char *)pszInterface;
            i++;
        }
        ppRet = (char**) calloc (i+1 , sizeof(char*));
        for (int j = 0; j < i; j++) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "interface %s (%u) parsed\n",
                            interfaces[j], inet_addr (interfaces[j]));
            ppRet[j] = strDup (interfaces[j]);
        }
    }
    else {
        // There is 1 interface to be set
        char *pszInterface = strDup ((char *)value);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "interface %s (%u) parsed\n",
                        pszInterface, inet_addr (pszInterface));
        ppRet = (char**) calloc (2 , sizeof(char*));
        ppRet[0] = pszInterface;
        return ppRet;
    }
    return ppRet;
}

////////////////////////////// TOPOLOGY /////////////////////////////////

bool ConfigFileReader::getSubscriptionsExchangeEnabled (void)
{
    const char *pszPropertyName = "aci.disService.worldstate.subscriptionsExchange.enabled";
    bool bEnabled = _pCfgMgr->getValueAsBool (pszPropertyName,
                                              DisseminationService::DEFAULT_SUBSCRIPTIONS_EXCHANGE_ENABLED);
    checkAndLogMsg ("ConfigFileReader::getSubscriptionsExchangeEnabled", Logger::L_Info,
                    "Subscriptions exchange %s.\n" , bEnabled ? "enabled" : "disabled");
    return bEnabled;
}

uint32 ConfigFileReader::getSubscriptionsExchangePeriod (void)
{
    const char *pszPropertyName = "aci.disService.worldstate.subscriptionsExchange.period";
    uint32 ui32Ret = 0U;
    if (getSubscriptionsExchangeEnabled()) {
        if (_pCfgMgr->hasValue (pszPropertyName)) {
            ui32Ret = _pCfgMgr->getValueAsUInt32 (pszPropertyName);
        }
        else {
            ui32Ret = DisseminationService::DEFAULT_SUBSCRIPTIONS_EXCHANGE_PERIOD;
        }
        checkAndLogMsg ("ConfigFileReader::getSubscriptionsExchangePeriod", Logger::L_Info,
                        "Subscriptions exchange period set to <%d>.\n" , ui32Ret);
    }
    return ui32Ret;
}

bool ConfigFileReader::getTopologyExchangeEnabled (void)
{
    const char *pszPropertyName = "aci.disService.worldstate.topologyExchange.enabled";
    bool bEnabled = _pCfgMgr->getValueAsBool (pszPropertyName,
                                              DisseminationService::DEFAULT_TOPOLOGY_EXCHANGE_ENABLED);
    checkAndLogMsg ("ConfigFileReader::getTopologyExchangeEnabled", Logger::L_Info,
                    "Topology exchange %s.\n" , bEnabled ? "enabled" : "disabled");
    return bEnabled;
}

uint32 ConfigFileReader::getTopologyExchangePeriod (void)
{
    const char *pszPropertyName = "aci.disService.worldstate.topologyExchange.period";
    uint32 ui32Ret = 0U;
    if (getTopologyExchangeEnabled()) {
        if (_pCfgMgr->hasValue (pszPropertyName)) {
            ui32Ret = _pCfgMgr->getValueAsUInt32 (pszPropertyName);
        } else {
            ui32Ret = DisseminationService::DEFAULT_TOPOLOGY_EXCHANGE_PERIOD;
        }
        checkAndLogMsg ("ConfigFileReader::getTopologyExchangePeriod", Logger::L_Info,
                        "Topology exchange period set to <%d>.\n" , ui32Ret);
    }
    return ui32Ret;
}

float ConfigFileReader::getProbContact (void)
{
    const char *pszPropertyName = "aci.disService.topology.prob_contact";
    float fRet;
    if (getTopologyExchangeEnabled()) {
        if (_pCfgMgr->hasValue (pszPropertyName)) {
            fRet = _pCfgMgr->getValueAsFloat (pszPropertyName);
        } else {
            fRet = DisseminationService::DEFAULT_PROB_CONTACT;
        }
        checkAndLogMsg ("ConfigFileReader::getProbContact", Logger::L_Info,
                        "Prob contact set to <%f>.\n", fRet);
    }
    return fRet;
}

float ConfigFileReader::getProbThreshold (void)
{
    const char *pszPropertyName = "aci.disService.topology.prob_threshold";
    float fRet;
    if (getTopologyExchangeEnabled()) {
        if (_pCfgMgr->hasValue (pszPropertyName)) {
            fRet = _pCfgMgr->getValueAsFloat (pszPropertyName);
        } else {
            fRet = DisseminationService::DEFAULT_PROB_THRESHOLD;
        }
        checkAndLogMsg ("ConfigFileReader::getProbThreshold", Logger::L_Info,
                        "Prob threshold set to <%f>.\n", fRet);
    }
    return fRet;
}

float ConfigFileReader::getAddParam (void)
{
    const char *pszPropertyName = "aci.disService.topology.add_param";
    float fRet;
    if (getTopologyExchangeEnabled()) {
        if (_pCfgMgr->hasValue (pszPropertyName)) {
            fRet = _pCfgMgr->getValueAsFloat (pszPropertyName);
        } else {
            fRet = DisseminationService::DEFAULT_ADD_PARAM;
        }
        checkAndLogMsg ("ConfigFileReader::getAddParam", Logger::L_Info,
                        "Add param set to <%f>.\n", fRet);
    }
    return fRet;
}

float ConfigFileReader::getAgeParam (void)
{
    const char *pszPropertyName = "aci.disService.topology.age_param";
    float fRet;
    if (getTopologyExchangeEnabled()) {
        if (_pCfgMgr->hasValue (pszPropertyName)) {
            fRet = _pCfgMgr->getValueAsFloat (pszPropertyName);
        } else {
            fRet = DisseminationService::DEFAULT_AGE_PARAM;
        }
        checkAndLogMsg ("ConfigFileReader::getAgeParam", Logger::L_Info,
                        "Age param set to <%f>.\n", fRet);
    }
    return fRet;
}
