/*
 * BandwidthSharing.cpp
 *
 *  This file is part of the IHMC DisService Library/Component
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

#include "BandwidthSharing.h"

#include "PeerState.h"
#include "TransmissionService.h"
#include "StringHashtable.h"

#include "NICInfo.h"
#include "NetUtils.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

BandwidthSharing::BandwidthSharing (PeerState *pPeerState, TransmissionService *pTrSvc)
{
   _pPeerState = pPeerState;
   _pTrSvc = pTrSvc;
   _activeRule = BS_EqualSharing;
   _bRequireQueueSizes = false;
   _ui32MinGuaranteedBw = 10 * 1024 /8;//10 Kbps
}

BandwidthSharing::~BandwidthSharing()
{
    _pPeerState = NULL;
    _pTrSvc = NULL;
}

void BandwidthSharing::adjustBandwidthShare (uint8 ui8Importance)
{
    uint32 ui32RateLimit;
    char **ppszInterfaces = _pTrSvc->getActiveInterfacesAddress();
    if (ppszInterfaces != NULL) {
        for (int i = 0; ppszInterfaces[i]; i++) {
            ui32RateLimit = calculateRateLimitByInterface (ppszInterfaces[i], ui8Importance);
            checkAndLogMsg ("BandwidthSharing::adjustBandwidthShare", Logger::L_Warning,
                                            "Rate limit: %u \n",
                                            ui32RateLimit);
            _pTrSvc->setTransmitRateLimit (ppszInterfaces[i], NULL, ui32RateLimit);
            free (ppszInterfaces[i]);
            ppszInterfaces[i] = NULL;
        }
        free (ppszInterfaces);
        ppszInterfaces = NULL;
    }
}

bool BandwidthSharing::isQueueSizeRequired (void)
{
    return _bRequireQueueSizes;
}

uint32 BandwidthSharing::calculateRateLimitByInterface (char *pszInterfaceName, uint8 ui8Importance)
{
    uint32 ui32RateLimit;
    switch (_activeRule) {
            case BS_EqualSharing:
                ui32RateLimit =  sharingRuleEqualSharing (pszInterfaceName);
                break;

            case BS_HigherPriorityDominates:
                ui32RateLimit =  sharingRuleHigherPriorityDominates (pszInterfaceName, ui8Importance);
                break;

            case BS_ProportionalSharing:
                ui32RateLimit = sharingRuleProportionalSharing (pszInterfaceName, ui8Importance);
                break;

            case BS_ProportionalSharingWithQueueLength:
                _m.lock();
                ui32RateLimit = sharingRuleProportionalSharingWithQueueLength (pszInterfaceName, ui8Importance);
                _m.unlock();
                break;

            case BS_BandwidthCappedPSWQL:
                _m.lock();
                ui32RateLimit = sharingRuleBandwidthCappedPSWQL (pszInterfaceName, ui8Importance);
                _m.unlock();
                break;

            case BS_BandwidthCappedHPD:
                ui32RateLimit = sharingRuleBandwidthCappedHPD (pszInterfaceName, ui8Importance);
                break;

            case BS_NoSharing:
                ui32RateLimit = sharingRuleNoSharing (pszInterfaceName);
                break;

            default:
                ui32RateLimit = 0;
                checkAndLogMsg ("BandwidthSharing::calculateRateLimitByInterface", Logger::L_Warning,
                                "Rule %u not found. Rate limit not applied\n", (uint32) _activeRule);
                break;
    }
    return maximum (ui32RateLimit, _ui32MinGuaranteedBw);
}

void BandwidthSharing::setSharingRule (int iSharingRule)
{
    switch (iSharingRule) {
        case 0:
            _activeRule = BS_EqualSharing;
            _bRequireQueueSizes = false;
            break;

        case 1:
            _activeRule = BS_HigherPriorityDominates;
            _bRequireQueueSizes = false;
            break;

        case 2:
            _activeRule = BS_ProportionalSharing;
            _bRequireQueueSizes = false;
            break;

        case 3:
            if ((_pTrSvc->getAsyncTransmission()) && (_pTrSvc->getSharesQueueLength())) {
                _activeRule = BS_ProportionalSharingWithQueueLength;
                _bRequireQueueSizes = true;
            }
            else {
                checkAndLogMsg ("BandwidthSharing::setSharingRule", Logger::L_Warning,
                                "Tried to set rule %u requiring queue length but either async transmission is disabled or the node doesn't share its queue length\n",
                                iSharingRule);
                _activeRule = BS_EqualSharing;
                _bRequireQueueSizes = false;
            }
            break;

        case 4:
            _activeRule = BS_BandwidthCappedHPD;
            _bRequireQueueSizes = false;
            break;

        case 5:
            if ((_pTrSvc->getAsyncTransmission()) && (_pTrSvc->getSharesQueueLength())) {
                _activeRule = BS_BandwidthCappedPSWQL;
                _bRequireQueueSizes = true;
            }
            else {
                checkAndLogMsg ("BandwidthSharing::setSharingRule", Logger::L_Warning,
                                "Tried to set rule %u requiring queue length but either async transmission is disabled or the node doesn't share its queue length\n",
                                iSharingRule);
                _activeRule = BS_EqualSharing;
                _bRequireQueueSizes = false;
            }
            break;

        case 255:
            _activeRule = BS_NoSharing;
            _bRequireQueueSizes = false;
            break;

        default:
            checkAndLogMsg ("BandwidthSharing::setSharingRule", Logger::L_Warning,
                            "Tried to set non-existent rule (%u). Keeping old rule (%u) \n",
                            iSharingRule, _activeRule);
            return;
            break;
    }
    checkAndLogMsg ("BandwidthSharing::setSharingRule", Logger::L_Info,
                    "Sharing rule set to %u\n", (uint32) _activeRule);
}

uint32 BandwidthSharing::sharingRuleEqualSharing (char *pszInterfaceName)
{
    //rate limit is equal to the network capacity divided by number of nodes, regardless
    //of importance
    uint32 ui32LinkCapacity = _pTrSvc->getLinkCapacity (pszInterfaceName);
    return ui32LinkCapacity / ((uint32) _pPeerState->getNumberOfActiveNeighbors() + 1);
}

uint32 BandwidthSharing::sharingRuleHigherPriorityDominates (char *pszInterfaceName, uint8 ui8Importance)
{
    /*
     * rate limit is equal to:
     *  for the nodes with highest importance -> network capacity / number of nodes with highest importance
     *  other nodes -> minimum guaranteed bw
     */
    uint32 ui32LinkCapacity = _pTrSvc->getLinkCapacity (pszInterfaceName);
    bool bNodesMoreImportantThanMeExist = false;
    uint16 ui16NodesWithTheSameImportanceAsMe = _pPeerState->getNumberOfActiveNeighborsWithImportance (ui8Importance, &bNodesMoreImportantThanMeExist);

    if (!bNodesMoreImportantThanMeExist) {
        return ui32LinkCapacity / ((uint32) ui16NodesWithTheSameImportanceAsMe + 1);
    }
    else {
        //return the minimum bandwidth possible. let's say it's 5Kbps = 5 * 1024 / 8 Bps =_ui32MinGuaranteedBw Bps
        return _ui32MinGuaranteedBw;
    }
}

//may need optimization
uint32 BandwidthSharing::sharingRuleBandwidthCappedHPD (char *pszInterfaceName, uint8 ui8Importance)
{
    /*
     * rate limit is equal to sharingRuleHigherPriorityDominates, but if the most important nodes
     * don't use all their share of bw, the remainder is divided equally between the lesser important nodes
     *
     * uses PeerState->bandwidth to estimate rate cap of a node
     */
    uint32 ui32LinkCapacity = _pTrSvc->getLinkCapacity (pszInterfaceName);
    uint32 ui32RateLimitCap = _pTrSvc->getTransmitRateLimitCap();

    int iNeighboursNumber = _pPeerState->getNumberOfActiveNeighbors();
    //at the start, assume this node is one of the most important ones
    int iNodesWithMaxImportance = 1;
    //the tot bw actually used by the most important nodes
    uint32 ui32TotBwForNodesWithMaxImportance = _pTrSvc->getTransmitRateLimitCap();
    //the 'most important' importance. (0 is the highest, 7 lowest)
    uint8 ui8MaxImportance = ui8Importance;
    unsigned int i;
    _pPeerState->lock();
    RemoteNodeInfo **ppRNIs = _pPeerState->getAllNeighborNodeInfos();
    if (ppRNIs != NULL) {
        for (i = 0; ppRNIs[i] != NULL; i++) {
            if ((_pPeerState->isActiveNeighbor(ppRNIs[i]->getNodeId()))) {
                //if there is another node more important than the current most important
                if (ppRNIs[i]->getNodeImportance() < ui8MaxImportance) {
                    iNodesWithMaxImportance = 1;
                    ui32TotBwForNodesWithMaxImportance = getRateCapFromPeerStateBandwidth (ppRNIs[i]->getBandwidth());
                    ui8MaxImportance = ppRNIs[i]->getNodeImportance();
                }
                //if there is another node with the same importance as the current most important
                else if (ppRNIs[i]->getNodeImportance() == ui8MaxImportance) {
                    ui32TotBwForNodesWithMaxImportance += getRateCapFromPeerStateBandwidth (ppRNIs[i]->getBandwidth());
                    iNodesWithMaxImportance++;
                }
            }
        }
        _pPeerState->release (ppRNIs);
    }
    _pPeerState->unlock();

    if (ui8MaxImportance < ui8Importance) { //if there are nodes more important than this
        //if there is some unused network capacity
        if (ui32LinkCapacity > ui32TotBwForNodesWithMaxImportance) {
            uint32 ui32Ris = (ui32LinkCapacity - ui32TotBwForNodesWithMaxImportance) / (iNeighboursNumber + 1 - iNodesWithMaxImportance);
            return ui32Ris;
        }
        else {
            return _ui32MinGuaranteedBw;
        }
    }
    else { //this is one of the most important nodes
        //if the capacity is higher than the sum of the rate limit caps, i can safely use the cap as rate limit
        if (ui32LinkCapacity > ui32TotBwForNodesWithMaxImportance) {
            return ui32RateLimitCap;
        }
        //else, just split the bw between the most important nodes
        else {
            return ui32LinkCapacity / iNodesWithMaxImportance;
        }
    }
}

uint32 BandwidthSharing::sharingRuleProportionalSharing (char *pszInterfaceName, uint8 ui8Importance)
{
    /*
     * the network capacity is shared proportionally to the node importance
     */
    uint32 ui32LinkCapacity = _pTrSvc->getLinkCapacity(pszInterfaceName);
    uint32 ui32NodeWeight = getNodeWeightPS (ui8Importance);
    uint32 ui32TotWeight = ui32NodeWeight;

    _pPeerState->lock();
    RemoteNodeInfo **ppRNIs = _pPeerState->getAllNeighborNodeInfos();
    if (ppRNIs != NULL) {
        for (unsigned int i = 0; ppRNIs[i] != NULL; i++) {
            if ((_pPeerState->isActiveNeighbor(ppRNIs[i]->getNodeId()))) {
                uint8 ui8RemoteNodeWeight = getNodeWeightPS (ppRNIs[i]->getNodeImportance());
                ui32TotWeight += ui8RemoteNodeWeight;
            }
        }
        _pPeerState->release (ppRNIs);
    }
    _pPeerState->unlock();

    ui32TotWeight = maximum(ui32TotWeight, (uint32) 1);
    uint32 ui32RateLimit = ui32LinkCapacity * ui32NodeWeight / ui32TotWeight;
    return ui32RateLimit;
}

//may need optimization
uint32 BandwidthSharing::sharingRuleProportionalSharingWithQueueLength (char *pszInterfaceName, uint8 ui8Importance)
{
    /*
     * the node capacity is split proportionally using queue length and importance as criteria
     */
    uint32 ui32LinkCapacity = _pTrSvc->getLinkCapacity(pszInterfaceName);
    uint8 ui8RescaledQueueLength = _pTrSvc->getRescaledTransmissionQueueSize (pszInterfaceName);
    uint32 ui32NodeWeight = getNodeWeightPSWQL (ui8RescaledQueueLength, ui8Importance);
    uint32 ui32TotWeight = ui32NodeWeight;

    checkAndLogMsg ("BandwidthSharing::sharingRuleProportionalSharingWithQueueLength", Logger::L_LowDetailDebug,
                                                                "This node has queue %u\n",
                                                                ui8RescaledQueueLength);
    _pPeerState->lock();
    RemoteNodeInfo **ppRNIs = _pPeerState->getAllNeighborNodeInfos();
    if (ppRNIs != NULL) {
        for (unsigned int i = 0; ppRNIs[i] != NULL; i++) {
            const char *pszNodeID = ppRNIs[i]->getNodeId();
            if (_pPeerState->isActiveNeighbor (pszNodeID)) {
                uint8 ui8QueueLength = _pTrSvc->getNeighborQueueSize (pszInterfaceName, _pPeerState->getIPAddress(pszNodeID));
                checkAndLogMsg ("BandwidthSharing::sharingRuleProportionalSharingWithQueueLength", Logger::L_LowDetailDebug,
                                                            "Node %s has queue %u\n",
                                                            pszNodeID, ui8QueueLength);
                uint32 ui32RemoteNodeWeight = getNodeWeightPSWQL (ui8QueueLength, ppRNIs[i]->getNodeImportance());
                ui32TotWeight += ui32RemoteNodeWeight;
            }
        }
        _pPeerState->release (ppRNIs);
    }
    _pPeerState->unlock();

    return ui32LinkCapacity * ui32NodeWeight / maximum (ui32TotWeight, (uint32) 1);
}

//may need optimization
uint32 BandwidthSharing::sharingRuleBandwidthCappedPSWQL (char *pszInterfaceName, uint8 ui8Importance)
{
    /*
     * like sharingRuleProportionalSharingWithQueueLength, but if a node doesn't use all the bandwidth assigned to it,
     * because of its rate cap limit, the unused capacity is shared proportionally with other nodes
     *
     * uses PeerState->bandwidth to estimate rate cap of a node
     */
    uint32 ui32LinkCapacity = _pTrSvc->getLinkCapacity(pszInterfaceName);

    uint8 ui8RescaledQueueLength = _pTrSvc->getRescaledTransmissionQueueSize (pszInterfaceName);
    uint32 ui32NodeWeight = getNodeWeightPSWQL (ui8RescaledQueueLength, ui8Importance);
    uint32 ui32TotWeight = ui32NodeWeight;

    _pPeerState->lock();
    RemoteNodeInfo **ppRNIs = _pPeerState->getAllNeighborNodeInfos();
    if (ppRNIs != NULL) {
        for (unsigned int i = 0; ppRNIs[i] != NULL; i++) {
            const char *pszNodeID = ppRNIs[i]->getNodeId();
            if (_pPeerState->isActiveNeighbor (pszNodeID)) {
                uint8 ui8QueueLength = _pTrSvc->getNeighborQueueSize (pszInterfaceName, _pPeerState->getIPAddress(pszNodeID));
                uint32 ui32RemoteNodeWeight = getNodeWeightPSWQL (ui8QueueLength, ppRNIs[i]->getNodeImportance());
                ui32TotWeight += ui32RemoteNodeWeight;
            }
        }
        _pPeerState->release (ppRNIs);
    }
    _pPeerState->unlock();

    ui32TotWeight = maximum (ui32TotWeight, (uint32) 1);
    //our preliminary rate limit
    uint32 ui32RateLimit = ui32LinkCapacity * ui32NodeWeight / ui32TotWeight;

    //our share of the capacity is >= than our maximum rate. we can just use that
    uint32 ui32RateCap = _pTrSvc->getTransmitRateLimitCap();
    if (ui32RateLimit >= ui32RateCap) {
        return ui32RateCap;
    }
    else { //our share is smaller than our maximum rate. let's see if someone else has spare bandwidth
        uint32 ui32RemoteNodeRateLimit;
        uint32 ui32SpareBandwidth = 0;
        uint32 ui32NotFullNodesWeightTotal = ui32NodeWeight;
        _pPeerState->lock();
        ppRNIs = _pPeerState->getAllNeighborNodeInfos();
        if (ppRNIs != NULL) {
            for (unsigned int i = 0; ppRNIs[i] != NULL; i++) {
                const char *pszNodeID = ppRNIs[i]->getNodeId();
                if (_pPeerState->isActiveNeighbor (pszNodeID)) {
                    uint8 ui8QueueLength= _pTrSvc->getNeighborQueueSize (pszInterfaceName, _pPeerState->getIPAddress(pszNodeID));
                    uint32 ui32RemoteNodeWeight = getNodeWeightPSWQL (ui8QueueLength, ppRNIs[i]->getNodeImportance());
                    ui32RemoteNodeRateLimit = ui32LinkCapacity * ui32RemoteNodeWeight / ui32TotWeight;
                    //if the node has a share bigger than its maximum limit
                    if (ui32RemoteNodeRateLimit > getRateCapFromPeerStateBandwidth (ppRNIs[i]->getBandwidth())) {
                        ui32SpareBandwidth += (ui32RemoteNodeRateLimit - getRateCapFromPeerStateBandwidth (ppRNIs[i]->getBandwidth()));
                    }
                    else {
                        ui32NotFullNodesWeightTotal += ui32RemoteNodeWeight;
                    }
                }
            }
            _pPeerState->release (ppRNIs);
        }
        _pPeerState->unlock();

        // if there is spare bw, we will redistribute it between the nodes that are not at the cap
        // let's add our share of this spare bw to get our final rate limit
        if (ui32SpareBandwidth > 0) {
            ui32RateLimit =  ui32RateLimit + ui32SpareBandwidth * ui32NodeWeight / ui32NotFullNodesWeightTotal;
        }
        return ui32RateLimit;
    }
}

uint32 BandwidthSharing::sharingRuleNoSharing (char *pszInterfaceName)
{
    return _pTrSvc->getLinkCapacity (pszInterfaceName);
}

uint8 BandwidthSharing::getQueuePriorityRange (uint8 ui8QueueLength)
{
    /*
     * queue length:
     *   0-31  -> range 7
     *  32-63  -> range 6
     *  64-95  -> range 5
     *  96-127 -> range 4
     * 128-159 -> range 3
     * 160-191 -> range 2
     * 192-223 -> range 1
     * 224-255 -> range 0
     */
    return (uint8)(7 - floor ((double) ui8QueueLength / 32));
}

uint32 BandwidthSharing::getRateCapFromPeerStateBandwidth (uint8  ui8Bandwidth)
{
    uint32 ui32ReturnValue;
    switch (ui8Bandwidth) {
        case PeerState::VERY_LOW : // 205 Kbps
            //26214 Bps
            ui32ReturnValue =  DisseminationService::DEFAULT_MIN_BANDWIDTH +
                   (DisseminationService::DEFAULT_MAX_BANDWIDTH - DisseminationService::DEFAULT_MIN_BANDWIDTH) / 5;
            break;

        case PeerState::LOW : // 256 Kbps
            //32768 Bps
            ui32ReturnValue = DisseminationService::DEFAULT_MIN_BANDWIDTH +
                   (DisseminationService::DEFAULT_MAX_BANDWIDTH - DisseminationService::DEFAULT_MIN_BANDWIDTH) / 4;
            break;

        case PeerState::NORMAL : // 341 Kbps
            //43691 Bps
            ui32ReturnValue = DisseminationService::DEFAULT_MIN_BANDWIDTH +
                   (DisseminationService::DEFAULT_MAX_BANDWIDTH - DisseminationService::DEFAULT_MIN_BANDWIDTH) / 3;
            break;

        case PeerState::HIGH : // 512 Kbps
            //65536 Bps
            ui32ReturnValue = DisseminationService::DEFAULT_MIN_BANDWIDTH +
                   (DisseminationService::DEFAULT_MAX_BANDWIDTH - DisseminationService::DEFAULT_MIN_BANDWIDTH) / 2;
            break;

        case PeerState::VERY_HIGH : // 1024 Kbps
            //131072 Bps
            ui32ReturnValue = DisseminationService::DEFAULT_MAX_BANDWIDTH;
            break;

        default:
            ui32ReturnValue = 0;
    };
    return ui32ReturnValue * 1024 / 8;//from Kbps to Bps
}

uint32 BandwidthSharing::getNodeWeightPS (uint8 ui8NodeImportance)
{
    return  2 * (8 - ui8NodeImportance);
}

uint32 BandwidthSharing::getNodeWeightPSWQL (uint8 ui8QueueLength, uint8 ui8NodeImportance)
{
    uint32 ui32Priority = (8 - getQueuePriorityRange (ui8QueueLength));
    return (8 - ui8NodeImportance) * ui32Priority * ui32Priority * ui32Priority;
}

