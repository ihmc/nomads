/*
 * DisServiceQueryController.h
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

#ifndef INCL_DISSERVICE_QUERY_CONTROLLER_H
#define INCL_DISSERVICE_QUERY_CONTROLLER_H

#include "QueryController.h"

namespace IHMC_ACI
{
    class DataCacheInterface;
    class MetaData;
    class MetadataConfiguration;

    class DisServiceQueryController : public QueryController
    {
        public:
            DisServiceQueryController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore);
            ~DisServiceQueryController (void);

            // SearchListener methods
            void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                const void *pQuery, unsigned int uiQueryLen);

            void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId);

        private:
            static const char * const SUPPORTED_QUERY_TYPE;

            uint16 _ui16ClientID;
            MetadataConfiguration *_pMetadataConf;
            CommAdaptorManager *_pCommAdaptMgr;
            DataCacheInterface *_pDataCacheInterface;
    };
}

#endif  // INCL_DISSERVICE_QUERY_CONTROLLER_H

