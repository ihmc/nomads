/*
 * ApplicationQueryController.cpp
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

#include "ApplicationQueryController.h"

#include "DataStore.h"
#include "DSPro.h"
#include "InformationStore.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * const ApplicationQueryController::SUPPORTED_QUERY_TYPE = "dspro-application-query";

ApplicationQueryController::ApplicationQueryController (DSProImpl *pDSPro, DataStore *pDataStore,
                                                        InformationStore *pInfoStore) 
    : QueryController ("ApplicationQueryController", SUPPORTED_QUERY_TYPE, pDSPro, pDataStore, pInfoStore)
{
}

ApplicationQueryController::~ApplicationQueryController (void)
{
}

void ApplicationQueryController::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                                const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                                const void *pQuery, unsigned int uiQueryLen)
{
    const char *pszMethodName = "ApplicationQueryController::searchArrived";
    if (pQuery == NULL || uiQueryLen == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Received null query\n");
    }
    QueryController::searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType,
                                    pszQueryQualifiers, pQuery, uiQueryLen);
    String query ((const char*)pQuery, uiQueryLen);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Query %s\n", query.c_str());
}

void ApplicationQueryController::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    QueryController::searchReplyArrived (pszQueryId, ppszMatchingMessageIds, pszMatchingNodeId);
}

