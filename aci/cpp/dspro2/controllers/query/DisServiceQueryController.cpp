/*
 * DisServiceQueryController.cpp
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

#include "DisServiceQueryController.h"

#include "DataStore.h"
#include "DSProImpl.h"
#include "InformationStore.h"

#include "Logger.h"
#include "NLFLib.h"

#include "xpath_static.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace TinyXPath;

const char * const DisServiceQueryController::SUPPORTED_QUERY_TYPE = "disservice-query";

DisServiceQueryController::DisServiceQueryController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore)
    : QueryController ("DisServiceQueryController", SUPPORTED_QUERY_TYPE, pDSPro, pDataStore, pInfoStore)
{
}

DisServiceQueryController::~DisServiceQueryController()
{
}

void DisServiceQueryController::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                               const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                               const void *pQuery, unsigned int uiQueryLen)
{
    QueryController::searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType, pszQueryQualifiers, pQuery, uiQueryLen);

    // A search arrived from the application
    const char *pszMethodName = "DisServiceQueryController::searchArrived";
   
    if (pQuery == NULL || pszQueryType == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "some of the needed parameters are null\n");
        return;
    }

    if (!supportsQueryType (pszQueryType)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "query type %s not supported\n", pszQueryType);
        return;
    }

    String query ((char *)pQuery, uiQueryLen);

    DArray2<NOMADSUtil::String> *pDArray2MessageIds = _pDataCacheInterface->getMessageIDs (query.c_str());
    if (pDArray2MessageIds == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "problems in retrieving the message ids from the data cache\n");
        return;
    }

    unsigned int uiNResults = (int) pDArray2MessageIds->size();
    if (uiNResults == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "no results found for the query %s\n", pQuery);
        return;
    }

    char **ppszMatchingMsgIds = (char **) calloc (uiNResults+1, sizeof (char*));
    if (ppszMatchingMsgIds == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "problems in creating the array with the results\n");
        return;
    }

    for (unsigned int i = 0; i < uiNResults; i++) {
        ppszMatchingMsgIds[i] = (*pDArray2MessageIds)[i].r_str();
    }
    ppszMatchingMsgIds[uiNResults] = NULL;

    int rc = notifySearchReply (pszQueryId, pszQuerier, (const char **)ppszMatchingMsgIds, getDSPro()->getNodeId());
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "notifySearchReply() returned and error: %d\n", rc);
    }
    deallocateNullTerminatedPtrArray (ppszMatchingMsgIds);

    checkAndLogMsg ("DSProQueryController::searchArrived", Logger::L_Info,
                    "Query %s\n", query.c_str());
}

void DisServiceQueryController::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    QueryController::searchReplyArrived (pszQueryId, ppszMatchingMessageIds, pszMatchingNodeId);
}

