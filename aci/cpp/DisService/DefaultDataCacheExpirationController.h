/*
 * DefaultDataCacheExpirationController.h
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

#ifndef INCL_DEFAULT_DATE_CACHE_EXPIRATION_CONTROLLER_H
#define INCL_DEFAULT_DATE_CACHE_EXPIRATION_CONTROLLER_H

#include "DataCacheExpirationController.h"

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_ACI
{
    class DataCache;
    class MessageHeader;
    class MessageInfo;
    class DisseminationService;

    class DefaultDataCacheExpirationController : public DataCacheExpirationController
    {
        public:
            DefaultDataCacheExpirationController (DisseminationService *pDisService);
            ~DefaultDataCacheExpirationController ();

            // DisService -> DataCacheExpirationController
            void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad);
            void capacityReached (void);
            void thresholdCapacityReached (uint32 ui32Length);
            void spaceNeeded (uint32 ui32bytesNeeded, MessageHeader * pIncomingMgsInfo, void * pIncomingData);
            int cacheCleanCycle (void);

        private:
            bool _bPurelyProbForwarding;

            TransmissionHistoryInterface *_pTransmissionHistory;
    };
}

#endif  // INCL_DEFAULT_DATE_CACHE_EXPIRATION_CONTROLLER_H
