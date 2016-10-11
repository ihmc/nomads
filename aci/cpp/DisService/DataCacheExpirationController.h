/*
 * DataCacheExpirationController.h
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

#ifndef INCL_DATA_CACHE_EXPIRATION_CONTROLLER_H
#define INCL_DATA_CACHE_EXPIRATION_CONTROLLER_H

#include "Listener.h"
#include "Services.h"

#include "DArray2.h"
#include "FTypes.h"

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_ACI
{
    class DataCacheInterface;
    class MessageInfo;
    class MessageHeader;
    class DisseminationService;

    class DataCacheExpirationController : public DataCacheListener, public DataCacheService
    {
        public:
            DataCacheExpirationController (DisseminationService *pDisService);
            virtual ~DataCacheExpirationController (void);

            // DisService -> DataCacheExpirationController
            virtual void capacityReached (void)=0;
            virtual void thresholdCapacityReached (uint32 ui32Length)=0;
            virtual void spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo, void *pIncomingData)=0;
            virtual int cacheCleanCycle (void)=0;

            // DataCacheExpirationController -> DisService

            virtual void setCapacityThreashold (uint32 ui32Capacity);
            virtual void setCacheCleanCycle (uint16 ui16Cycle);
            /**
             * Returns the id of all the expired entries.
             */
            NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredMessageIDs (void);
		
            void lockDataCache (void);
            void releaseDataCache (void);

            /**
             * Returns the amount of bytes freed up or a negative value
             * if an error occured.
             */
            virtual int deleteMessage (const char * pszKey);

        protected:
            DataCacheInterface *_pDataCacheInterface;
            DisseminationService * _pDisService;
    };
}

#endif // end INCL_DATA_CACHE_EXPIRATION_CONTROLLER_H
