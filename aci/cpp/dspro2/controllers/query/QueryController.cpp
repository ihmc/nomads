/*
 * QueryController.cpp
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

#include "QueryController.h"

#include "DataStore.h"
#include "DSProImpl.h"
#include "MetaData.h"
#include "QueryQualifierBuilder.h"
#include "SearchProperties.h"
#include "Searches.h"
#include "Topology.h"

#include "InformationStore.h"
#include "Logger.h"
#include "Mutex.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

#define returnedErr(methodName,rc) Logger::L_Warning, "%s returned %d\n", methodName, rc

namespace IHMC_ACI
{
    static Mutex _mSentQueries;
    static StringHashset _sentQueries;
}

QueryController::QueryController (const char *pszDescription, const char *pszSupportedQueryType, DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore)
    : SearchListener (pszDescription),
      StorageController (pDSPro, pDataStore, pInfoStore),
      ApplicationNotificationSvc (pDSPro),
      MessagingSvc (pDSPro), TopologySvc (pDSPro),
      _nodeId (pDSPro->getNodeId())
{
    if (pszSupportedQueryType != nullptr) {
        _supportedQueryTypes[0] = pszSupportedQueryType; // String makes a copy of ppszSupportedQueryTypes[i]
    }
}

QueryController::QueryController (const char *pszDescription, const char **ppszSupportedQueryTypes, DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore)
    : SearchListener (pszDescription),
      StorageController (pDSPro, pDataStore, pInfoStore),
      ApplicationNotificationSvc (pDSPro),
      MessagingSvc (pDSPro), TopologySvc (pDSPro),
      _nodeId (pDSPro->getNodeId())
{
    if (ppszSupportedQueryTypes != nullptr) {
        for (unsigned int i = 0; ppszSupportedQueryTypes[i] != nullptr; i++) {
            _supportedQueryTypes[i] = ppszSupportedQueryTypes[i]; // String makes a copy of ppszSupportedQueryTypes[i]
        }
    }
}

QueryController::~QueryController()
{
    getDSPro()->getCallbackHandler()->deregisterSearchListener (_ui16SearchClientId, this);
}

int QueryController::init (MetadataConfigurationImpl *pMetadataConf, CommAdaptorManager *pCommAdaptorMgr)
{
    _pMetadataConf = pMetadataConf;
    _pCommAdaptMgr = pCommAdaptorMgr;
    _ui16CommAdaptorClientId = 0;
    int rc;
    _ui16SearchClientId = 0;
    if ((rc = getDSPro ()->getCallbackHandler()->registerSearchListener (0, this, _ui16SearchClientId)) != 0) {
        checkAndLogMsg ("QueryController::init", Logger::L_SevereError,
                        "Failed in registering the search listener with rc=%d\n", rc);
        return -2;
    }

    return 0;
}

// SearchListener
void QueryController::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                     const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                     const void *pszQuery, unsigned int uiQueryLen)
{
    // The application issued a search query
    if (pszQueryId == nullptr || pszGroupName == nullptr || pszQuerier == nullptr ||
        pszQueryType == nullptr || pszQuery == nullptr || uiQueryLen == 0) {
        return;
    }

    uint16 ui16ClientId = 0xFFFF;
    Searches::getSearches()->addSearchInfo (pszQueryId, pszQueryType, getDSPro()->getNodeId(), ui16ClientId);

    _mSentQueries.lock();
    if ((_nodeId != pszQuerier) && !_sentQueries.containsKey (pszQueryId)) {
        /*SearchProperties searchProp;
        searchProp.pszQueryId = pszQueryId;
        searchProp.pszGroupName = pszGroupName;
        searchProp.pszQuerier = pszQuerier;
        searchProp.pszQueryType = pszQueryType;
        searchProp.pszQueryQualifiers = pszQueryQualifiers;
        searchProp.pQuery = pszQuery;
        searchProp.uiQueryLen = uiQueryLen;
        sendSearch (&searchProp);*/
    }
    _mSentQueries.unlock();
}

void QueryController::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    const char *pszMethodName = "QueryController::searchReplyArrived";
    if (pszQueryId == nullptr || ppszMatchingMessageIds == nullptr || pszMatchingNodeId == nullptr) {
        return;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if ((Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) < 0) || (queryType.length() <= 0)) {
        return;
    }

    int rc = 0;
    if (supportsQueryType (queryType)) {

        if (!isNewQueryReply (pszQueryId, pszMatchingNodeId)) {
            // replies from the same node should not be notified multiple times,
            // on the other hand, if a search is received multiple times, it should
            // be replied multiple times (because the response may be lost)
            return;
        }

        // it was me that issued the query, I can therefore retrieve the matching messages
        const String dsproId (getDSPro()->getNodeId());
        if (((dsproId == querier) == 1) && ((dsproId != pszMatchingNodeId) == 1)) {
            // a remote application, connected to a different instance of DSPro,
            for (unsigned int i = 0; ppszMatchingMessageIds[i] != nullptr; i++) {
                // Request the returned message
                const String matchingMsgId (ppszMatchingMessageIds[i]);
                if (getDataStore()->hasData (matchingMsgId)) {
                    addRequestedMessageToUserRequests (matchingMsgId, pszQueryId);
                }
                else if ((rc = sendAsynchronousRequestMessage (matchingMsgId)) == 0) {
                    // TODO: consider whether making the addRequestedMessageToUserRequests() conditional
                    // using the search group for the custum chunks
                    if (queryType == "dsprochunkquery") {
                        addRequestedMessageToUserRequests (matchingMsgId, pszQueryId);
                    }
                    checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                    "requested message %s\n", matchingMsgId.c_str());
                }
                else {
                    checkAndLogMsg (pszMethodName, returnedErr("sendAsynchronousRequestMessage()", rc));
                }
            }
        }
        // Regardless on whether it was a remote, or a local application that replied,
        // I need to notify the local client applications that a query was matched
        if ((rc = notifySearchReply (pszQueryId, querier, ppszMatchingMessageIds, dsproId)) == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "notifySearchReply() notified %s\n", pszQueryId);
        }
        else {
            checkAndLogMsg (pszMethodName, returnedErr ("notifySearchReply()", rc));
        }
    }
}

void QueryController::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    const char *pszMethodName = "QueryController::volatileSearchReplyArrived";
    if (pszQueryId == nullptr || pReply == nullptr || ui162ReplyLen == 0 || pszMatchingNodeId == nullptr) {
        return;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if ((Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) < 0) || (queryType.length () <= 0)) {
        return;
    }

    int rc = 0;
    if (supportsQueryType (queryType)) {

        if (!isNewQueryReply (pszQueryId, pszMatchingNodeId)) {
            // replies from the same node should not be notified multiple times,
            // on the other hand, if a search is received multiple times, it should
            // be replied multiple times (because the response may be lost)
            return;
        }

        // Regardless on whether it was a remote, or a local application that replied,
        // I need to notify the local client applications that a query was matched
        RawDataReply reply (pszQueryId, querier, getDSPro()->getNodeId(), pReply, ui162ReplyLen);
        if ((rc = notifySearchReply (reply)) == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "notifySearchReply() notified %s\n", pszQueryId);
        }
        else {
            checkAndLogMsg (pszMethodName, returnedErr ("volatileSearchReplyArrived()", rc));
        }
    }
}

MetadataList * QueryController::getAllMetadata (const char *pszQueryQualifier)
{
    InformationStore *pInfoStore = getInformationStore();
    if (pInfoStore == nullptr) {
        checkAndLogMsg ("QueryController::getAllMetadata", Logger::L_Warning, "Could not get information store\n");
        return nullptr;
    }

    QueryQualifierBuilder *pQualifier = pszQueryQualifier != nullptr ?
        QueryQualifierBuilder::parse (pszQueryQualifier) : nullptr;

    char **ppszMessageIdIn = nullptr;
    if (pQualifier != nullptr && pQualifier->getGroupBy() != nullptr &&
        pQualifier->getOrder() != nullptr && pQualifier->getLimit() != nullptr) {
        String table (getMetadataTableName());

        String innerSql = "SELECT COUNT (*) FROM ";
        innerSql       += table;
        innerSql       += " AS m WHERE m.";
        innerSql       += pQualifier->getGroupBy();
        innerSql       += " = t.";
        innerSql       += pQualifier->getGroupBy();
        innerSql       += " AND m. ";
        innerSql       += pQualifier->getOrder();
        innerSql       += " >= t.";
        innerSql       += pQualifier->getOrder();

        String sql = "SELECT ";
        sql += MetaData::MESSAGE_ID;
        sql += " FROM ";
        sql += table;
        sql += " AS t WHERE (";
        sql +=     innerSql;
        sql += ")  <= ";
        sql += pQualifier->getLimit();

        ppszMessageIdIn = pInfoStore->getDSProIds (sql.c_str());
    }

    MetadataList *pMetadataList = pInfoStore->getAllMetadata ((const char **)ppszMessageIdIn, false);

    if (ppszMessageIdIn != nullptr) {
        deallocateNullTerminatedPtrArray (ppszMessageIdIn);
    }

    return pMetadataList;
}

int QueryController::notifySearchReply (const Reply &reply)
{
    if (reply._pszQueryId == nullptr || reply._pszQuerier == nullptr) {
        return -1;
    }

    int rc = 0;
    if (strcmp (reply._pszQuerier, getDSPro()->getNodeId()) == 0) {
        // A client of the local DSPro issued the query
        switch (reply._type) {
            case Reply::IDS: {
                const MatchingIdReply *pReply = static_cast<const MatchingIdReply *>(&reply);
                if (pReply->_ppszMatchingMsgIds == nullptr) {
                    return -2;
                }
                asynchronouslyNotifyMatchingMetadata (reply._pszQueryId, pReply->_ppszMatchingMsgIds);
            }
            case Reply::RAW: {
                const RawDataReply *pReply = static_cast<const RawDataReply *>(&reply);
                if (pReply->_pReply == nullptr) {
                    return -3;
                }
                asynchronouslyNotifyMatchingSearch (reply._pszQueryId, pReply->_pReply, pReply->_ui162ReplyLen);
            }
        }
    }
    else {
        // A remote peer issued the query
        rc = sendSearchReply (reply);
    }

    return rc;
}

int QueryController::notifySearchReply (const char *pszQueryId, const char *pszQuerier,
                                        const char **ppszMatchingMsgIds, const char *pszMatchingNodeId)
{
    MatchingIdReply reply (pszQueryId, pszQuerier, pszMatchingNodeId, ppszMatchingMsgIds);
    return notifySearchReply (reply);
}

bool QueryController::supportsQueryType (const char *pszQueryType)
{
    if (pszQueryType == nullptr) {
        return false;
    }

    for (unsigned int i = 0; i < _supportedQueryTypes.size(); i++) {
        // do case-insensitive comparison
        if (_supportedQueryTypes.used (i) && (_supportedQueryTypes[i] ^= pszQueryType)) {
            return true;
        }
    }

    return false;
}

bool QueryController::isNewQueryReply (const char *pszQueryId, const char *pszMatchingNodeId)
{
    if (pszQueryId == nullptr || pszMatchingNodeId == nullptr) {
        return false;
    }

    bool bIsNew = false;
    StringHashset *pQueriers = _rcvdQueryReplies.get (pszQueryId);
    if (pQueriers == nullptr) {
        bIsNew = true;
        pQueriers = new StringHashset();
        if (pQueriers != nullptr) {
            _rcvdQueryReplies.put (pszQueryId, pQueriers);
        }
    }
    if (pQueriers != nullptr) {
        bIsNew = pQueriers->put (pszMatchingNodeId);
    }

    return bIsNew;
}

int QueryController::sendSearch (SearchProperties *pSearchProperties)
{
    Topology *pTopology = getTopology();
    if (pTopology == nullptr || pSearchProperties == nullptr) {
        return -1;
    }
    Targets **ppTargets = pTopology->getNeighborsAsTargets();
    int rc = sendSearchMessage (*pSearchProperties, ppTargets);
    if (rc == 0) {
        _sentQueries.put (pSearchProperties->pszQueryId);
    }
    Targets::deallocateTargets (ppTargets);
    return (rc == 0 ? 0 : -2);
}

int QueryController::sendSearchReply (const Reply &reply)
{
    Topology *pTopology = getTopology();
    if (pTopology == nullptr || reply._pszQueryId == nullptr || reply._pszQuerier == nullptr) {
        return -1;
    }

    TargetPtr targets[2] = { pTopology->getNextHopAsTarget (reply._pszQuerier), nullptr };

    int rc;
    switch (reply._type) {
        case Reply::IDS: {
            const MatchingIdReply *pReply = static_cast<const MatchingIdReply *>(&reply);
            if (pReply->_ppszMatchingMsgIds == nullptr) {
                return -2;
            }
            rc = sendSearchReplyMessage (reply._pszQueryId, pReply->_ppszMatchingMsgIds,
                                         reply._pszQuerier, reply._pszMatchingNodeId, targets);
            break;
        }

        case Reply::RAW: {
            const RawDataReply *pReply = static_cast<const RawDataReply *>(&reply);
            if (pReply->_pReply == nullptr) {
                return -3;
            }
            rc = sendSearchReplyMessage (reply._pszQueryId, pReply->_pReply, pReply->_ui162ReplyLen,
                                         reply._pszQuerier, reply._pszMatchingNodeId, targets);
            break;
        }

        default:
            assert (false);
            rc = -2;
    }

    delete targets[0];
    return (rc == 0 ? 0 : -3);
}

//-----------------------------------------------------------------------------

QueryController::Reply::Reply (Type type, const char *pszQueryId, const char *pszQuerier, const char *pszMatchingNodeId)
    : _type (type), _pszQueryId (pszQueryId), _pszQuerier (pszQuerier), _pszMatchingNodeId (pszMatchingNodeId)
{
}

QueryController::Reply::~Reply (void)
{
}

QueryController::MatchingIdReply::MatchingIdReply (const char *pszQueryId, const char *pszQuerier, const char *pszMatchingNodeId, const char **ppszMatchingMsgIds)
    : QueryController::Reply (Reply::IDS, pszQueryId, pszQuerier, pszMatchingNodeId),
      _ppszMatchingMsgIds (ppszMatchingMsgIds)
{
}

QueryController::MatchingIdReply::~MatchingIdReply (void)
{
}

QueryController::RawDataReply::RawDataReply (const char *pszQueryId, const char *pszQuerier, const char *pszMatchingNodeId, const void *pReply, uint16 ui162ReplyLen)
    : QueryController::Reply (Reply::RAW, pszQueryId, pszQuerier, pszMatchingNodeId),
      _ui162ReplyLen (ui162ReplyLen), _pReply (pReply)
{
}

QueryController::RawDataReply::~RawDataReply (void)
{
}

