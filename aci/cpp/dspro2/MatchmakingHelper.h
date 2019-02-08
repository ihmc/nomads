/*
 * MatchmakingHelper.h
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
 * Created on September 11, 2013, 5:06 PM
 */

#ifndef INCL_INFORMATION_STORE_HELPER_H
#define INCL_INFORMATION_STORE_HELPER_H

#include "MetadataInterface.h"

namespace IHMC_ACI
{
    class DSPro;
    class InformationStore;
    class NodeContextImpl;
    class TransmissionHistoryInterface;
    class Topology;

    class MatchmakingHelper
    {
        public:
            MatchmakingHelper (InformationStore *pInfoStore, Topology *pTopology,
                               TransmissionHistoryInterface *pTrHistory);
            virtual ~MatchmakingHelper (void);

            IHMC_VOI::MetadataList * getMetadataToMatchmake (NodeContextImpl *pNodeContext, const char *pszSenderNodeId,
                                                             bool bEnableTopologyExchange, bool bLocalMatchmakingOnly) const;
            static void deallocateMetadataToMatchmake (IHMC_VOI::MetadataList *pMetadataList);

        private:
            InformationStore *_pInfoStore;
            Topology *_pTopology;
            TransmissionHistoryInterface *_pTrHistory;
    };
}

#endif    /* INFORMATIONSTOREHELPER_H */
