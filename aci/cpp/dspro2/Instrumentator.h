/** 
 * Instrumentator.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 24, 2010, 11:54 AM
 */

#ifndef INCL_INSTRUMENTATOR_H
#define	INCL_INSTRUMENTATOR_H

#include "MatchmakingIntrumentation.h"

#include "DArray2.h"
#include "FTypes.h"
#include "Logger.h"
#include "LoggingMutex.h"
#include "PtrLList.h"

namespace IHMC_ACI
{
    class MatchmakingLogListener;

    class Instrumentator
    {
        public:
            Instrumentator (NOMADSUtil::LoggingMutex *pmCallback);
            virtual ~Instrumentator (void);

            int registerAndEnableMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pMatchmakingListener, uint16 &ui16AssignedClientId);
            int deregisterAndDisableMatchmakingLogListener (uint16 ui16ClientId);

            bool isEnabled (void);

            void notify (MatchmakingIntrumentation *pIntrumentation);
            static void notifyAndRelease (NOMADSUtil::PtrLList<MatchmakingIntrumentation> *pInstrumentations);

        private:
            struct MatchmakerClientInfoPro
            {
                MatchmakerClientInfoPro (void);
                ~MatchmakerClientInfoPro (void);

                MatchmakingLogListener *pMatchmakerListener;
            };
            NOMADSUtil::DArray2<Instrumentator::MatchmakerClientInfoPro> _matchmakerClients;
            unsigned int _uiNListeners;
            NOMADSUtil::LoggingMutex *_pmCallback;
    };

    extern Instrumentator *pInstrumentator;
    extern NOMADSUtil::Logger *pNetLog;

    inline bool Instrumentator::isEnabled()
    {
        return _uiNListeners > 0;
    }
}

#endif	// INCL_INSTRUMENTATOR_H

