/*
 * DSProQueryController.h
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
 * Author: Rita Lenzi    (rlenzi@ihmc.us)
 * Created on February 11, 2013, 09:35 AM
 */

#ifndef INCL_DSPRO_QUERY_CONTROLLER_H
#define INCL_DSPRO_QUERY_CONTROLLER_H

#include "QueryController.h"

#include "PtrLList.h"
#include "SearchProperties.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class DataStore;
    class InformationStore;
    class MetaData;
    class SQLAVList;

    class PropertyCondition
    {
        public:
            PropertyCondition (const char *pszValue, const char *pszOperator);
            ~PropertyCondition (void);

            const char * getPropertyConditionValue (void);
            const char * getPropertyConditionOperator (void);

        private:
            const char *_pszValue;
            const char *_pszOperator;
    };

    class PropertiesConditionsList
    {
        public:
            PropertiesConditionsList (void);
            ~PropertiesConditionsList (void);

            int addNewPropertyCondition (const char *pPropertyCondition);

            PropertyCondition * getPropertyCondition (const char *pszPropertyKey);

        private:
            static const char * const SUPPORTED_OPERATORS[];
             NOMADSUtil::StringHashtable<PropertyCondition> _propertyConditionsList;
    };

    class DSProQueryController : public QueryController
    {
        public:
            DSProQueryController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore);
            ~DSProQueryController (void);

            // SearchListener methods
            void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                const void *pQuery, unsigned int uiQueryLen);

            void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId);
            void volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId);

        private:
            int doXPathQuery (const void *pQuery, unsigned int uiQueryLen, const char *pszQueryQualifiers,
                              NOMADSUtil::PtrLList<const char> *pResultsList);
            int doSQLQueryOnMetadata (const char *pszGroupName, const void *pQuery, unsigned int uiQueryLen,
                                      const char *pszQueryQualifiers, InformationStore *pInfoStore,
                                      NOMADSUtil::PtrLList<const char> *pResultsList);
            int doQueryWithPropertyListOnApplicationMetadata (const void *pQuery, unsigned int uiQueryLen, 
                                                              const char *pszQueryQualifiers,
                                                              NOMADSUtil::PtrLList<const char> *pResultsList);
            char ** getMatchingIds (const char *pszQueryId, const char *pszGroupName, const char *pszQueryType,
                                    const char *pszQueryQualifiers, const void *pQuery, unsigned int uiQueryLen);
            int matchPropertyListToApplicationMetadata (char *pszApplicationMetadataBuffer, PropertiesConditionsList *pPropertiesConditionsList,
                                                        MetadataInterface *pCurr, NOMADSUtil::PtrLList<const char> *pResultsList);

        private:
            static const char * const SUPPORTED_QUERY_TYPES[];       
    };
}

#endif  // INCL_DSPRO_QUERY_CONTROLLER_H
