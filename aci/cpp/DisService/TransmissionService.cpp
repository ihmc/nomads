/*
 * TransmissionService.cpp
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

#include "TransmissionService.h"

#include "DisServiceDefs.h"
#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "Message.h"

#include "NetworkMessageService.h"
#include "NetworkMessageServiceProxy.h"

#include "SessionId.h"
#include "TransmissionServiceListener.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "InstrumentedWriter.h"
#include "NLFLib.h"
#include "NullWriter.h"
#include "StrClass.h"
#include "DisServiceMsgHelper.h"

#include <time.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * TransmissionService::DEFAULT_DISSERVICE_MCAST_GROUP  = "239.0.0.239";
const uint16  TransmissionService::DEFAULT_DISSERVICE_MCAST_PORT = 6666;
const uint8 TransmissionService::DEFAULT_DISSERVICE_TTL = 32;
const char * TransmissionService::DEFAULT_DISSERVICE_BCAST_ADDR = "255.255.255.255";
const uint16 TransmissionService::DEFAULT_DISSERVICE_BCAST_PORT = 6666;
const bool TransmissionService::DEFAULT_ASYNCHRONOUS_DELIVERY = true;
const bool TransmissionService::DEFAULT_ASYNCHRONOUS_TRANSMISSION = false;
const uint32 TransmissionService::DEFAULT_TRANSMIT_RATE_LIMIT = 0;   // 0, means no limit!
const uint8 TransmissionService::DEFAULT_MESSAGE_VERSION = 1;

void deallocateStringsArray (char **ppszStrings)
{
    if (ppszStrings) {
        for (int i = 0; ppszStrings[i] != nullptr; i++) {
            free (ppszStrings[i]);
        }
        free (ppszStrings);
    }
}

TransmissionService::TransmissionService (TRANSMISSION_SVC_MODE mode, uint16 ui16NetworkMessageServicePort,
                                          const char *pszMcastGroup, uint8 ui8McastTTL,
                                          const char *pszNodeId, bool bAsyncDelivery, bool bAsyncTransmission,
                                          uint8 ui8MessageVersion)
    : _mode (mode),
      _bAsyncDelivery (bAsyncDelivery),
      _bAsyncTransmission (bAsyncTransmission),
      _ui8McastTTL (ui8McastTTL),
      _ui8MessageVersion (ui8MessageVersion),
      _ui16Port (ui16NetworkMessageServicePort),
      _ui16MaxFragmentSize (1400), // in bytes
      _ui32RateLimitCap (0),
      _pMPS (nullptr),
      _dstAddr (pszMcastGroup),
      _nodeId (pszNodeId)
{
}

TransmissionService::~TransmissionService (void)
{
    _nmsHelper.requestTerminationAndWait();
    delete _pMPS;
    _pMPS = nullptr;
}

TransmissionService * TransmissionService::getInstance (ConfigManager *pCfgMgr, const char *pszNodeId)
{
    const char *pszMethodName = "TransmissionService::getInstance";

    TransmissionServiceConfReader cfgReader (pCfgMgr);
    const TRANSMISSION_SVC_MODE mode = cfgReader.getMode();
    const uint16 ui16Port = cfgReader.getPort();
    uint8 ui8McastTTL = 1;

    const bool bAsyncDelivery = cfgReader.getAsyncDelivery();
    const bool bAsyncTransmission = cfgReader.getAsyncTransmission();
    const uint8 ui8MessageVersion = cfgReader.getNetworkMessageVersion();

    const char *pszAddr = nullptr;
    switch (mode) {
        case TransmissionService::MULTICAST: {
            pszAddr = cfgReader.getMcastGroup();
            ui8McastTTL = cfgReader.getMcastTTL();
            break;
        }

        case TransmissionService::BROADCAST: {
            pszAddr = cfgReader.getBcastAddr();
            ui8McastTTL = 1; // not used
            break;
        }

        default: {
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "An unknown mode was set for Message Propagation "
                            "Service. Assuming MULTICAST, DisseminationService "
                            "may not behave as expected");
            pszAddr = cfgReader.getMcastGroup();
            ui8McastTTL = cfgReader.getMcastTTL();
        }
    }

    TransmissionService *pTrSvc = nullptr;
    const bool bUseRateEstimator = pCfgMgr->getValueAsBool ("aci.disService.nodeConfiguration.estimateCapacity", false);
    if (bUseRateEstimator) {
        //values used to speed up testing
        const uint8 ui8RateEstimatorUpdateFactor = pCfgMgr->getValueAsInt ("aci.disService.nodeConfiguration.estimateCapacity.updateFactor", 0);
        const uint8 ui8RateEstimatorDecreaseFactor = pCfgMgr->getValueAsInt ("aci.disService.nodeConfiguration.estimateCapacity.decreaseFactor", 0);
        const uint32 ui32StartingCapacity = pCfgMgr->getValueAsInt ("aci.disService.nodeConfiguration.estimatedLinkCapacity", 200);
        pTrSvc = new RateEstimatingTransmissionService (mode, ui16Port, pszAddr, ui8McastTTL,
                                                        pszNodeId, ui8RateEstimatorUpdateFactor,
                                                        ui8RateEstimatorDecreaseFactor, ui32StartingCapacity,
                                                        bAsyncDelivery, bAsyncTransmission, ui8MessageVersion);
    }
    else {
        pTrSvc = new TransmissionService (mode, ui16Port, pszAddr, ui8McastTTL, pszNodeId,
                                          bAsyncDelivery, bAsyncTransmission, ui8MessageVersion);
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "Disservice Running in %s mode. "
                    "The default destination address is %s:%d. The rate estimator is %s enabled.\n",
                    (mode == TransmissionService::BROADCAST ? "BROADCAST" : "MULTICAST"),
                    pszAddr, ui16Port, (bUseRateEstimator ? "" : "not"));

    return pTrSvc;
}

int TransmissionService::registerWithListeners (DisseminationService *pDisService)
{
    return 0;
}

int TransmissionService::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    return _pMPS->changeEncryptionKey (pchKey, ui32Len);
}

int TransmissionService::init (ConfigManager *pCfgMgr, const char **ppszBindingInterfaces,
                               const char **ppszIgnoredInterfaces, const char **ppszAddedInterfaces)
{
    const char *pszMethodName = "TransmissionService::init";

    _m.lock();
    if ((!_bAsyncTransmission) && (_ui8MessageVersion == 2)) {
        _ui8MessageVersion = 1;
        checkAndLogMsg (pszMethodName, Logger::L_Info, "No need to use NetworkMessage V2 "
                        "with AsyncTransmission disabled.\n");
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Using NetworkMessage V%u\n",
                    _ui8MessageVersion);

    const bool bReplyViaUnicast = pCfgMgr->getValueAsBool ("aci.disService.networkMessageService.replyViaUnicast", false);
	const char *pszGroupKeyFileName = pCfgMgr->getValue ("aci.disService.networkMessageService.groupKeyFile", nullptr);
    const bool bPassphraseEncryption = pCfgMgr->getValueAsBool ("aci.disService.networkMessageService.passphrase.encryption", false);
    // Instantiate NetworkMessageService in the proper mode
    const bool bRestart = (_pMPS != nullptr);
    if (bRestart) {
        _nmsHelper.start();
    }
    else {
        // initialize
        NOMADSUtil::PROPAGATION_MODE nmsMode;
        switch (_mode) {
            case MULTICAST:
                nmsMode = NOMADSUtil::MULTICAST;
                break;

            case BROADCAST:
                nmsMode = NOMADSUtil::BROADCAST;
                break;

            default:
                nmsMode = NOMADSUtil::MULTICAST;
        }
        const String sessionId (SessionId::getInstance ()->getSessionId());
        _pMPS = _nmsHelper.getNetworkMessageService (nmsMode, _ui16Port, _bAsyncDelivery, _bAsyncTransmission, _ui8MessageVersion, bReplyViaUnicast,
                                                     ppszBindingInterfaces, ppszIgnoredInterfaces, ppszAddedInterfaces, _dstAddr, _ui8McastTTL,
                                                     sessionId, bPassphraseEncryption, pszGroupKeyFileName, pCfgMgr);
        if (_pMPS == nullptr) {
            _m.lock();
            return -1;
        }
    }

    // Initialize NetworkMessageService
    if ((ppszBindingInterfaces != nullptr) && (ppszBindingInterfaces[0] != nullptr)) {
        _primaryIface = ppszBindingInterfaces[0];
    }

    // Set the retransmission timeout if specified
    if (pCfgMgr->hasValue ("aci.disService.networkMessageService.retransmissionTimeout")) {
        uint32 ui32Timeout = pCfgMgr->getValueAsUInt32 ("aci.disService.networkMessageService.retransmissionTimeout");
        _pMPS->setRetransmissionTimeout (ui32Timeout);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "set retransmission timeout to %u ms\n", ui32Timeout);
    }

    // See if there is a default link capacity specified in the config file. in that case, use it
    if (pCfgMgr->hasValue ("aci.disService.nodeConfiguration.estimatedLinkCapacity")) {
        uint32 ui32LinkCapacity = pCfgMgr->getValueAsUInt32 ("aci.disService.nodeConfiguration.estimatedLinkCapacity");
        ui32LinkCapacity = ui32LinkCapacity * (1024/8);
        char **ppszInterfaces = getActiveInterfacesAddress();
        if (ppszInterfaces != nullptr) {
            for (int i = 0; ppszInterfaces[i]; i++) {
                setLinkCapacity (ppszInterfaces[i], ui32LinkCapacity);
                free (ppszInterfaces[i]);
                ppszInterfaces[i] = nullptr;
            }
            free (ppszInterfaces);
            ppszInterfaces = nullptr;
        }
    }

    if (bRestart) {
        _nmsHelper.stop();
    }

    // Count the active interfaces and store the count
    char **ppszNICs = _pMPS->getActiveNICsInfoAsString();
    if (ppszNICs != nullptr) {
        for (uint8 ui8NInterfaces = 0; ppszNICs[ui8NInterfaces] != nullptr; ui8NInterfaces++) {
            int64 *pI64Time = new int64;
            *pI64Time = 0;
            _latestBcastTimeByIface.put (ppszNICs[ui8NInterfaces], pI64Time);
            free (ppszNICs[ui8NInterfaces]);
            ppszNICs[ui8NInterfaces] = nullptr;
        }
        free (ppszNICs);
    }

    if (pCfgMgr->hasValue ("aci.disService.transmissionLogFile")) {
        const char *pszFileName = pCfgMgr->getValue ("aci.disService.transmissionLogFile");
        _logger.init (pszFileName);
    }

    _m.unlock();

    return 0;
}

bool TransmissionService::isInitialized (void)
{
    _m.lock();
    bool bIsInitialized = _nmsHelper.isConnected();
    _m.unlock();
    return bIsInitialized;
}

int TransmissionService::start()
{
    return _nmsHelper.startAdaptors();
}

void TransmissionService::stop (void)
{
    _nmsHelper.stopAdaptors();
}

void TransmissionService::requestTermination (void)
{
    _nmsHelper.requestTerminationAndWait();
}

uint32 TransmissionService::getIncomingQueueSize()
{
    if (_pMPS == nullptr) {
        return 0U;
    }
    return _pMPS->getDeliveryQueueSize();
}

char ** TransmissionService::getActiveInterfacesAddress()
{
    if (_pMPS == nullptr) {
        return nullptr;
    }
    return _pMPS->getActiveNICsInfoAsString();
}

bool TransmissionService::getAsyncTransmission()
{
    return _bAsyncTransmission;
}

bool TransmissionService::getSharesQueueLength()
{
    return (_ui8MessageVersion == 2) ? true : false;
}

uint16 TransmissionService::getMTU()
{
    uint16 ui16FragSize = _pMPS->getMinMTU();
    if (ui16FragSize == 0) {
        return ui16FragSize;
    }
    // This is to accommodate the 1 byte of metadata passed by DisService to the NetworkMessageService
    return (ui16FragSize - 1);
}

char ** TransmissionService::getInterfacesByDestinationAddress (const char *pszDestinationAddresses)
{
    if (pszDestinationAddresses == nullptr) {
        return nullptr;
    }
    const InetAddr addr (pszDestinationAddresses);
    return getInterfacesByDestinationAddress (addr.getIPAddress());
}

char ** TransmissionService::getInterfacesByDestinationAddress (uint32 ui32DestinationAddress)
{
    if (ui32DestinationAddress == INADDR_NONE || ui32DestinationAddress == INADDR_ANY) {
        return nullptr;
    }

    return _pMPS->getActiveNICsInfoAsStringForDestinationAddr (ui32DestinationAddress);
}

char ** TransmissionService::getInterfacesByReceiveRate (float fPercRateThreshold)
{
    if (fPercRateThreshold > 1 || fPercRateThreshold < 0) {
        checkAndLogMsg ("TransmissionService::getInterfacesByReceiveRate", Logger::L_Warning,
                        "fPercRateThreshold must be a value between 0 and 1; %f\n",
                        fPercRateThreshold);
        return nullptr;
    }

    // The value returned by _pMPS->getActiveNICsInfoAsString() will be deallocated by
    // getInterfacesByReceiveRate()
    return getInterfacesByReceiveRate (fPercRateThreshold, _pMPS->getActiveNICsInfoAsString());
}

char ** TransmissionService::getSilentInterfaces (uint32 ui32TimeThreshold)
{
    const int64 i64CurrTime = getTimeInMilliseconds();
    if (i64CurrTime <= 0) {
        return nullptr;
    }

    _m.lock();
    char **ppszAllInterfaces = _pMPS->getActiveNICsInfoAsString();
    if (ppszAllInterfaces == nullptr) {
        _m.unlock();
        return nullptr;
    }
    unsigned int uiCount = 0;
    for (; ppszAllInterfaces[uiCount] != nullptr; uiCount++);
    if (uiCount == 0) {
        free (ppszAllInterfaces);
        _m.unlock();
        return nullptr;
    }

    char **ppszInterfaces = (char **) calloc (uiCount + 1, sizeof (char *));
    if (ppszInterfaces == nullptr) {
        deallocateNullTerminatedPtrArray (ppszAllInterfaces);
        _m.unlock();
        return nullptr;
    }
    uiCount = 0;
    for (unsigned int i = 0; ppszAllInterfaces[i] != nullptr; i++) {
        int64 *pUI64Time = _latestBcastTimeByIface.get (ppszAllInterfaces[i]);
        if ((pUI64Time == nullptr) || ((i64CurrTime - (*pUI64Time)) >= ui32TimeThreshold)) {
            ppszInterfaces[i] = strDup (ppszAllInterfaces[i]);
            if (ppszInterfaces[i] != nullptr) {
                uiCount++;
            }
            else {
                deallocateNullTerminatedPtrArray (ppszAllInterfaces);
                deallocateNullTerminatedPtrArray (ppszInterfaces);
                _m.unlock();
                return nullptr;
            }
        }
    }
    ppszInterfaces[uiCount] = nullptr;
    _m.unlock();

    deallocateNullTerminatedPtrArray (ppszAllInterfaces);
    if (ppszInterfaces[0] == nullptr) {
        deallocateNullTerminatedPtrArray (ppszInterfaces);
        ppszInterfaces = nullptr;
    }

    return ppszInterfaces;
}

char ** TransmissionService::getInterfacesByOutgoingQueueLength (float fPercLengthThreshold, char **ppszInterfaces)
{
    if (fPercLengthThreshold > 1 || fPercLengthThreshold < 0) {
        checkAndLogMsg ("TransmissionService::getInterfacesByOutgoingQueueLength", Logger::L_Warning,
                        "fPercRateThreshold must be a value between 0 and 1; %f\n",
                        fPercLengthThreshold);
        return nullptr;
    }

    bool bReleaseInterfaces = false;
    if (ppszInterfaces == nullptr) {
        ppszInterfaces = _pMPS->getActiveNICsInfoAsString();
        bReleaseInterfaces = true;
    }

    char **pszRetInterfaces = getInterfacesByOutgoingQueueLengthInternal (fPercLengthThreshold, (const char **) ppszInterfaces);
    if (bReleaseInterfaces) {
        deallocateStringsArray (ppszInterfaces);
    }
    return pszRetInterfaces;
}

char ** TransmissionService::getInterfacesByReceiveRateAndOutgoingQueueLenght (float fPercRateThreshold, float fPercLengthThreshold)
{
    if (fPercRateThreshold > 1 || fPercRateThreshold < 0) {
        checkAndLogMsg ("TransmissionService::getInterfacesByReceiveRateAndOutgoingQueueLenght", Logger::L_Warning,
                        "fPercRateThreshold must be a value between 0 and 1; %f\n",
                        fPercRateThreshold);
        return nullptr;
    }
    if (fPercLengthThreshold > 1 || fPercLengthThreshold < 0) {
        checkAndLogMsg ("TransmissionService::getInterfacesByReceiveRateAndOutgoingQueueLenght", Logger::L_Warning,
                        "fPercLengthThreshold must be a value between 0 and 1; %f\n",
                        fPercRateThreshold);
        return nullptr;
    }

    // _pMPS->getActiveNICsInfoAsString() will be deallocated by getInterfacesByReceiveRate()
    char **ppszInterfacesByRate = getInterfacesByReceiveRate (fPercRateThreshold, _pMPS->getActiveNICsInfoAsString());
    if (ppszInterfacesByRate == nullptr) {
        return nullptr;
    }

    char **ppNICsByOutQueue = getInterfacesByOutgoingQueueLength (fPercLengthThreshold, ppszInterfacesByRate);
    // getInterfacesByReceiveRate deallocates a new array and copies the
    // interfaces addresses into it, thus it must be deallocated
    deallocateStringsArray (ppszInterfacesByRate);

    return ppNICsByOutQueue;
}

char ** TransmissionService::getInterfacesByReceiveRate (float fPercRateThreshold, char **ppszInputInterfaces)
{
    const char *pszMethodName = "TransmissionService::getInterfacesByReceiveRate";

    if (ppszInputInterfaces == nullptr) {
        return nullptr;
    }

    uint8 ui8InputInterfaces = 0;
    for (; ppszInputInterfaces[ui8InputInterfaces] != nullptr; ui8InputInterfaces++);

    char **ppszInterfaces = (char **) calloc (sizeof (char *), ui8InputInterfaces+1);
    if (ppszInterfaces == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return nullptr;
    }
    uint8 ui8AddedInterfaces = 0;
    for (unsigned int uiIndex = 0; (ppszInputInterfaces[uiIndex] != nullptr) && (ui8AddedInterfaces < ui8InputInterfaces); ++uiIndex) {
        char *pszIPAddr = ppszInputInterfaces[uiIndex];
        int64 i64EstimatedRate = _pMPS->getReceiveRate (pszIPAddr);
        if (i64EstimatedRate < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "error when trying to "
                            "retrieve the estimated receive rate. rc = %lld\n", i64EstimatedRate);
            i64EstimatedRate = 0;
        }
        uint32 ui32LinkCapacity = _pMPS->getLinkCapacity (pszIPAddr);
        uint32 ui32RateLimit;
        if (ui32LinkCapacity > 0) {
            ui32RateLimit = ui32LinkCapacity;
        }
        else{
            ui32RateLimit = _pMPS->getTransmitRateLimit (pszIPAddr);
        }

        if (ui32RateLimit == 0) {
            // Rate limit not set.  Assume 100Mbps
            ui32RateLimit = 100*(1024/8);   // Bps
            if ((uint32)i64EstimatedRate > ui32RateLimit) {
                // Assume 1000Mbps
                ui32RateLimit = 1000*(1024/8);   // Bps
            }
        }
        float fPercentualRate = (float)i64EstimatedRate/(float)ui32RateLimit;

        checkAndLogMsg (pszMethodName, Logger::L_Info, "Estimated rate: %lld. Rate Limit: %u. Ration %f\n",
                        i64EstimatedRate, ui32RateLimit, fPercentualRate);
        if (fPercentualRate > 1) {
            fPercentualRate = 1;
        }
        if (fPercentualRate <= fPercRateThreshold) {
            ppszInterfaces[ui8AddedInterfaces] = pszIPAddr;
            ui8AddedInterfaces++;
        }
        else {
            free (pszIPAddr);
        }
    }

    if (ui8AddedInterfaces == 0) {
        free (ppszInterfaces);
        ppszInterfaces = nullptr;
    }
    free (ppszInputInterfaces);

    return ppszInterfaces;
}

char ** TransmissionService::getInterfacesByOutgoingQueueLengthInternal (float fPercLengthThreshold,
                                                                         const char **ppszInputInterfaces)
{
    if (ppszInputInterfaces == nullptr) {
        return nullptr;
    }

    const char *pszMethodName = "TransmissionService::getInterfacesByOutgoingQueueLengthInternal";

    // Count the input interfaces
    uint8 ui8NInterfaces = 0;
    for ( ; ppszInputInterfaces[ui8NInterfaces] != nullptr; ui8NInterfaces++);

    char **ppszInterfaces = (char **) calloc (sizeof (char *), ui8NInterfaces+1);
    if (ppszInterfaces == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return nullptr;
    }

    uint8 ui8AddedInterfaces = 0;
    for (unsigned int i = 0; (ppszInputInterfaces[i] != nullptr) && (ui8AddedInterfaces < ui8NInterfaces); i++) {
        uint8 ui8RescaledQueueLength = getRescaledTransmissionQueueSize (ppszInputInterfaces[i]);
        uint8 ui8Limit = (uint8) (255 * fPercLengthThreshold);

        checkAndLogMsg (pszMethodName, Logger::L_Info, "Ratio %u/255\n", ui8RescaledQueueLength);

        if (ui8RescaledQueueLength <= ui8Limit) {
            ppszInterfaces[ui8AddedInterfaces] = strDup (ppszInputInterfaces[i]);
            if (ppszInterfaces[ui8AddedInterfaces] != nullptr) {
                ui8AddedInterfaces++;
            }
            else {
                checkAndLogMsg (pszMethodName, memoryExhausted);
            }
        }
    }

    if (ui8AddedInterfaces == 0) {
        free (ppszInterfaces);
        ppszInterfaces = nullptr;
    }

    return ppszInterfaces;
}

bool TransmissionService::checkAndPackMsg (DisServiceMsg *pDSMsg, BufferWriter *pWriter, uint32 ui32MaxMsgSize)
{
    const char *pszMethodName = "TransmissionService::checkAndPackMsg";
    if (pWriter == nullptr) {
        return false;
    }
    if (pDSMsg == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "null message\n");
        return false;
    }

    // Sanity checks
    const String sessionId (SessionId::getInstance()->getSessionId());
    if (pDSMsg->getSenderNodeId() == nullptr) {
        pDSMsg->setSenderNodeId (_nodeId);
    }
    if (((pDSMsg->getSessionId() == nullptr) && (sessionId.length() > 0)) || (sessionId != pDSMsg->getSessionId())) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "sending message of type %s (%d) with wrong "
                        "session id: <%s> while it should have been set to <%s>.\n",
                        DisServiceMsgHelper::getMessageTypeAsString (pDSMsg->getType()), (int) pDSMsg->getType(),
                        pDSMsg->getSessionId(), sessionId.c_str());
    }
    bool bUseWildcardSessionId = false;
    if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) {
        DisServiceDataMsg *pDataMsg = (DisServiceDataMsg *)pDSMsg;
        String group (pDataMsg->getMessageHeader()->getGroupName());
        group.convertToLowerCase();
        if (wildcardStringCompare (group, "*reset")) {
            bUseWildcardSessionId = true;
            checkAndLogMsg (pszMethodName, Logger::L_Info, "setting wildcard sessionId for message of group %s.\n", group.c_str());
        }
    }

    if (bUseWildcardSessionId) {
        pDSMsg->setSessionId ("*");
    }
    else if (pDSMsg->getSessionId() == nullptr || (strlen (pDSMsg->getSessionId()) == 0)) {
        pDSMsg->setSessionId (sessionId);
    }

    // Serialize
    int rc = pDSMsg->write (pWriter, ui32MaxMsgSize);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "could not serialize message. Returned code: %d\n", rc);
        return false;
    }
    unsigned long ul32written = pWriter->getBufferLength();
    if (ul32written > ui32MaxMsgSize) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "the written message exceeds "
                        "the MAX FRAGMENT SIZE (%u): <%u>\n", ui32MaxMsgSize, ul32written);
        return false;
    }
    if (ul32written == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "wrote empty message\n");
        return false;
    }
    return true;
}

uint32 TransmissionService::getReceiveRate (const char *pszAddr)
{
    return (uint32) _pMPS->getReceiveRate (pszAddr);
}

uint32 TransmissionService::getTransmitRateLimit (const char *pszAddr)
{
    return _pMPS->getTransmitRateLimit (pszAddr);
}

uint32 TransmissionService::getTransmitRateLimitCap (void)
{
    return _ui32RateLimitCap;
}

void TransmissionService::setTransmitRateLimitCap (uint32 ui32Cap)
{
    _ui32RateLimitCap = ui32Cap;
    checkAndLogMsg ("TransmissionService::setTransmitRateLimitCap",
                    Logger::L_Info, "The Rate Limit cap is %u\n", _ui32RateLimitCap);
}

uint32 TransmissionService::getLinkCapacity (const char *pszInterface)
{
    return _pMPS->getLinkCapacity (pszInterface);
}

void TransmissionService::setLinkCapacity (const char *pszInterface, uint32 ui32Capacity)
{
    _pMPS->setLinkCapacity (pszInterface, ui32Capacity);
}

uint16 TransmissionService::getMaxFragmentSize (void)
{
    return _ui16MaxFragmentSize;
}

void TransmissionService::setMaxFragmentSize (uint16 ui16MaxFragmentSize)
{
    if (ui16MaxFragmentSize > 0) {
        _ui16MaxFragmentSize = ui16MaxFragmentSize;
    }
}

uint32 TransmissionService::getTransmissionQueueSize (const char *pszInterface)
{
    return _pMPS->getTransmissionQueueSize (pszInterface);
}

uint8 TransmissionService::getRescaledTransmissionQueueSize (const char *pszInterface)
{
    return _pMPS->getRescaledTransmissionQueueSize (pszInterface);
}

uint8 TransmissionService::getNeighborQueueSize (const char *pszInterface, uint32 ui32Address)
{
    return _pMPS->getNeighborQueueLength (pszInterface, ui32Address);
}

uint32 TransmissionService::getTransmissionQueueMaxSize (const char *pszInterface)
{
    return _pMPS->getTransmissionQueueMaxSize (pszInterface);
}

int TransmissionService::setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    uint32 ui32RateLimitCapped;
    if (_ui32RateLimitCap > 0) {
         ui32RateLimitCapped = minimum (ui32RateLimit, _ui32RateLimitCap);
    }
    else {
        ui32RateLimitCapped = ui32RateLimit;
    }
    return _pMPS->setTransmitRateLimit (pszInterface, pszDestinationAddress, ui32RateLimitCapped);
}

int TransmissionService::setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    uint32 ui32RateLimitCapped;
    if (_ui32RateLimitCap > 0) {
            ui32RateLimitCapped = minimum (ui32RateLimit, _ui32RateLimitCap);
    }
    else {
        ui32RateLimitCapped = ui32RateLimit;
    }
    return _pMPS->setTransmitRateLimit (pszDestinationAddress, ui32RateLimitCapped);
}

int TransmissionService::setTransmitRateLimit (uint32 ui32RateLimit)
{
    uint32 ui32RateLimitCapped;
    if (_ui32RateLimitCap > 0) {
         ui32RateLimitCapped = minimum (ui32RateLimit, _ui32RateLimitCap);
    }
    else {
        ui32RateLimitCapped = ui32RateLimit;
    }
    return _pMPS->setTransmitRateLimit (ui32RateLimitCapped);
}

bool TransmissionService::clearToSend (const char *pszInterface)
{
    if (_pMPS != nullptr) {
        return _pMPS->clearToSend (pszInterface);
    }
    return false;
}

bool TransmissionService::clearToSendOnAllInterfaces (void)
{
    if (_pMPS != nullptr) {
        return _pMPS->clearToSendOnAllInterfaces();
    }
    return false;
}

int TransmissionService::registerHandlerCallback (uint8 ui8MsgType, TransmissionServiceListener *pListener)
{
    if (_pMPS != nullptr) {
        if (_pMPS->registerHandlerCallback (ui8MsgType, pListener) < 0) {
            return -2;
        }
        return 0;
    }

    return -1;
}

TransmissionService::TransmissionResults TransmissionService::broadcast (DisServiceMsg *pDSMsg, const char **ppszOutgoingInterfaces,
                                                                         const char *pszPurpose, const char *pszTargetAddr, const char *pszHints)
{
    const char *pszMethodName = "TransmissionService::broadcast";

    BufferWriter bw;
    TransmissionResults bcastRes;
    if (pDSMsg == nullptr) {
        bcastRes.rc = -1;
        return bcastRes;
    }

    // The NMS does not fragment broadcast messages, therefore it must be ensured
    // the the outgoing messages fit the MTU
    if (!checkAndPackMsg (pDSMsg, &bw, minimum (getMTU(), getMaxFragmentSize()))) {
        bcastRes.rc = -2;
        return bcastRes;
    }

    bcastRes.bDeallocateOutgoingInterfaces = false;
    if (ppszOutgoingInterfaces == nullptr) {
        // Send on all the active interfaces
        bcastRes.ppszOutgoingInterfaces = getActiveInterfacesAddress();
        if (bcastRes.ppszOutgoingInterfaces == nullptr) {
            bcastRes.rc = -3;
            return bcastRes;
        }
        bcastRes.bDeallocateOutgoingInterfaces = true;
    }
    else {
        // Copy interfaces
        unsigned int i = 0;
        for (; ppszOutgoingInterfaces[i] != nullptr; i++);
        if (i > 0) {
            bcastRes.ppszOutgoingInterfaces = (char **) calloc (i+1, sizeof (char*));
            if (bcastRes.ppszOutgoingInterfaces != nullptr) {
                for (unsigned j = 0; ppszOutgoingInterfaces[j] != nullptr; j++) {
                    bcastRes.ppszOutgoingInterfaces[j] = strDup (ppszOutgoingInterfaces[j]);
                    if (bcastRes.ppszOutgoingInterfaces[j] == nullptr) {
                        break;
                    }
                }
            }
            bcastRes.bDeallocateOutgoingInterfaces = true;
        }
        else {
            bcastRes.ppszOutgoingInterfaces = nullptr;
            bcastRes.bDeallocateOutgoingInterfaces = false;
        }
    }
    if (bcastRes.ppszOutgoingInterfaces == nullptr) {
        bcastRes.rc = -3;
        return bcastRes;
    }

    const uint32 ui32TargetAddr = (pszTargetAddr == nullptr ?
                                   0 :  // NMS will use the default broadcast/multicast address
                                   inet_addr (pszTargetAddr));
    bool bExpedited = ((pDSMsg->getType() == DisServiceMsg::DSMT_DataReq) ? true : false);
    if (pDSMsg->getType() > 0xFF) {
        bcastRes.rc = -4;
        return bcastRes;
    }

    const uint16 ui16Type = static_cast<uint16>(pDSMsg->getType());
    if (ui16Type > 0xFF) {
        bcastRes.rc = -5;
        return bcastRes;
    }
    const uint8 ui8Type = static_cast<uint8>(ui16Type);
    bcastRes.rc = 0;

    _m.lock();
    for (unsigned int i = 0; bcastRes.ppszOutgoingInterfaces[i] != nullptr; i++) {
        const char *interfaces[2];
        interfaces[0] = bcastRes.ppszOutgoingInterfaces[i];
        interfaces[1] = nullptr;
        int rc = _pMPS->broadcastMessage (DisseminationService::MPSMT_DisService,
                                          interfaces, ui32TargetAddr,
                                          static_cast<uint16>(0), // ui16MsgId
                                          static_cast<uint8>(0),  // ui8HopCount
                                          static_cast<uint8>(1),  // TTL
                                          static_cast<uint16>(0), // delay tolerance
                                          &ui8Type, sizeof (ui8Type), bw.getBuffer(),
                                          static_cast<uint16>(bw.getBufferLength()),
                                          bExpedited, pszHints);
        if (rc != 0) {
            bcastRes.rc = rc;
        }
        else {
            updateLatestBcastTimeByIface (pDSMsg, bcastRes.ppszOutgoingInterfaces[i]);
        }
    }

    if (bcastRes.rc == 0) {
        _logger.log (pDSMsg->getType(), bw.getBufferLength(), pszPurpose);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not broadcast "
                        "message. MPS failed with rc = %d\n", bcastRes.rc);
    }
    _m.unlock();

    return bcastRes;
}

TransmissionService::TransmissionResults TransmissionService::unicast (DisServiceMsg *pDSMsg, const char **ppszOutgoingInterfaces,
                                                                       const char *pszPurpose, const char *pszTargetAddr, const char *pszHints,
                                                                       bool bReliable)
{
    const char *pszMethodName = "TransmissionService::unicast";

    BufferWriter bw;

    TransmissionResults ucastRes;
    if (pDSMsg == nullptr) {
        ucastRes.rc = -1;
        return ucastRes;
    }

    // The NMS fragments transmitted messages that are longer than the MTU, therefore
    // there's no need to check for the length of the outgoing packet
    if (!checkAndPackMsg (pDSMsg, &bw, 0xFFFF)) {
        ucastRes.rc = -2;
        return ucastRes;
    }

    const uint32 ui32TargetAddr = (pszTargetAddr == nullptr ?
                                   0 :  // NMS will use the default broadcast/multicast address
                                   inet_addr (pszTargetAddr));

    ucastRes.bDeallocateOutgoingInterfaces = false;
    if (ppszOutgoingInterfaces == nullptr) {
        // Send on all the active interfaces
        ucastRes.ppszOutgoingInterfaces = (ui32TargetAddr > 0 ? getInterfacesByDestinationAddress (ui32TargetAddr) : getActiveInterfacesAddress());
        if ((ucastRes.ppszOutgoingInterfaces == nullptr) && (_primaryIface.length() > 0)) {
            ucastRes.ppszOutgoingInterfaces = (char **) calloc (2, sizeof (char*));
            ucastRes.ppszOutgoingInterfaces[0] = strDup (_primaryIface);
            ucastRes.ppszOutgoingInterfaces[1] = nullptr;
        }
        if (ucastRes.ppszOutgoingInterfaces == nullptr) {
            ucastRes.rc = -3;
            return ucastRes;
        }
        ucastRes.bDeallocateOutgoingInterfaces = true;
    }
    else {
        // Copy interfaces
        unsigned int i = 0;
        for (; ppszOutgoingInterfaces[i] != nullptr; i++);
        if (i > 0) {
            ucastRes.ppszOutgoingInterfaces = (char **) calloc (i+1, sizeof (char*));
            if (ucastRes.ppszOutgoingInterfaces != nullptr) {
                for (unsigned j = 0; ppszOutgoingInterfaces[j] != nullptr; j++) {
                    ucastRes.ppszOutgoingInterfaces[j] = strDup (ppszOutgoingInterfaces[j]);
                    if (ucastRes.ppszOutgoingInterfaces[j] == nullptr) {
                        break;
                    }
                }
            }
            ucastRes.bDeallocateOutgoingInterfaces = true;
        }
        else {
            ucastRes.ppszOutgoingInterfaces = nullptr;
            ucastRes.bDeallocateOutgoingInterfaces = false;
        }
    }
    if (ucastRes.ppszOutgoingInterfaces == nullptr) {
        ucastRes.rc = -4;
        return ucastRes;
    }

    if (pDSMsg->getType() > 0xFF) {
        ucastRes.rc = -5;
        return ucastRes;
    }
    const uint8 ui8Type = (uint8) pDSMsg->getType();
    ucastRes.rc = 0;

    _m.lock();

    if (bReliable) {
        ucastRes.rc = _pMPS->transmitReliableMessage (DisseminationService::MPSMT_DisService,
                                                       (const char **) ucastRes.ppszOutgoingInterfaces,
                                                       ui32TargetAddr,
                                                       static_cast<uint16>(0), // ui16MsgId
                                                       static_cast<uint8>(0),  // ui8HopCount
                                                       static_cast<uint8>(1),  // TTL
                                                       static_cast<uint16>(0), // delay tolerance
                                                       &ui8Type, sizeof (ui8Type), bw.getBuffer(),
                                                       static_cast<uint16>(bw.getBufferLength()), pszHints);
    }
    else {
        ucastRes.rc = _pMPS->transmitMessage (DisseminationService::MPSMT_DisService,
                                               (const char **) ucastRes.ppszOutgoingInterfaces,
                                               ui32TargetAddr,
                                               static_cast<uint16>(0), // ui16MsgId
                                               static_cast<uint8>(0),  // ui8HopCount
                                               static_cast<uint8>(1),  // TTL
                                               static_cast<uint16>(0), // delay tolerance
                                               &ui8Type, sizeof (ui8Type), bw.getBuffer(),
                                               static_cast<uint16>(bw.getBufferLength()), pszHints);
    }

    if (ucastRes.rc == 0) {
        _logger.log (pDSMsg->getType(), bw.getBufferLength(), pszPurpose);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not broadcast message. MPS failed with rc = %d\n",
                        ucastRes.rc);
    }
    _m.unlock();

    return ucastRes;
}

void logTransmission (DisServiceMsg *p, const char *pszOutgoingInterface, uint64 ui64LatestTransmission)
{
    if (p->getType() == DisServiceMsg::DSMT_Data) {
        DisServiceDataMsg *pDataMsg = static_cast<DisServiceDataMsg *>(p);
        static StringHashtable<uint32> _count{true, true, true, true};
        uint32 *pCount = _count.get (pDataMsg->getMessageHeader()->getMsgId());
        if (pCount == nullptr) {
            pCount = new uint32;
            *pCount = 1;
            _count.put (pDataMsg->getMessageHeader()->getMsgId(), pCount);
        }
        else {
            *pCount = (*pCount + 1);
        }
        String repair;
        if (pDataMsg->isRepair()) {
            repair = ("[Repair] for");
            repair += pDataMsg->getTargetNodeId();
            repair += "]";
        }

        if (pDataMsg->isRepair()) {
            printf ("Sent Message DSMT_Data at time %lld on interface %s. Id: %s. Served: %u times. [Repair for %s].\n",
                    ui64LatestTransmission, pszOutgoingInterface, pDataMsg->getMessageHeader()->getMsgId(),
                    *pCount, pDataMsg->getTargetNodeId());
        }
        else {
            printf ("Sent Message DSMT_Data at time %lld on interface %s. Id: %s. Served: %u times.\n",
                    ui64LatestTransmission, pszOutgoingInterface, pDataMsg->getMessageHeader()->getMsgId(),
                    *pCount);
        }
    }
    else {
        printf ("Sent Message %s at time %lld on interface %s\n",
                DisServiceMsgHelper::getMessageTypeAsString (p->getType()),
                ui64LatestTransmission, pszOutgoingInterface);
    }
    fflush (stdout);
}

void TransmissionService::updateLatestBcastTimeByIface (DisServiceMsg *p, const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == nullptr) {
        return;
    }
    int64 i64CurrTime = getTimeInMilliseconds();
    if (i64CurrTime > 0) {
        int64 *pUI64LatestTransmission = _latestBcastTimeByIface.get (pszOutgoingInterface);
        if (pUI64LatestTransmission == nullptr) {
            pUI64LatestTransmission = new int64;
            _latestBcastTimeByIface.put (pszOutgoingInterface, pUI64LatestTransmission);
        }

        *pUI64LatestTransmission = i64CurrTime;
        logTransmission (p, pszOutgoingInterface, i64CurrTime);
    }
}

TransmissionService::TransmissionResults::TransmissionResults (void)
{
    rc = -1;
    ppszOutgoingInterfaces = nullptr;
    bDeallocateOutgoingInterfaces = false;
}

TransmissionService::TransmissionResults::TransmissionResults (TransmissionResults && rTransmissionResults) :
    rc{std::move (rTransmissionResults.rc)}, ppszOutgoingInterfaces{std::move (rTransmissionResults.ppszOutgoingInterfaces)},
    bDeallocateOutgoingInterfaces{std::move (rTransmissionResults.bDeallocateOutgoingInterfaces)}
{
    rTransmissionResults.ppszOutgoingInterfaces = nullptr;
    rTransmissionResults.bDeallocateOutgoingInterfaces = false;
}

TransmissionService::TransmissionResults::~TransmissionResults (void)
{
    if (bDeallocateOutgoingInterfaces) {
        reset();
    }
}

void TransmissionService::TransmissionResults::reset (void)
{
    rc = -1;
    if (bDeallocateOutgoingInterfaces && (ppszOutgoingInterfaces != nullptr)) {
        for (unsigned int i = 0; ppszOutgoingInterfaces[i] != nullptr; i++) {
            free (ppszOutgoingInterfaces[i]);
        }
        free (ppszOutgoingInterfaces);
    }

    ppszOutgoingInterfaces = nullptr;
    bDeallocateOutgoingInterfaces = false;
}

//------------------------------------------------------------------------------
// TransmissionServiceHelper
//------------------------------------------------------------------------------

RateEstimatingTransmissionService::RateEstimatingTransmissionService (TRANSMISSION_SVC_MODE mode, uint16 ui16NetworkMessageServicePort,
                                                                      const char* pszMcastGroup, uint8 ui8McastTTL, const char *pszNodeId,
                                                                      uint8 ui8RateEstimatorUpdateFactor, uint8 ui8RateEstimatorDecreaseFactor,
                                                                      uint32 ui32RateEstimatorStartingCapacity, bool bAsyncDelivery, bool bAsyncTransmission,
                                                                      uint8 ui8MessageVersion)
    : TransmissionService (mode, ui16NetworkMessageServicePort, pszMcastGroup, ui8McastTTL,
                           pszNodeId, bAsyncDelivery, bAsyncTransmission,
                           ui8MessageVersion),
      _rateEstimator (this, ui8RateEstimatorUpdateFactor, ui8RateEstimatorDecreaseFactor,
                      ui32RateEstimatorStartingCapacity)
{
}

RateEstimatingTransmissionService::~RateEstimatingTransmissionService (void)
{
}

int RateEstimatingTransmissionService::init (NOMADSUtil::ConfigManager *pCfgMgr, const char **ppszBindingInterfaces,
                                             const char **ppszIgnoredInterfaces, const char **ppszAddedInterfaces)
{
    int rc = TransmissionService::init (pCfgMgr, ppszBindingInterfaces, ppszIgnoredInterfaces, ppszAddedInterfaces);
    if (rc < 0) {
        return rc;
    }

    rc = _rateEstimator.init();
    if (rc < 0) {
        return rc;
    }

    return rc;
}

TransmissionService::TransmissionResults RateEstimatingTransmissionService::broadcast (DisServiceMsg *pDSMsg, const char **ppszOutgoingInterfaces,
                                                                                       const char *pszPurpose, const char *pszTargetAddr, const char *pszHints)
{
    TransmissionResults bcastRes;

    if (pDSMsg == nullptr) {
        return bcastRes;
    }
    if (pDSMsg->getType() != DisServiceMsg::DSMT_Data) {
        return TransmissionService::broadcast (pDSMsg, ppszOutgoingInterfaces, pszPurpose, pszTargetAddr, pszHints);
    }

    // It's data Message: set the sending rate
    bcastRes.bDeallocateOutgoingInterfaces = false;
    if (ppszOutgoingInterfaces == nullptr) {
        // Send on all the active interfaces
        bcastRes.ppszOutgoingInterfaces = getActiveInterfacesAddress();
        if (bcastRes.ppszOutgoingInterfaces == nullptr) {
            bcastRes.rc = -3;
            return bcastRes;
        }
        bcastRes.bDeallocateOutgoingInterfaces = true;
    }
    else {
        // Copy interfaces
        unsigned int i = 0;
        for (; ppszOutgoingInterfaces[i] != nullptr; i++);
        if (i > 0) {
            bcastRes.ppszOutgoingInterfaces = (char **) calloc (i+1, sizeof (char*));
            if (bcastRes.ppszOutgoingInterfaces != nullptr) {
                for (unsigned j = 0; ppszOutgoingInterfaces[j] != nullptr; j++) {
                    bcastRes.ppszOutgoingInterfaces[j] = strDup (ppszOutgoingInterfaces[j]);
                    if (bcastRes.ppszOutgoingInterfaces[j] == nullptr) {
                        break;
                    }
                }
            }
            bcastRes.bDeallocateOutgoingInterfaces = true;
        }
        else {
            bcastRes.ppszOutgoingInterfaces = nullptr;
            bcastRes.bDeallocateOutgoingInterfaces = false;
        }
    }

    bcastRes.rc = 0;
    DisServiceDataMsg *pDataMsg = ((DisServiceDataMsg *) pDSMsg);
    for (unsigned int i = 0; bcastRes.ppszOutgoingInterfaces[i] != nullptr; i++) {
        const char *interfaces[2] = {bcastRes.ppszOutgoingInterfaces[i], nullptr};

        if (pDataMsg->getMessageHeader()->getMsgSeqId() % 2 == 0) {
            pDataMsg->setSendRate (getTransmitRateLimit (bcastRes.ppszOutgoingInterfaces[i]));
        }
        else {
            pDataMsg->setRateEstimate (_rateEstimator.getNetworkCapacityToAdvertise (bcastRes.ppszOutgoingInterfaces[i]));
        }

        TransmissionResults tmpRet = TransmissionService::broadcast (pDataMsg, interfaces, pszPurpose, pszTargetAddr, pszHints);
        if (tmpRet.rc == 0) {
            _rateEstimator.setInterfaceIsActive (interfaces);
        }
        else {
            bcastRes.rc = tmpRet.rc;
        }
    }

    return bcastRes;
}

int RateEstimatingTransmissionService::registerWithListeners (DisseminationService *pDisService)
{
    unsigned int uiIndex;
    if (pDisService->registerMessageListener (&_rateEstimator, uiIndex) == 0) {
        _rateEstimator.start();
        return 0;
    }
    return -1;
}

//------------------------------------------------------------------------------
// TransmissionServiceHelper
//------------------------------------------------------------------------------

TransmissionServiceHelper::TransmissionServiceHelper (bool bUseRateEstimator)
    : _bUseRateEstimator (bUseRateEstimator)
{
}

TransmissionServiceHelper::~TransmissionServiceHelper (void)
{
}

uint32 TransmissionServiceHelper::computeMessageHeaderSize (const char *pszNodeId, const char *pszTargetNodeId,
                                                            const char *pszSessionId, MessageHeader *pMH)
{
    if (pMH == nullptr) {
        return 0U;
    }

    // Create a dummy message to see how long its length is going to be
    Message msg (pMH, nullptr);
    DisServiceDataMsg dataMsg (pszNodeId, &msg, pszTargetNodeId);
    dataMsg.setSessionId (pszSessionId);
    if (_bUseRateEstimator) {
        //put a temporary value, just to have the right header size
        dataMsg.setRateEstimate (1U);
    }

    static NullWriter nw;  // NullWriter is stateless - therefore thread-safe
    InstrumentedWriter iw (&nw, false);
    dataMsg.write (&iw, 0);
    return iw.getBytesWritten();
}

//------------------------------------------------------------------------------
// TransmissionServiceLogger
//------------------------------------------------------------------------------

TransmissionServiceLogger::TransmissionServiceLogger (void)
{
    _filePacketXMitLog = nullptr;
}

TransmissionServiceLogger::~TransmissionServiceLogger (void)
{
    if (_filePacketXMitLog != nullptr) {
        fclose (_filePacketXMitLog);
        _filePacketXMitLog = nullptr;
    }
}

void TransmissionServiceLogger::init (const char *pszFileName)
{
    if (_filePacketXMitLog != nullptr) {
        fclose (_filePacketXMitLog);
        _filePacketXMitLog = nullptr;
    }
    if (pszFileName != nullptr) {
        _filePacketXMitLog = fopen (pszFileName , "w");
        if (_filePacketXMitLog == nullptr) {
            checkAndLogMsg ("TransmissionServiceLogger::TransmissionServiceLogger",
                            Logger::L_MildError, "error opening transmission log file\n");
        }
    }
}

void TransmissionServiceLogger::log (DisServiceMsg::Type msgType, uint32 ui32MsgLen, const char *pszPurpose)
{
    if (_filePacketXMitLog == nullptr) {
        return;
    }
    time_t now = time (nullptr);
    struct tm *ptm = localtime (&now);
    fprintf (_filePacketXMitLog, "%02d:%02d:%02d - %d - %u - %s\n",
             ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
             (int) msgType, ui32MsgLen,
             pszPurpose ? pszPurpose : "<unknown>");
}

//------------------------------------------------------------------------------
// TransmissionServiceConfReader
//------------------------------------------------------------------------------

TransmissionServiceConfReader::TransmissionServiceConfReader (NOMADSUtil::ConfigManager *pCfgMgr)
    : _pCfgMgr (pCfgMgr)
{
}

TransmissionServiceConfReader::~TransmissionServiceConfReader (void)
{
}

TransmissionService::TRANSMISSION_SVC_MODE TransmissionServiceConfReader::getMode (void)
{
    const char *pszPropertyName = "aci.disService.propagationMode";
    if (_pCfgMgr->hasValue(pszPropertyName)) {
        String mode (_pCfgMgr->getValue (pszPropertyName));
        mode.convertToUpperCase();
        if (mode.length() > 0) {
            if (wildcardStringCompare (mode, "*BROADCAST")) {
                return TransmissionService::BROADCAST;
            }
            if (wildcardStringCompare (mode, "*MULTICAST")) {
                return TransmissionService::MULTICAST;
            }
            if (1 == (mode == "NORM")) {
                return TransmissionService::NORM;
            }
        }
        checkAndLogMsg ("TransmissionServiceConfReader::getMode", Logger::L_SevereError,
                        "Mode %s is not supported! Quitting\n", mode.c_str());
        assert (false);
    }
    return TransmissionService::MULTICAST;
}

uint16 TransmissionServiceConfReader::getPort (void)
{
    const char *pszPropertyName = "aci.disService.networkMessageService.port";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        return (atoi(_pCfgMgr->getValue (pszPropertyName)));
    }
    checkAndLogMsg ("TransmissionServiceConfReader::getPort", Logger::L_Warning,
                    "Port was not set, the default value for it (%d) was set. "
                    "DisseminationService may not behave has expected\n",
                    TransmissionService::DEFAULT_DISSERVICE_MCAST_PORT);
    return TransmissionService::DEFAULT_DISSERVICE_MCAST_PORT;
}

uint8 TransmissionServiceConfReader::getMcastTTL (void)
{
    const char *pszPropertyName = "aci.disService.networkMessageService.mcastTTL";
    if (_pCfgMgr->hasValue (pszPropertyName)) {
        checkAndLogMsg ("TransmissionServiceConfReader::getMcastTTL", Logger::L_Info,
                        "multicast TTL was specified as %d\n",
                        _pCfgMgr->getValueAsInt (pszPropertyName));
        return (uint8) _pCfgMgr->getValueAsInt (pszPropertyName);
    }
    checkAndLogMsg ("TransmissionServiceConfReader::getMcastTTL", Logger::L_Warning,
                    "TTL was not set, the default value for it (%d) was set. "
                    "DisseminationService may not behave has expected\n",
                    TransmissionService::DEFAULT_DISSERVICE_TTL);
    return TransmissionService::DEFAULT_DISSERVICE_TTL;
}

const char * TransmissionServiceConfReader::getBcastAddr (void)
{
    const char *pszRet = _pCfgMgr->getValue ("aci.disService.networkMessageService.bcastAddr",
                                             TransmissionService::DEFAULT_DISSERVICE_BCAST_ADDR);
    checkAndLogMsg ("TransmissionServiceConfReader::getBcastAddr", Logger::L_Info,
                    "Multicast Group Address set to %s\n", pszRet);

    return pszRet;
}

const char * TransmissionServiceConfReader::getMcastGroup (void)
{
    const char *pszRet = _pCfgMgr->getValue ("aci.disService.networkMessageService.mcastAddr",
                                             TransmissionService::DEFAULT_DISSERVICE_MCAST_GROUP);
    checkAndLogMsg ("TransmissionServiceConfReader::getMcastGroup", Logger::L_Info,
                    "Multicast Group Address set to %s\n", pszRet);

    return pszRet;
}

bool TransmissionServiceConfReader::getAsyncDelivery (void)
{
    bool bAsyncDelivery = _pCfgMgr->getValueAsBool ("aci.disService.networkMessageService.delivery.async",
                                                    TransmissionService::DEFAULT_ASYNCHRONOUS_DELIVERY);
    checkAndLogMsg ("TransmissionServiceConfReader::getAsyncDelivery", Logger::L_Info,
                    "Asynchronous delivery %s\n", bAsyncDelivery ? "ENABLED" : "DISABLED");

    return bAsyncDelivery;
}

bool TransmissionServiceConfReader::getAsyncTransmission (void)
{
    bool bAsyncTransmission = _pCfgMgr->getValueAsBool ("aci.disService.networkMessageService.transmission.async",
                                                        TransmissionService::DEFAULT_ASYNCHRONOUS_TRANSMISSION);
    checkAndLogMsg ("TransmissionServiceConfReader::getAsyncTransmission", Logger::L_Info,
                    "Asynchronous transmission %s\n", bAsyncTransmission ? "ENABLED" : "DISABLED");

    return bAsyncTransmission;
}

uint8 TransmissionServiceConfReader::getNetworkMessageVersion (void)
{
    const char *pszPropertyName = "aci.disService.networkMessageService.networkMessageVersion";
    uint8 ui8MsgVersion = (uint8) _pCfgMgr->getValueAsInt (pszPropertyName,
                                  TransmissionService::DEFAULT_MESSAGE_VERSION);
    checkAndLogMsg ("TransmissionServiceConfReader::getNetworkMessageVersion", Logger::L_Info,
                    "NetworkMessageVersion not specified. USing default value %u\n",
                    ui8MsgVersion);
    return ui8MsgVersion;
}

