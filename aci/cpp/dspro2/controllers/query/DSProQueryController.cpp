/*
 * DSProQueryController.cpp
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

#include "DSProQueryController.h"

#include "CommAdaptorManager.h"
#include "DataStore.h"
#include "DSProImpl.h"
#include "InformationStore.h"
#include "MetadataConfigurationImpl.h"
#include "StringTokenizer.h"

#include "Logger.h"
#include "NLFLib.h"

#include "xpath_static.h"
#include "tinyxml.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;
using namespace TinyXPath;

const char * const PropertiesConditionsList::SUPPORTED_OPERATORS[] = {"=", "!=", ">=", "<=", ">", "<"};
const char * const DSProQueryController::SUPPORTED_QUERY_TYPES[] = {"sql-on-metadata-dspro-query",
                                                                    "prop-list-on-application-metadata-dspro-query", nullptr};

DSProQueryController::DSProQueryController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore)
    : QueryController ("DSProQueryController", (const char **)SUPPORTED_QUERY_TYPES, pDSPro, pDataStore, pInfoStore)
{
}

DSProQueryController::~DSProQueryController()
{
}

void DSProQueryController::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                          const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                          const void *pQuery, unsigned int uiQueryLen)
{
    QueryController::searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType, pszQueryQualifiers, pQuery, uiQueryLen);

    // A search arrived from the application
    const char *pszMethodName = "DSProQueryController::searchArrived";

    if (pszQueryId == nullptr || pszGroupName == nullptr || pszQueryType == nullptr || pQuery == nullptr || uiQueryLen == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "some of the needed parameters is null\n");
        return;
    }

    if (!supportsQueryType (pszQueryType)) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "query type %s not supported\n", pszQueryType);
        return;
    }

    char **ppszMatchingMsgIds = getMatchingIds (pszQueryId, pszGroupName, pszQueryType, pszQueryQualifiers, pQuery, uiQueryLen);

    if (ppszMatchingMsgIds != nullptr) {
        int rc = notifySearchReply (pszQueryId, pszQuerier, const_cast<const char **>(ppszMatchingMsgIds),
                                    getDSPro()->getNodeId());
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "notifySearchReply() returned and error: %d\n", rc);
        }
        deallocateNullTerminatedPtrArray (ppszMatchingMsgIds);
    }

    String query (static_cast<const char*>(pQuery), uiQueryLen);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Query %s\n", query.c_str());
}

void DSProQueryController::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    QueryController::searchReplyArrived (pszQueryId, ppszMatchingMessageIds, pszMatchingNodeId);
}

void DSProQueryController::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    QueryController::volatileSearchReplyArrived (pszQueryId, pReply, ui162ReplyLen, pszMatchingNodeId);
}

char ** DSProQueryController::getMatchingIds (const char *pszQueryId, const char *pszGroupName,
                                              const char *pszQueryType, const char *pszQueryQualifiers,
                                              const void *pQuery, unsigned int uiQueryLen)
{
    const char *pszMethodName = "DSProQueryController::getMatchingIds";

    checkAndLogMsg (pszMethodName, Logger::L_Info, "SearchProperties: queryId=%s\n", pszQueryId);

    if (pszQueryId == nullptr || pszGroupName == nullptr ||
        pszQueryType == nullptr || pQuery == nullptr || uiQueryLen == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "some of the needed parameters is null\n");
        return nullptr;
    }

    if (!supportsQueryType (pszQueryType)) {
        return nullptr;
    }

    InformationStore *pInfoStore = getInformationStore();
    if (pInfoStore == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Could not get information store\n");
        return nullptr;
    }

    PtrLList<const char> resultList;
    int rc;
    if (strcmp (pszQueryType, SUPPORTED_QUERY_TYPES[0]) == 0) {
        rc = doSQLQueryOnMetadata (pszGroupName, pQuery, uiQueryLen, pszQueryQualifiers, pInfoStore, &resultList);
    }
    else {
        rc = doQueryWithPropertyListOnApplicationMetadata (pQuery, uiQueryLen, pszQueryQualifiers, &resultList);
    }

    const int iNResults = resultList.getCount();
    if (iNResults <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Failed in retrieving the message ids with rc=%d\n", rc);
        return nullptr;
    }

    char **ppszMatchingMsgIds = (char **) calloc (iNResults+1, sizeof (char *));
    if (ppszMatchingMsgIds == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Failed in retrieving the message ids with rc=%d\n", rc);
        return nullptr;
    }

    const char *pszId;
    const char *pszIdNext = resultList.getFirst();
    for (unsigned int i = 0; (pszId = pszIdNext) != nullptr; i++) {
        pszIdNext = resultList.getNext();
        ppszMatchingMsgIds[i] = (char *) pszId;
    }

    ppszMatchingMsgIds[iNResults] = nullptr;
    resultList.removeAll();

    return ppszMatchingMsgIds;
}

int DSProQueryController::matchPropertyListToApplicationMetadata (char *pszBuffer, PropertiesConditionsList *pPropertiesConditionsList,
                                                                  MetadataInterface *pCurr, PtrLList<const char> *pResultsList)
{
    const char *pszMethodName = "DSProQueryController::matchPropertyListToApplicationMetadata";
    char *pszTemp;
    char *pszApplicationMetadataFields = nullptr;
    char *pszToken = strtok_mt (pszBuffer, "{", &pszTemp);
    if (pszToken != nullptr) {
        pszApplicationMetadataFields = strtok_mt (pszToken, "}", &pszTemp);
    }
    if (pszApplicationMetadataFields == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "The Application Metadata field is nullptr\n");
    }

    StringTokenizer tokenizer (pszApplicationMetadataFields, ' ', ',');
    StringTokenizer innerTokenizer;
    for (const char *pszAMField = tokenizer.getNextToken(); pszAMField != nullptr; pszAMField = tokenizer.getNextToken()) {
        innerTokenizer.init (pszAMField, '=', '=');
        const char *pszKey = innerTokenizer.getNextToken();
        const char *pszValue = innerTokenizer.getNextToken();
        if (pszKey == nullptr) {
            continue;
        }

        checkAndLogMsg (pszMethodName, Logger::L_Info, "Application Metadata field: <%s, %s>\n", pszKey, pszValue);

        PropertyCondition *pPropertyCondition = pPropertiesConditionsList->getPropertyCondition (pszKey);
        if (pPropertyCondition == nullptr) {
            continue;
        }

        checkAndLogMsg (pszMethodName, Logger::L_Info, "Found property key %s\n", pszKey);

        bool bAddMessageId = false;

        const char *pPropertyConditionOperator = pPropertyCondition->getPropertyConditionOperator();
        const char *pPropertyConditionValue = pPropertyCondition->getPropertyConditionValue();
        if (strcmp (pPropertyConditionOperator, "!=") == 0) {
            if (strcmp (pszValue, pPropertyConditionValue) != 0) {
                bAddMessageId = true;
            }
        }
        else if (strcmp (pPropertyConditionOperator, "=") == 0) {
            if (strcmp (pszValue, pPropertyConditionValue) == 0) {
                bAddMessageId = true;
            }
        }
        else {
            double dValue;
            if (!atod (pszValue, dValue)) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "Expected a numeric value for property %s. Found %s\n",
                                pszKey, pszValue);
                continue;
            }
            if (!atod (pPropertyConditionValue, dValue)) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "Expected a numeric value for property condition %s. Found %s\n",
                                pszKey, pPropertyConditionValue);
                continue;
            }

            float fValue = (float) dValue;
            float fConditionValue = (float) atof (pPropertyConditionValue);

            if (strcmp (pPropertyConditionOperator, ">=") == 0) {
                if (fValue >= fConditionValue) {
                    bAddMessageId = true;
                }
            }
            else if (strcmp (pPropertyConditionOperator, ">") == 0) {
                if (fValue > fConditionValue) {
                    bAddMessageId = true;
                }
            }
            else if (strcmp (pPropertyConditionOperator, "<=") == 0) {
                if (fValue <= fConditionValue) {
                    bAddMessageId = true;
                }
            }
            else {
                if (fValue < fConditionValue) {
                    bAddMessageId = true;
                }
            }
        }

        if (bAddMessageId) {
            char *pszMetadataId = nullptr;
            if (0 == pCurr->getFieldValue (MetadataInterface::MESSAGE_ID, &pszMetadataId) && pszMetadataId != nullptr) {
                pResultsList->append (pszMetadataId);
            }
        }

    }

    return 0;
}

int DSProQueryController::doSQLQueryOnMetadata (const char *pszGroupName, const void *pQuery, unsigned int uiQueryLen,
                                                const char *, InformationStore *pInfoStore,
                                                PtrLList<const char> *pResultsList)
{
    const char *pszMethodName = "DSProQueryController::doSQLQueryOnMetadata";

    String query ((char *) pQuery, uiQueryLen);

    /*const char *pszSqlConstraints = nullptr;

    char *pszTemp = nullptr;
    char *pszToken = nullptr;
    pszToken = strtok_mt (query.c_str(), "WHERE ", &pszTemp);
    if (pszToken == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error in parsing the char * containing the sql query\n");
        return -1;
    }
    pszSqlConstraints = strtok_mt (nullptr, "WHERE ", &pszTemp);
    if (pszSqlConstraints == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "No where conditions\n");
        return -2;
    }*/

    PtrLList<const char> *ptmp = pInfoStore->getMessageIDs (pszGroupName, query);
    if (ptmp == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error in retrieving the messageIds\n");
        return -3;
    }
    if (pResultsList != nullptr) {
        const char *pszId = ptmp->getFirst();
        for (; pszId != nullptr; pszId = ptmp->getNext()) {
            pResultsList->prepend (pszId);
        }
        ptmp->removeAll (false);
        delete ptmp;
    }

    return 0;
}

int DSProQueryController::doQueryWithPropertyListOnApplicationMetadata (const void *pQuery, unsigned int uiQueryLen, const char *pszQueryQualifiers,
                                                                        PtrLList<const char> *pResultsList)
{
    const char *pszMethodName = "DSProQueryController::doQueryWithPropertyListOnApplicationMetadata";

    MetadataList *pMetadataList = getAllMetadata (pszQueryQualifiers);
    if (pMetadataList == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "\n");
        return -1;
    }

    PropertiesConditionsList *pPropertiesConditionsList = new PropertiesConditionsList();

    String query ((char *) pQuery, uiQueryLen);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "Property conditions list = %s\n", query.c_str());

    char *pszTemp = nullptr;
    char *pszToken = nullptr;
    pszToken = strtok_mt (query.c_str(), ";", &pszTemp);
    while (pszToken) {
        int rc;
        if ((rc = pPropertiesConditionsList->addNewPropertyCondition (pszToken)) != 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Failed in adding the new property condition\n");
            return -2;
        }
        pszToken = strtok_mt (nullptr, ";", &pszTemp);
    }

    MetadataInterface *pCurr, *pNext;
    pNext = pMetadataList->getFirst();
    for (unsigned int i = 0; (pCurr = pNext) != nullptr; i++) {
        pNext = pMetadataList->getNext();
        char *pszBuffer = nullptr;
        if (pCurr->getFieldValue (MetadataInterface::APPLICATION_METADATA, (char **) &pszBuffer) == 0 && pszBuffer != nullptr) {
            matchPropertyListToApplicationMetadata (pszBuffer, pPropertiesConditionsList, pCurr, pResultsList);
            free (pszBuffer);
        }
        delete pMetadataList->remove (pCurr);
    }

    delete pMetadataList;
    return 0;
}

PropertiesConditionsList::PropertiesConditionsList()
    : _propertyConditionsList (true, // bCaseSensitiveKeys
                               true, // bCloneKeys
                               true, // bDeleteKeys
                               true) //bDeleteValues
{
}

PropertiesConditionsList::~PropertiesConditionsList()
{
}

int PropertiesConditionsList::addNewPropertyCondition (const char *pPropertyCondition)
{
    const char *pszMethodName = "PropertiesConditionsList::addNewPropertyCondition";

    char *pszKey;
    char *pszValue;
    char *pszOperator;

    checkAndLogMsg (pszMethodName, Logger::L_Info, "property condition = %s\n", pPropertyCondition);

    int noOperators = sizeof (SUPPORTED_OPERATORS) / sizeof (SUPPORTED_OPERATORS[0]);
    for (int i=0; i<noOperators; i++) {
        char *pszTemp;
        char *pszToken;
        pszToken = strtok_mt (pPropertyCondition, SUPPORTED_OPERATORS[i], &pszTemp);
        if (pszToken != nullptr) {
            pszKey = pszToken;
            pszOperator = (char *) SUPPORTED_OPERATORS[i];
            pszValue = strtok_mt (nullptr, SUPPORTED_OPERATORS[i], &pszTemp);
            if (pszValue != nullptr) {
                PropertyCondition *pPropertyCondition = new PropertyCondition (pszValue, pszOperator);
                if (pPropertyCondition != nullptr) {
                    _propertyConditionsList.put (pszKey, pPropertyCondition);
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "property key: %s, operator: %s, value: %s\n",
                        pszKey, pszOperator, pszValue);
                    return 0;
                }
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "Failed in creating the pPropertyCondition\n");
                return -1;
            }
        }
    }
    checkAndLogMsg (pszMethodName, Logger::L_Warning, "The pPropertyCondition %s is not well formatted\n",
                    pPropertyCondition);
    return -1;
}

PropertyCondition * PropertiesConditionsList::getPropertyCondition (const char *pszPropertyKey)
{
    return _propertyConditionsList.get (pszPropertyKey);
}

PropertyCondition::PropertyCondition (const char *pszValue, const char *pszOperator)
{
    _pszValue = pszValue;
    _pszOperator = pszOperator;
}

PropertyCondition::~PropertyCondition()
{
}

const char * PropertyCondition::getPropertyConditionValue (void)
{
    return _pszValue;
}

const char * PropertyCondition::getPropertyConditionOperator (void)
{
    return _pszOperator;
}

