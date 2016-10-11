/*
 * NodeInfo.cpp
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

#include "NodeInfo.h"

#include "ConnectivityHistory.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DSSFLib.h"
#include "History.h"
#include "Message.h"
#include "MessageInfo.h"
#include "WorldState.h"

#include "Graph.h"
#include "Logger.h"
#include "InetAddr.h"
#include "NetUtils.h"
#include "NICInfo.h"
#include "Reader.h"
#include "Writer.h"
#include "ListenerNotifier.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//==============================================================================
// NodeInfo
//==============================================================================

NodeInfo::NodeInfo (const char *pszNodeId, uint8 ui8MemorySpace, uint8 ui8Bandwidth,
                    int64 i64WinSize)
    : StringHashthing (pszNodeId),
      _ui8MemorySpace (ui8MemorySpace),
      _ui8Bandwidth (ui8Bandwidth),
      _ui8NodeImportance (0),
      _nodeId (pszNodeId),
      _connectivityHistory (i64WinSize)
{
}

NodeInfo::~NodeInfo()
{
    InetAddrWrapper *pCurrIfaceAddr, *pTmpIfaceAddr;
    pTmpIfaceAddr = _ipAddresses.getFirst();
    while ((pCurrIfaceAddr = pTmpIfaceAddr) != NULL) {
        pTmpIfaceAddr = _ipAddresses.getNext();
        delete _ipAddresses.remove (pCurrIfaceAddr);
    }
}

bool NodeInfo::setIPAddress (const char *pszIPAddress)
{
    if (pszIPAddress == NULL) {
        return false;
    }
    InetAddrWrapper *pIfaceWr = new InetAddrWrapper (pszIPAddress);
    if (pIfaceWr == NULL) {
        return false;
    }
    pIfaceWr->i64MostRecentMessageRcvdTime = getTimeInMilliseconds();
    if (_ipAddresses.search (pIfaceWr) == NULL) {
        // the list is sorted in descending value of i64MostRecentMessageRcvdTime,
        // I can therefore pre-pend pIfaceWr
        _ipAddresses.prepend (pIfaceWr);
        return true;
    }
    else {
        // Remove current entry and add new one
        delete _ipAddresses.remove (pIfaceWr);
        // the list is sorted in descending value of i64MostRecentMessageRcvdTime,
        // I can therefore pre-pend pIfaceWr
        _ipAddresses.prepend (pIfaceWr);
        return false;
    }
}

bool NodeInfo::setIPAddress (uint32 ui32IpAddress)
{
    InetAddr addr (ui32IpAddress);
    return setIPAddress (addr.getIPAsString());
}

const char * NodeInfo::getNodeId() const
{
    return (const char *) _nodeId;
}

uint32 NodeInfo::getDefaultIPAddress()
{
    InetAddrWrapper *pIfaceWr = _ipAddresses.getFirst();
    if (pIfaceWr == NULL) {
        return 0;
    }
    InetAddr addr (pIfaceWr->ifaceAddr);
    return addr.getIPAddress();
}

const char * NodeInfo::getDefaultIPAddressAsString()
{
    InetAddrWrapper *pIfaceWr = _ipAddresses.getFirst();
    if (pIfaceWr == NULL) {
        return NULL;
    }

    return pIfaceWr->ifaceAddr.c_str();
}

const char ** NodeInfo::getIPAddressesAsStrings()
{
    int iCount = _ipAddresses.getCount();
    if (iCount <= 0) {
        return NULL;
    }
    const char **ppIPAddresses = (const char **) calloc (sizeof (char *), iCount+1);
    InetAddrWrapper *pIfaceWr = _ipAddresses.getFirst();
    for (int i = 0; pIfaceWr != NULL; i++) {
        ppIPAddresses[i] = pIfaceWr->ifaceAddr.c_str();
        pIfaceWr = _ipAddresses.getNext();
    }
    return ppIPAddresses;
}

//------------------------------------------------------------------------------

void NodeInfo::setGraph (Graph *pGraph)
{
    StringHashthing::setGraph (pGraph);
}

Graph * NodeInfo::getGraph()
{
    return StringHashthing::getGraph();
}

const char * NodeInfo::getId()
{
    return StringHashthing::getId();
}

Thing * NodeInfo::put (const char *pszId, Thing *pThing)
{
    return StringHashthing::put (pszId, pThing);
}

bool NodeInfo::contains (const char *pszKey)
{
    return StringHashthing::contains (pszKey);
}

Thing * NodeInfo::getThing (const char *pszKey)
{
    return StringHashthing::getThing (pszKey);
}

Thing * NodeInfo::getParent()
{
    return StringHashthing::getParent();
}

bool NodeInfo::isReachable (const char *pszKey)
{
    return StringHashthing::isReachable (pszKey);
}

bool NodeInfo::isReachable (const char **ppszInterfaces, NICInfo **ppNICInfos)
{
    InetAddrWrapper *pIfaceWr = _ipAddresses.getFirst();
    if (pIfaceWr == NULL || ppNICInfos == NULL) {
        return false;
    }
    do {
        for (int i = 0; ppNICInfos[i]; i++) {
            InetAddr addr (pIfaceWr->ifaceAddr.c_str());
            if (NetUtils::areInSameNetwork (ppNICInfos[i]->ip.s_addr, ppNICInfos[i]->netmask.s_addr,
                                            addr.getIPAddress(), ppNICInfos[i]->netmask.s_addr)) {
                return true;
            }
        }
    } while ((pIfaceWr = _ipAddresses.getNext()) != NULL);
    return false;
}

bool NodeInfo::isReachable (PtrLList<String> *pList, const char *pszKey)
{
    return StringHashthing::isReachable (pList, pszKey);
}

Thing * NodeInfo::remove (const char *pszKey)
{
    return StringHashthing::remove (pszKey);
}

PtrLList<Thing> * NodeInfo::list()
{
    return StringHashthing::list();
}

StringHashtable<Thing>::Iterator NodeInfo::iterator()
{
    return StringHashthing::iterator();
}

Thing * NodeInfo::clone()
{
    return StringHashthing::clone();
}

Thing * NodeInfo::deepClone()
{
    return StringHashthing::deepClone();
}       

double NodeInfo::getWeight()
{
    return StringHashthing::getWeight();
}

int NodeInfo::read (Reader *pReader, uint32 ui32MaxSize)
{
    return StringHashthing::read (pReader, ui32MaxSize);
}

int NodeInfo::write (Writer *pWriter, uint32 ui32MaxSize)
{
    return StringHashthing::write (pWriter, ui32MaxSize);
}

void NodeInfo::setNodeImportance (uint8 ui8NodeImportance)
{
    _ui8NodeImportance = ui8NodeImportance;
}

uint8 NodeInfo::getNodeImportance (void)
{
    return _ui8NodeImportance;
}

bool NodeInfo::moreImportantThan (uint8 ui8Importance)
{
    return (_ui8NodeImportance < ui8Importance);
}

//==========================================================================
// RemoteNodeInfo
//==========================================================================

RemoteNodeInfo::RemoteNodeInfo (const char *pszNodeId, uint32 ui32CurrentTopologyStateSeqId,
                                uint16 ui16CurrentSubscriptionStateCRC, uint32 ui32CurrentDataCacheStateSeqId)
    : NodeInfo (pszNodeId),
      _bTopologyStateEnabled (false),
      _ui8NumberOfActiveNeighbors (0),
      _ui8NodesInConnectivityHistory (0),
      _ui8NodesRepetitivity (0),
      _ui8NodeOccurrence (0),
      _ui32ExpectedTopologyStateSeqId ((ui32CurrentTopologyStateSeqId == 1) ? 2 : ui32CurrentTopologyStateSeqId),
      _ui16ExpectedSubscriptionStateCRC (ui16CurrentSubscriptionStateCRC),
      _ui32ExpectedDataCacheStateSeqId (ui32CurrentDataCacheStateSeqId),
      _i64MostRecentMessageRcvdTime (0),
      _pSubscriptionStateTable (NULL),
      _pRemoteSubscriptions (NULL),
      _pIndirectProbabilities (NULL)
{
}

RemoteNodeInfo::RemoteNodeInfo (const char *pszNodeId, SubscriptionList *pRemoteSubscriptions)
    : NodeInfo (pszNodeId),
      _bTopologyStateEnabled (false),
      _ui8NumberOfActiveNeighbors (0),
      _ui8NodesInConnectivityHistory (0),
      _ui8NodesRepetitivity (0),
      _ui8NodeOccurrence (0),
      _ui32ExpectedTopologyStateSeqId (0),
      _ui16ExpectedSubscriptionStateCRC (0),
      _ui32ExpectedDataCacheStateSeqId (0),
      _i64MostRecentMessageRcvdTime (0),
      _pSubscriptionStateTable (NULL),
      _pRemoteSubscriptions (pRemoteSubscriptions),
      _pIndirectProbabilities (NULL)
{
}

RemoteNodeInfo::~RemoteNodeInfo()
{
    delete _pRemoteSubscriptions;
    _pRemoteSubscriptions = NULL;
    delete _pIndirectProbabilities;
    _pIndirectProbabilities = NULL;
}

void RemoteNodeInfo::setExpectedTopologyStateSeqId (uint32 ui32CurrentTopologyStateSeqId)
{
    _ui32ExpectedTopologyStateSeqId = ui32CurrentTopologyStateSeqId;
}

void RemoteNodeInfo::setExpectedSubscriptionStateCRC (uint16 ui16CurrentSubscriptionStateCRC)
{
    _ui16ExpectedSubscriptionStateCRC = ui16CurrentSubscriptionStateCRC;
}

void RemoteNodeInfo::setExpectedDataCacheStateSeqId (uint32 ui32CurrentDataCacheStateSeqId)
{
    _ui32ExpectedDataCacheStateSeqId = ui32CurrentDataCacheStateSeqId;
}

bool RemoteNodeInfo::setMostRecentMessageRcvdTime (const char *pszRemoteAddr)
{
    if (pszRemoteAddr == NULL) {
        return false;
    }
    InetAddrWrapper *pIfaceWr = new InetAddrWrapper (pszRemoteAddr);
    if (pIfaceWr == NULL) {
        return false;
    }
    pIfaceWr->i64MostRecentMessageRcvdTime = getTimeInMilliseconds();

    InetAddrWrapper *pIfaceWrOld = _ipAddresses.remove (pIfaceWr);
    bool bNewLink = (pIfaceWrOld == NULL);
    delete pIfaceWrOld;

    _ipAddresses.prepend (pIfaceWr);
    return bNewLink;
}

void RemoteNodeInfo::setTopologyStateEnabled (bool bTopologyStateEnabled)
{
    _bTopologyStateEnabled = bTopologyStateEnabled;
}

void RemoteNodeInfo::incrementOccurrence (void)
{
    if (_ui8NodeOccurrence < 255) {
        _ui8NodeOccurrence++;
    }
}

uint32 RemoteNodeInfo::getExpectedTopologyStateSeqId (void)
{
    return _ui32ExpectedTopologyStateSeqId;
}

uint16 RemoteNodeInfo::getExpectedSubscriptionStateCRC (void)
{
    return _ui16ExpectedSubscriptionStateCRC;
}

uint32 RemoteNodeInfo::getExpectedDataCacheStateSeqId (void)
{
    return _ui32ExpectedDataCacheStateSeqId;
}

int64 RemoteNodeInfo::getMostRecentMessageRcvdTime (void)
{
    InetAddrWrapper *pIfaceWr = _ipAddresses.getFirst();
    if (pIfaceWr == NULL) {
        return 0;
    }
    // pIfaceWr is sorted in descending order, so the first element is the most
    // recent one
    return pIfaceWr->i64MostRecentMessageRcvdTime;
}

uint8 RemoteNodeInfo::getOccurrence (void)
{
    return _ui8NodeOccurrence;
}

bool RemoteNodeInfo::isTopologyStateEnabled (void) const
{
    return _bTopologyStateEnabled;
}

bool RemoteNodeInfo::isNodeConfigurationEnabled (void)
{
    if (_ui8NumberOfActiveNeighbors == 0) {
        return false;
    }
    return true;
}

int RemoteNodeInfo::getAndRemoveLinksToDrop (uint32 ui32DeadPeerIntervalTime, DArray2<String> &linksToDrop)
{
    InetAddrWrapper *pIfaceWr, *pIfaceNext;
    pIfaceNext = _ipAddresses.getFirst();
    while ((pIfaceWr = pIfaceNext) != NULL) {
        pIfaceNext = _ipAddresses.getNext();
        if ((getTimeInMilliseconds() - pIfaceWr->i64MostRecentMessageRcvdTime) > ui32DeadPeerIntervalTime) {
            linksToDrop[linksToDrop.firstFree()] = pIfaceWr->ifaceAddr;
            pIfaceWr = _ipAddresses.remove (pIfaceWr);
        }
    }
    return 0;
}

NOMADSUtil::StringHashtable<uint32> * RemoteNodeInfo::getRemoteSubscriptionStateTable (void)
{
    return _pSubscriptionStateTable;
}

void  RemoteNodeInfo::setRemoteSubscriptionStateTable (NOMADSUtil::StringHashtable<uint32> *pSubscriptionStateTable)
{
    _pSubscriptionStateTable = pSubscriptionStateTable;
}

int RemoteNodeInfo::subscribe (const char *pszGroupName, Subscription *pSubscription)
{
    if (_pRemoteSubscriptions == NULL) {
        _pRemoteSubscriptions = new SubscriptionList();
    }
    if (!_pRemoteSubscriptions->hasGenericSubscription (pszGroupName)) {
        _pRemoteSubscriptions->addSubscription (pszGroupName, pSubscription);   
    }
    else {
        _pRemoteSubscriptions->removeGroup (pszGroupName);
        _pRemoteSubscriptions->addSubscription (pszGroupName, pSubscription);  
    }
    return 0;
}

Subscription * RemoteNodeInfo::getSubscription (const char *pszGroupName)
{
    if (_pRemoteSubscriptions != NULL) {
        Subscription *pSub = _pRemoteSubscriptions->getSubscription (pszGroupName);
        return pSub;
    }
    return NULL;
}

PtrLList<Subscription> * RemoteNodeInfo::getSubscriptionWild (const char *pszTemplate)
{
    if (_pRemoteSubscriptions != NULL) {
        PtrLList<Subscription> *pRet = _pRemoteSubscriptions->getSubscriptionWild (pszTemplate);
        return pRet;
    }
    return NULL;
}

// Return true if the RemoteNode contain a not NULL subscrition with the  pszGroupName
bool RemoteNodeInfo::hasSubscription (const char *pszGroupName)
{
    if (_pRemoteSubscriptions != NULL && _pRemoteSubscriptions->hasGenericSubscriptionWild (pszGroupName)) {
        return true;
    }
    return false;
}

int RemoteNodeInfo::unsubscribe (const char *pszGroupName)
{
    if (_pRemoteSubscriptions == NULL || !_pRemoteSubscriptions->hasGenericSubscription (pszGroupName)) {
        return -1;
    }
    else {
        int ret = _pRemoteSubscriptions->removeGroup (pszGroupName);
        return ret;
    }
}

PtrLList<String> * RemoteNodeInfo::getAllSubscribedGroups()
{
    if (_pRemoteSubscriptions == NULL) {
        return NULL;
    }
    else {
        PtrLList<String> *pRet = _pRemoteSubscriptions->getAllSubscribedGroups();
        return pRet;
    }
}

void RemoteNodeInfo::printAllSubscribedGroups (void)
{
    PtrLList<String> *pGroupNames = getAllSubscribedGroups();
    if (pGroupNames) {
        for (String *pGroupName = pGroupNames->getFirst(); pGroupName; pGroupName = pGroupNames->getNext()) {
            checkAndLogMsg ("RemoteNodeInfo::printAllSubscribedGroups", Logger::L_Info, "       Subscribed group: %s\n", pGroupName->c_str());
        }
    }
}

int RemoteNodeInfo::unsubscribeAll (void)
{
    if (_pRemoteSubscriptions != NULL) {
        delete _pRemoteSubscriptions;
        _pRemoteSubscriptions = NULL;
    }
    return 0;
}

SubscriptionList * RemoteNodeInfo::getRemoteSubscriptions (void)
{
    return _pRemoteSubscriptions;
}

SubscriptionList * RemoteNodeInfo::getRemoteSubscriptionsCopy (void)
{
    // Returns a copy of _pRemoteSubscriptions
    SubscriptionList *pRemoteSubscriptionsCopy = NULL;
    if (_pRemoteSubscriptions) {
        pRemoteSubscriptionsCopy = new SubscriptionList();
        for (StringHashtable<Subscription>::Iterator i = _pRemoteSubscriptions->getIterator(); !i.end(); i.nextElement()) {
            pRemoteSubscriptionsCopy->addSubscription (i.getKey(), (i.getValue())->clone()); // Clone subscription before adding it
        }
    }
    return pRemoteSubscriptionsCopy;
}

bool RemoteNodeInfo::isNodeInterested (DisServiceDataMsg *pDSDMsg)
{
    // Returns true if the node is interested in pDSDMsg
    bool bFound = false;
    const char * pszMsgGroupName = pDSDMsg->getMessageHeader()->getGroupName();
    if (_pRemoteSubscriptions) {
        for (StringHashtable<Subscription>::Iterator i = _pRemoteSubscriptions->getIterator(); !i.end(); i.nextElement()) {
            const char *pszSubGroupName = i.getKey();
            if (0 == stricmp (pszSubGroupName, pszMsgGroupName)) { 
                Subscription *pSub = i.getValue();
                if ((pSub->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) || pSub->matches(pDSDMsg->getMessage())) {
                    bFound = true;
                }
            }
        }
    }
    return bFound;
}

void RemoteNodeInfo::printRemoteNodeInfo (void)
{
    checkAndLogMsg ("RemoteNodeInfo::printRemoteNodeInfo", Logger::L_Info, "         Node id: %s\n", getNodeId());
    printIndirectProbabilities();
    if (getAllSubscribedGroups()) {
        checkAndLogMsg ("RemoteNodeInfo::printRemoteNodeInfo", Logger::L_Info, "         %d SUBSCRIBED GROUPS:\n", getAllSubscribedGroups()->getCount());
        printAllSubscribedGroups();
        checkAndLogMsg ("RemoteNodeInfo::printRemoteNodeInfo", Logger::L_Info, "         REMOTE SUBSCRIPTIONS:\n");
        for (StringHashtable<Subscription>::Iterator i = getRemoteSubscriptions()->getIterator(); !i.end(); i.nextElement()) {
            checkAndLogMsg ("RemoteNodeInfo::printRemoteNodeInfo", Logger::L_Info, "         SUBSCRIPTION:\n");
            checkAndLogMsg ("RemoteNodeInfo::printRemoteNodeInfo", Logger::L_Info, "         groupname %s\n", i.getKey());
            (i.getValue())->printInfo(); // Print subscription info
        }
    }
}

/**
 * Topology
 */

// Indirect encounters

NOMADSUtil::StringFloatHashtable * RemoteNodeInfo::getIndirectProbabilities (void) 
{
    return _pIndirectProbabilities;
}

void RemoteNodeInfo::setIndirectProbabilities (NOMADSUtil::StringFloatHashtable *pIndirectProbabilities) 
{
    _pIndirectProbabilities = pIndirectProbabilities;
}

void RemoteNodeInfo::ageIndirectProbabilities (float fAgeParam, float fProbThreshold, int iTimeSinceLastAging) 
{
    // Ages the indirect probabilities
    // TODO: find a better way, instead of creating a temporary table
    // _pIndirectProbabilities: gatewayNodeId->probValue
    if (_pIndirectProbabilities) {
        StringFloatHashtable * pIndProbTemp = new StringFloatHashtable();
        for (StringFloatHashtable::Iterator iNodes = _pIndirectProbabilities->getAllElements(); !iNodes.end(); iNodes.nextElement()) {
            float fProb = *(iNodes.getValue()) * pow (fAgeParam, iTimeSinceLastAging);
            if (fProb > fProbThreshold) {
                float *pProb = &fProb;
                pIndProbTemp->put (iNodes.getKey(), pProb);
            }
        }
        delete _pIndirectProbabilities;
        _pIndirectProbabilities = pIndProbTemp;
    }
}

void RemoteNodeInfo::printIndirectProbabilities (void)
{
    if (_pIndirectProbabilities) {
        for (StringFloatHashtable::Iterator iNodes = _pIndirectProbabilities->getAllElements(); !iNodes.end(); iNodes.nextElement()) {
            checkAndLogMsg ("RemoteNodeInfo::printIndirectProbabilities", Logger::L_Info, "       Through gateway node: %s --> probability %f\n", iNodes.getKey(), *(iNodes.getValue()));
        }
    }
}

void RemoteNodeInfo::addIndirectProbability (const char *pszNeighborNodeId, float fIndProb, float fAddParam, float fProbThreshold)
{
    // Adds indirect probability to reach the specific RemoteNode
    // The gateway node is pszNeighborNodeId
    if (_pIndirectProbabilities == NULL) {
        _pIndirectProbabilities = new StringFloatHashtable();
    }
    float fProb = fIndProb * fAddParam;
    if (fProb > fProbThreshold) {
        float *pProb = &fProb;
        _pIndirectProbabilities->put (pszNeighborNodeId, pProb);
    } else {
        _pIndirectProbabilities->remove (pszNeighborNodeId);
    }
}

//==============================================================================
// LocalNodeInfo
//==============================================================================

LocalNodeInfo::LocalNodeInfo (const char *pszNodeId, uint8 ui8MemorySpace, uint8 ui8Bandwidth, int64 i64WinSize)
    : NodeInfo (pszNodeId, ui8MemorySpace, ui8Bandwidth, i64WinSize),
      _m (30),
      _localSubscriptions (US_INITSIZE, // ulInitSize
                           true),       // bDelValues
      _pubState (true,  // bCaseSensitiveKeys
                 true,  // bool bCloneKeys
                 true,  // bool bDeleteKeys
                 true)  // bool bDeleteValues
{
}

LocalNodeInfo::~LocalNodeInfo()
{
    _localSubscriptions.removeAll();
}

int LocalNodeInfo::deregisterAllGroupMembershipListeners (void)
{
    return _notifier.deregisterAllListeners();
}

int LocalNodeInfo::deregisterGroupMembershipListener (unsigned int uiIndex)
{
    return _notifier.deregisterListener (uiIndex);
}

int LocalNodeInfo::registerGroupMembershipListener (GroupMembershipListener *pListener)
{
    return _notifier.registerListener (pListener);
}

uint32 LocalNodeInfo::getSubscriptionStateSequenceID (void) const
{
    return ui32SubscriptionStateSeqID;
}

int LocalNodeInfo::subscribe (uint16 ui16ClientId, const char *pszGroupName, Subscription *pSubscription)
{
    _m.lock (300);
    if (pszGroupName == NULL || pSubscription == NULL) {
        delete pSubscription;
        _m.unlock (300);
        return -1;
    }

    char *pszOnDemandDataGroupName = getOnDemandDataGroupName (pszGroupName);
    if (pszOnDemandDataGroupName == NULL) {
        // The on-demand subscription group name could not be generated
        delete pSubscription;
        _m.unlock (300);
        return -2;
    }

    Subscription *pOnDemandSubscription = pSubscription->getOnDemandSubscription();
    if (pOnDemandSubscription == NULL) {
        free (pszOnDemandDataGroupName);
        delete pSubscription;
        _m.unlock (300);
        return -3;
    }

    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL == NULL) {
        pSL = new SubscriptionList();
        _localSubscriptions.put (ui16ClientId, pSL);
    }

    if (pSL->addGroup (pszGroupName, pSubscription) != 0) {
        // The subscription could not be added
        free (pszOnDemandDataGroupName);
        delete pSubscription;
        delete pOnDemandSubscription;
        _m.unlock (300);
        return -4;
    }

    if (pSL->addGroup (pszOnDemandDataGroupName, pOnDemandSubscription) != 0) {
        // The on-demand subscription could not be added
        free (pszOnDemandDataGroupName);
        delete pOnDemandSubscription;
        pSL->removeGroup (pszGroupName, pSubscription);    // Should delete pSubscription
        _m.unlock (300);
        return -5;
    }

    // The subscription was added, check if the ConsolidateList needs to be modified too.
    updateConsolidateSubsciptionList (pszGroupName, pSubscription);

    // The subscription was added, check if the ConsolidateList needs to be modified too.
    updateConsolidateSubsciptionList (pszOnDemandDataGroupName, pOnDemandSubscription);
    free (pszOnDemandDataGroupName);

    _m.unlock (300);
    return 0;
}

int LocalNodeInfo::addFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    _m.lock (301);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL == NULL) {
        _m.unlock (301);
        return -1;
    }
    if (pSL->addFilterToGroup (pszGroupName, ui16Tag) != 0) {
        _m.unlock (301);
        return -2;
    }
    bool bAddFilter = true;

    // I can add the filter to the ConsolidatedSubscriptions only if every client subscribing that group has the filter too
    for (UInt32Hashtable<SubscriptionList>::Iterator i = _localSubscriptions.getAllElements(); !i.end(); i.nextElement()) {
        SubscriptionList *pSL = i.getValue();
        Subscription *pS = pSL->getSubscription(pszGroupName);
        switch (pS->getSubscriptionType()) {
            case Subscription::GROUP_SUBSCRIPTION: {
                if (!((GroupSubscription *)pS)->hasFilter(ui16Tag)) {
                    bAddFilter = false;
                    break;
                }
                break;
            }
            case Subscription::GROUP_TAG_SUBSCRIPTION: {
                if (((GroupTagSubscription *)pS)->hasTag(ui16Tag)) {
                    bAddFilter = false;
                    break;
                }
                break;
            }
            case Subscription::GROUP_PREDICATE_SUBSCRIPTION :
                 bAddFilter = false;
                 break;
        }
    }
    if (bAddFilter) {
        _consolidatedSubscriptions.addFilterToGroup (pszGroupName, ui16Tag);
    }

    _m.unlock (301);
    return 0;
}

int LocalNodeInfo::modifyGroupPriority (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8NewPriority)
{
    _m.lock (302);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL == NULL) {
        _m.unlock (302);
        return -1;
    }
    int ret = pSL->modifyPriority (pszGroupName, 0,ui8NewPriority);
    _m.unlock (302);
    return ret;
}

int LocalNodeInfo::removeFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    _m.lock (303);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if ((pSL != NULL) && (pSL->removeFilterFromGroup (pszGroupName, ui16Tag) == 0)) {
        _consolidatedSubscriptions.removeFilterFromGroup (pszGroupName, ui16Tag);
        _m.unlock (303);
        return 0;
    }
    _m.unlock (303);
    return -1;
}

int LocalNodeInfo::removeAllFilters (uint16 ui16ClientId, const char *pszGroupName)
{
    _m.lock (304);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if ((pSL != NULL) && (pSL->removeAllFiltersFromGroup (pszGroupName) == 0)) {
        _consolidatedSubscriptions.removeAllFiltersFromGroup (pszGroupName);
        _m.unlock (304);
        return 0;
    }
    _m.unlock (304);
    return -1;
}

int LocalNodeInfo::unsubscribe (uint16 ui16ClientId, const char *pszGroupName)
{
    char *pszOnDemandDataGroupName = getOnDemandDataGroupName (pszGroupName);
    if (pszOnDemandDataGroupName == NULL) {
        // The on-demand subscription group name could not be generated
        return -1;
    }
    _m.lock (305);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL != NULL) {
        if ((pSL->removeGroup (pszGroupName) == 0) && (pSL->removeGroup (pszOnDemandDataGroupName) == 0)) {
            recomputeConsolidateSubsciptionList();
            free (pszOnDemandDataGroupName);
            _m.unlock (305);
            return 0;
        }
    }
    free (pszOnDemandDataGroupName);
    _m.unlock (305);
    return -2;
}

int LocalNodeInfo::unsubscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    char *pszOnDemandDataGroupName = getOnDemandDataGroupName (pszGroupName);
    if (pszOnDemandDataGroupName == NULL) {
        // The on-demand subscription group name could not be generated
        return -1;
    }
    _m.lock (306);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    int rc = 0;
    if (pSL == NULL) {
        rc = -2;
    }
    else {
        Subscription *pSub = pSL->getSubscription (pszGroupName);
        if (pSub == NULL) {
            rc = -3;
        }
        else {
            uint8 ui8SubscriptionType = pSub->getSubscriptionType();
            if ((ui8SubscriptionType == Subscription::GROUP_SUBSCRIPTION) && (ui16Tag == 0)) {
                if (pSL->removeGroup (pszGroupName)) {
                    rc = -4;
                }
                else {
                    if (pSL->removeGroup (pszOnDemandDataGroupName)) {
                        rc = -5;
                    }
                }
            }
            else if ((ui8SubscriptionType == Subscription::GROUP_TAG_SUBSCRIPTION) && (ui16Tag != 0)) {
                if (pSL->removeGroupTag (pszGroupName, ui16Tag)) {
                    rc = -6;
                }
                else {
                    if (pSL->removeGroupTag (pszOnDemandDataGroupName, ui16Tag)) {
                        rc = -7;
                    }
                }
            }
        }
    }
    if (rc == 0) {
        recomputeConsolidateSubsciptionList ();
    }
    free (pszOnDemandDataGroupName);
    _m.unlock (306);
    return rc;
}

int LocalNodeInfo::addHistory (uint16 ui16ClientId, const char *pszGroupName, History *pHistory)
{
    _m.lock (307);
    int rc = addHistoryInternal (ui16ClientId, pszGroupName, pHistory);
    _m.unlock (307);
    return rc;
}

int LocalNodeInfo::addHistoryInternal (uint16 ui16ClientId, const char *pszGroupName, History *pHistory)
{
    Subscription *pSubscription = getSubscriptionForClient (ui16ClientId, pszGroupName);
    if (pSubscription == NULL) {
        return -1;
    }
    if (pSubscription->getSubscriptionType() != Subscription::GROUP_SUBSCRIPTION) {
        return -2;
    }
   return pSubscription->addHistory (pHistory);
}

int LocalNodeInfo::addHistory (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, History *pHistory)
{
    _m.lock (308);
    if (ui16Tag == Subscription::DUMMY_TAG) {
        int rc = addHistoryInternal (ui16ClientId, pszGroupName, pHistory);
        _m.unlock (308);
        return rc;
    }

    Subscription *pSubscription = getSubscriptionForClient (ui16ClientId, pszGroupName);
    if (pSubscription == NULL) {
        _m.unlock (308);
        return -1;
    }
    /* Do not do the following test because it prevents a history request with a specific tag number
       even though the group subscription is for all tags
    if (pSubscription->getSubscriptionType() != Subscription::GROUP_TAG_SUBSCRIPTION) {
        _m.unlock (308);
        return -2;
    } */
    int rc = pSubscription->addHistory (pHistory, ui16Tag);
    _m.unlock (308);
    return rc;
}

int LocalNodeInfo::addHistory (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate, History *pHistory)
{
    _m.lock (309);
    Subscription *pSubscription = getSubscriptionForClient (ui16ClientId, pszGroupName);
    if (pSubscription == NULL) {
        _m.unlock (309);
        return -1;
    }
    if (pSubscription->getSubscriptionType() != Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
        _m.unlock (309);
        return -2;
    }
    int rc = pSubscription->addHistory (pHistory);
    _m.unlock (309);
    return rc;
}

PtrLList<HistoryRequest> * LocalNodeInfo::getHistoryRequests (uint16 ui16ClientId)
{
    _m.lock (310);
    PtrLList<HistoryRequest> *pRet = new PtrLList<HistoryRequest>();
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL != NULL) {
        pSL->getHistoryRequests (*pRet);
    }
    if (!pRet->getFirst()) {
        delete pRet;
        pRet = NULL;
    }
    _m.unlock (310);
    return pRet;
}

bool LocalNodeInfo::isInAnyHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)
{
    _m.lock (311);
    UInt32Hashtable<SubscriptionList>::Iterator i = _localSubscriptions.getAllElements();
    while (!i.end()) {
        if (isInHistory (i.getKey(), pMsg, ui32LatestMsgRcvdPerSender)) {
            _m.unlock (311);
            return true;
        }
        i.nextElement();
    }
    _m.unlock (311);
    return false;
}

bool LocalNodeInfo::isInHistory (uint16 ui16ClientId, Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)
{
    _m.lock (312);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL != NULL) {
        Subscription *pS = pSL->getSubscription (pMsg->getMessageInfo()->getGroupName());
        if (pS) {
            bool bRet = pS->isInHistory (pMsg, ui32LatestMsgRcvdPerSender);
            _m.unlock (312);
            return bRet;
        }
    }
    _m.unlock (312);
    return false;
}

SubscriptionList * LocalNodeInfo::getSubscriptionListForClient (uint16 ui16ClientId)
{
    _m.lock (313);
    return _localSubscriptions.get (ui16ClientId);
}

void LocalNodeInfo::releaseLocalNodeInfo (void)
{
    _m.unlock (313);
}

DArray<uint16> * LocalNodeInfo::getSubscribingClients (Message *pMsg)
{
    DArray<uint16> *pSubscribingClients = NULL;
    uint16 j = 0;
     _m.lock (314);
    for (UInt32Hashtable<SubscriptionList>::Iterator i = _localSubscriptions.getAllElements(); !i.end(); i.nextElement()) {
        PtrLList<Subscription> *pSubscriptions = i.getValue()->getSubscriptionWild (pMsg->getMessageHeader()->getGroupName());
        // Get every subscribed group that matches with the message's group
        if (pSubscriptions != NULL) {
            for (Subscription *pSub = pSubscriptions->getFirst(); pSub != NULL; pSub = pSubscriptions->getNext()) {
                if ((pSub->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) || pSub->matches(pMsg)) {
                    if (pSubscribingClients == NULL) {
                        pSubscribingClients = new DArray<uint16>();
                    }
                    bool bFound = false;
                    for (unsigned int k = 0; k < pSubscribingClients->size(); k++) {
                        if ((*pSubscribingClients)[k] == i.getKey()) {
                            bFound = true;
                            break;
                        }
                    }
                    if (!bFound) {
                        (*pSubscribingClients)[j] = i.getKey();
                        j++;
                    }
                }
            }
            delete pSubscriptions;
            pSubscriptions = NULL;
        }
    }
    _m.unlock (314);
    return pSubscribingClients;
}

DArray<uint16> * LocalNodeInfo::getAllSubscribingClients()
{
    _m.lock (315);
    DArray<uint16> *pSubscribingClients = NULL;
    uint16 j = 0;
    for (UInt32Hashtable<SubscriptionList>::Iterator i = _localSubscriptions.getAllElements(); !i.end(); i.nextElement()) {
        if (pSubscribingClients == NULL) {
            pSubscribingClients = new DArray<uint16>();
        }
        (*pSubscribingClients)[j] = i.getKey();
        j++;
    }
    _m.unlock (315);
    return pSubscribingClients;
}

void LocalNodeInfo::removeClient (uint16 ui16ClientId)
{
    _m.lock (316);
    SubscriptionList *pSL = _localSubscriptions.remove (ui16ClientId);
    recomputeConsolidateSubsciptionList();
    _m.unlock (316);
    if (pSL != NULL) {
        delete pSL;
    }
}

bool LocalNodeInfo::hasSubscription (Message *pMessage)
{
    _m.lock (317);
    for (UInt32Hashtable<SubscriptionList>::Iterator i = _localSubscriptions.getAllElements(); !i.end(); i.nextElement()) {
        SubscriptionList *pSL = i.getValue();
        if (pSL->hasSubscriptionWild (pMessage)) {
            _m.unlock (317);
            return true;
        }
    }
    _m.unlock (317);
    return false;
}

bool LocalNodeInfo::hasSubscription (uint16 ui16ClientId, Message *pMessage)
{
    _m.lock (318);
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL == NULL) {
        _m.unlock (318);
        return false;
    }
    bool bRet = pSL->hasSubscriptionWild (pMessage);
    _m.unlock (318);
    return bRet;
}

bool LocalNodeInfo::requireGroupReliability (const char *pszGroupName, uint16 ui16Tag)
{
    _m.lock (319);
    PtrLList<Subscription> *pSubscriptions = _consolidatedSubscriptions.getSubscriptionWild (pszGroupName);
    if (pSubscriptions != NULL) {
        for (Subscription *pSub = pSubscriptions->getFirst(); pSub != NULL; pSub = pSubscriptions->getNext()) {
            if (pSub->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                GroupSubscription *pGS = (GroupSubscription *) pSub;
                if (pGS->isGroupReliable()) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (319);
                    return true;
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
                GroupTagSubscription *pGTS = (GroupTagSubscription *) pSub;
                if (pGTS->isGroupReliable (ui16Tag)) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (319);
                    return true;
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
                GroupPredicateSubscription *pGPS = (GroupPredicateSubscription *) pSub;
                if (pGPS->isGroupReliable()) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (319);
                    return true;
                }
            }
        }

        delete pSubscriptions;
        pSubscriptions = NULL;
    }

    _m.unlock (319);
    return false;
}

bool LocalNodeInfo::requireMessageReliability (const char *pszGroupName, uint16 ui16Tag)
{
    _m.lock (320);
    PtrLList<Subscription> *pSubscriptions = _consolidatedSubscriptions.getSubscriptionWild (pszGroupName);
    if (pSubscriptions != NULL) {
        for (Subscription *pSub = pSubscriptions->getFirst(); pSub != NULL; pSub = pSubscriptions->getNext()) {
            if (pSub->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                GroupSubscription *pGS = (GroupSubscription *) pSub;
                if (pGS->isMsgReliable()) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (320);
                    return true;
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
                GroupTagSubscription *pGTS = (GroupTagSubscription *) pSub;
                if (pGTS->isMsgReliable (ui16Tag)) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (320);
                    return true;
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
                GroupPredicateSubscription *pGPS = (GroupPredicateSubscription *) pSub;
                if (pGPS->isMsgReliable()) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (320);
                    return true;
                }
            }
        }

        delete pSubscriptions;
        pSubscriptions = NULL;
    }

    _m.unlock (320);
    return false;
}

bool LocalNodeInfo::requireSequentiality (const char *pszGroupName, uint16 ui16Tag)
{
    _m.lock (321);
    PtrLList<Subscription> *pSubscriptions = _consolidatedSubscriptions.getSubscriptionWild(pszGroupName);
    if (pSubscriptions != NULL) {
        for (Subscription *pSub = pSubscriptions->getFirst(); pSub != NULL; pSub = pSubscriptions->getNext()) {
            if (pSub->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                GroupSubscription *pGS = (GroupSubscription *) pSub;
                if (pGS->isSequenced()) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (321);
                    return true;
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
                GroupTagSubscription *pGTS = (GroupTagSubscription *) pSub;
                if (pGTS->isSequenced(ui16Tag)) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (321);
                    return true;
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
                GroupPredicateSubscription *pGPS = (GroupPredicateSubscription *) pSub;
                if (pGPS->isSequenced()) {
                    delete pSubscriptions;
                    pSubscriptions = NULL;
                    _m.unlock (321);
                    return true;
                }
            }
        }

        delete pSubscriptions;
        pSubscriptions = NULL;
    }

    _m.unlock (321);
    return false;
}

uint32 LocalNodeInfo::getGroupPubState (const char *pszGroupName)
{
    _m.lock (322);
    GroupPubState *pGPS = _pubState.get (pszGroupName);
    if (pGPS == NULL) {
        pGPS = new GroupPubState();
        if (pGPS != NULL) {
            _pubState.put (pszGroupName, pGPS);
        }
    }
    uint32 ui32Ret = pGPS->ui32NextSeqNum;
    _m.unlock (322);
    return ui32Ret;
}

int LocalNodeInfo::incrementGroupPubState (const char *pszGroupName)
{
    _m.lock (323);
    GroupPubState *pGPS = _pubState.get (pszGroupName);
    if (pGPS == NULL) {
        _m.unlock (323);
        return -1;
    }
    pGPS->ui32NextSeqNum++;
    _m.unlock (323);
    return 0;
}

int LocalNodeInfo::setGroupPubState (const char *pszGroupName, uint32 ui32NewSeqId)
{
    _m.lock (324);
    GroupPubState *pGPS = _pubState.get (pszGroupName);
    if (pGPS == NULL) {
        pGPS = new GroupPubState();
        _pubState.put (pszGroupName, pGPS);
    }
    pGPS->ui32NextSeqNum = ui32NewSeqId;
    _m.unlock (324);
    return 0;
}

PtrLList<String> * LocalNodeInfo::getAllSubscribedGroups (void)
{
    _m.lock (325);
    if (_consolidatedSubscriptions.isEmpty()) {
        _m.unlock (325);
        return NULL;
    }
    PtrLList<String> *pRet = _consolidatedSubscriptions.getAllSubscribedGroups();
    _m.unlock (325);
    return pRet;
}

PtrLList<String> * LocalNodeInfo::getAllSubscriptions (void)
{
    _m.lock (326);
    if (_consolidatedSubscriptions.isEmpty()) {
        _m.unlock (326);
        return NULL;
    }
    PtrLList<String> *pRet = _consolidatedSubscriptions.getAllSubscribedGroups();
    const char *pszEnd = ".[od]";
    PtrLList<String> temp = (*pRet);
    for (String *pszCurr = temp.getFirst(); pszCurr != NULL; pszCurr = temp.getNext()) {
        if (pszCurr->endsWith (pszEnd) == 1) {
            String *pDel = pRet->remove (pszCurr);
            if (pDel != NULL) {
                delete pDel;
            }
        }
    }
    _m.unlock (326);
    return pRet;
}

PtrLList<Subscription> * LocalNodeInfo::getSubscriptions (const char *pszGroupName)
{
    _m.lock (327);
    if (_consolidatedSubscriptions.isEmpty()) {
        _m.unlock (327);
        return NULL;
    }
    PtrLList<Subscription> *pRet = _consolidatedSubscriptions.getSubscriptionWild (pszGroupName);
    _m.unlock (327);
    return pRet;
}

void LocalNodeInfo::addAddFiltersToConsolidateList (const char *pszGroupName)
{
    _m.lock (328);
    // Get all the client subscribing the group
    DArray<uint16> *pSubClients = NULL;//getSubscribingClients (pszGroupName);
    Subscription *pSCons = _consolidatedSubscriptions.getSubscription (pszGroupName);
    if ((pSubClients == NULL) ||  (!((pSCons != NULL) && (pSCons->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION)))) {
        _m.unlock (328);
        return;
    }
    GroupSubscription *pGSCons = (GroupSubscription *) pSCons;

    // Look for the first subscribing client which subscribes by a GROUP_SUBSCRIPTION
    uint16 ui16ClientId;
    Subscription *pS = NULL;
    for (int i = 0; i <= pSubClients->getHighestIndex(); i++) {
        ui16ClientId = (*pSubClients)[i];
        SubscriptionList *pSL = NULL;
        if (((pSL = _localSubscriptions.get(ui16ClientId)) != NULL) && ((pS = pSL->getSubscription(pszGroupName)) != NULL)) {
            if (pS->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                break;
            }
            if (pS->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
                // I want every tag - remove them and return
                pGSCons->removeAllFilters();
                _m.unlock (328);
                return;
            }
        }
    }

    // match every filter against every other subscribing client's tag list.
    // Add it iff:
    // 1) Every other GROUP_SUBSCRIPTION has the same filter
    // 2) No one of the other GROUP_TAG_SUBSCRIPTION subscribe the tag
    // 3) There is not any GROUP_PREDICATE_SUBSCRIPTION for the group
    GroupSubscription *pGS = (GroupSubscription*) pS;
    DArray<uint16> *pTags = pGS->getAllFilters();

    for (int i = 0; i <= pTags->getHighestIndex(); i++) {
        bool bAddFilter = true;
        int16 ui16Tag = (*pTags)[i];
        for (int j = 0; j <= pSubClients->getHighestIndex(); j++) {
            Subscription *pS = NULL;
            if (pS->matches(ui16Tag)) {
                bAddFilter = false;
                break;
            }
        }
        if (bAddFilter) {
            pGSCons->addFilter((*pTags)[i]);
        }
    }
    _m.unlock (328);
}

uint8 LocalNodeInfo::getConnectivityHistoryNodesCount()
{
    _m.lock (330);
    uint8 ui8Ret = _connectivityHistory.getCount();
    _m.unlock (330);
    return ui8Ret;
}

void LocalNodeInfo::addNodeToConnectivityHistory (const char*pszNodeId)
{
    _m.lock (331);
    _connectivityHistory.add (pszNodeId);
    _m.unlock (331);
}

void LocalNodeInfo::setConnectivityHistoryWinSize (int64 i64WinSize)
{
    _m.lock (332);
    _connectivityHistory.setWindowSize (i64WinSize);
    _m.unlock (332);
}

Subscription * LocalNodeInfo::getSubscriptionForClient (uint16 ui16ClientId, const char *pszGroupName)
{
    SubscriptionList *pSL = _localSubscriptions.get (ui16ClientId);
    if (pSL == NULL) {
        return NULL;
    }
    return pSL->getSubscription (pszGroupName);
}

void LocalNodeInfo::updateConsolidateSubsciptionList (const char *pszGroupName, Subscription *pSubscription)
{
    // TODO CHECK: I don't think the includes applies anymore, so I commented it
    // In fact, even if the includes returns true,
    // I still need to merge subscriptions in terms of priority, reliability, sequenced, etc

    bool bConsolidateListChanged = false;
    Subscription *pS = _consolidatedSubscriptions.getSubscription (pszGroupName);
    if (pS == NULL) {
        Subscription *pSubCopy = pSubscription->clone();
        if (pSubCopy == NULL) {
            checkAndLogMsg ("LocalNodeInfo::updateConsolidateSubsciptionList", memoryExhausted);
            return;
        }
        _consolidatedSubscriptions.addGroup (pszGroupName, pSubCopy);
        bConsolidateListChanged = true;
    }
    /*else if (pSubscription->includes (pS)) {
        _consolidatedSubscriptions.removeGroup (pszGroupName);
        Subscription *pSubCopy = pSubscription->clone();
        if (pSubCopy == NULL) {
            checkAndLogMsg ("LocalNodeInfo::updateConsolidateSubsciptionList", memoryExhausted);
            return;
        }
        _consolidatedSubscriptions.addGroup (pszGroupName, pSubCopy);
        bConsolidateListChanged = true;

    }*/
    else {
        // Adds the content of pSubscription into pS
        bConsolidateListChanged = pSubscription->merge (pS); //TODO: implement this method for every type of subscription
    }

    if (bConsolidateListChanged) {
        ui32SubscriptionStateSeqID++;
        _notifier.newSubscriptionForPeer (_pszId, pSubscription);
    }
}

void LocalNodeInfo::recomputeConsolidateSubsciptionList (void)
{
    // TODO CHECK: I don't think the includes applies anymore, so I commented it
    // In fact, even if the includes returns true,
    // I still need to merge subscriptions in terms of priority, reliability, sequenced, etc

    CRC * pCRC = new CRC();
    pCRC->init();
    for (StringHashtable<Subscription>::Iterator iterator = _consolidatedSubscriptions.getIterator(); !iterator.end(); iterator.nextElement()) {
        pCRC->update ((const char *)iterator.getKey());
        pCRC->update32 (iterator.getValue());
    }
    uint16 oldCRC = pCRC->getChecksum();

    // Delete the current Consolidate Subscription List and compute a new one
    _consolidatedSubscriptions.clear();

    // For each client
    for (UInt32Hashtable<SubscriptionList>::Iterator i = _localSubscriptions.getAllElements(); !i.end(); i.nextElement()) {
        SubscriptionList *pSL = i.getValue();
        if (pSL != NULL) {
            // Get all its subscriptions
            PtrLList<String> *pSubgroups = pSL->getAllSubscribedGroups();
            for (String *pSubGroupName = pSubgroups->getFirst(); pSubGroupName != NULL; pSubGroupName = pSubgroups->getNext()) {
                // For each group, get the subscription the client has
                const char *pszGroupName = pSubGroupName->c_str();
                Subscription *pClientSub = pSL->getSubscription (pszGroupName);
                // And the subscription in the consolidate subscription list if any
                Subscription *pSubInConsolidateList = _consolidatedSubscriptions.getSubscription (pszGroupName);
                if (pSubInConsolidateList == NULL) {
                    _consolidatedSubscriptions.addSubscription (pszGroupName, pClientSub->clone());
                }
                else {
                    /*if (pClientSub->includes (pSubInConsolidateList)) {
                        _consolidatedSubscriptions.removeGroup (pszGroupName);
                        _consolidatedSubscriptions.addSubscription (pszGroupName, pClientSub->clone());
                    }
                    else {*/
                        pClientSub->merge (pSubInConsolidateList);
                    /*}*/
                }
            }
        }
    }

    pCRC->reset();
    for (StringHashtable<Subscription>::Iterator iterator = _consolidatedSubscriptions.getIterator(); !iterator.end(); iterator.nextElement()) {
        pCRC->update ((const char *) iterator.getKey());
        pCRC->update32 (iterator.getValue());
    }
    uint16 newCRC = pCRC->getChecksum();
    if (oldCRC != newCRC) {
        ui32SubscriptionStateSeqID++;
        GroupSubscription *pSubscription = new GroupSubscription(); // Void subscription to respect method interface
        _notifier.modifiedSubscriptionForPeer (_pszId, pSubscription);
    }
}

SubscriptionList * LocalNodeInfo::getConsolidatedSubscriptions (void)
{
    return &_consolidatedSubscriptions;
}

SubscriptionList * LocalNodeInfo::getConsolidatedSubscriptionsCopy (void)
{
    SubscriptionList *pConsolidatedSubscriptionsCopy = new SubscriptionList();
    if (pConsolidatedSubscriptionsCopy != NULL) {
        for (StringHashtable<Subscription>::Iterator i = _consolidatedSubscriptions.getIterator(); !i.end(); i.nextElement()) {
            pConsolidatedSubscriptionsCopy->addSubscription (i.getKey(), (i.getValue())->clone()); // Clone subscription before adding it
        }
    }
    return pConsolidatedSubscriptionsCopy;
}

void LocalNodeInfo::printAllSubscribedGroups (void)
{
    PtrLList<String> *pGroupNames = getAllSubscribedGroups();
    if (pGroupNames && pGroupNames->getFirst()) {
        for (String *pGroupName = pGroupNames->getFirst(); pGroupName; pGroupName = pGroupNames->getNext()) {
            checkAndLogMsg ("LocalNodeInfo::printAllSubscribedGroups", Logger::L_Info, "        Subscribed group: %s\n", pGroupName->c_str());
        }
    }
}
