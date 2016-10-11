/*
 * BandwidthSharing.h
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
 *
 * This class determines how neighbor nodes share
 * the available network capacity among them.
 *
 * Created on: May 20, 2011
 * Author: Andrea Rossi    (arossi@ihmc.us)
 */

#ifndef INCL_BANDWIDTH_SHARING_H
#define INCL_BANDWIDTH_SHARING_H

#include "WorldState.h"
#include "TransmissionService.h"

namespace IHMC_ACI
{
    class DisseminationService;
    class RemoteNodeInfo;
    class PeerState;
    class TransmissionService;

    class BandwidthSharing
    {
        public:
            BandwidthSharing (PeerState *pPeerState, TransmissionService *pTrSvc);
            virtual ~BandwidthSharing (void);

            /**
             * Changes all the rate limits on the various interfaces of the node
             * to respect the bandwidth sharing rules.
             * Returns true if the current sharing rule uses the queue length
             */
            bool isQueueSizeRequired (void);

            void adjustBandwidthShare (uint8 ui8Importance);

            enum SharingRule {
                BS_EqualSharing = 0x00,
                BS_HigherPriorityDominates = 0x01,
                BS_ProportionalSharing = 0x02,
                BS_ProportionalSharingWithQueueLength = 0x03,
                BS_BandwidthCappedHPD = 0x04,
                BS_BandwidthCappedPSWQL = 0x05,
                BS_NoSharing = 0xFF
            };

            /**
             * Sets the sharing rule to use
             */
            void setSharingRule (int iSharingRule);

        private:
            /*
             * Returns the rate limit for the specified interface, using the
             * current rule.
             */
            uint32 calculateRateLimitByInterface (char *pszInterfaceName, uint8 ui8Importance);

            //the different sharing rules
            /*
             # 0  -> Equal Sharing:
             #       the capacity of each node is equal to
             #       total capacity / (number of neighbours + 1)
             # 1  -> Higher Priority Dominates:
             #       the nodes with higher priority share equally
             #       between them the link capacity, the others can send msgs at 5Kbps
             # 2  -> Proportional Sharing:
             #       the weight a node has in sharing the bandwidth is
             #       (8 - nodeImportance), so more important nodes get a bit more bandwidth
             # 3  -> Proportional Sharing With Queue Length:
             #        the weight a node has in sharing
             #       (8 - nodeImportance) * (queueLengthFactor)^3, so more important nodes
             #       get a bit more bandwidth, and nodes with data to send get a lot more
             #       bandwidth. queueLengthFactor is higher when the queue is fuller
             # 4  -> Bandwidth Capped HPD :
             #       like number 1, but if the sum of the rate caps of all the most important
             #       nodes is lower than the link capacity, the remaining capacity is split
             #       equally between the other nodes
             # 5  -> Bandwidth Capped PSQL :
             #       like number 3, but if a node rate cap is lower than its bandwidth share
             #       the remaining bandwidth is split between the other nodes
             # 255 -> No Sharing:
             #        the node rate limit is the link capacity
             */
            uint32 sharingRuleEqualSharing (char *pszInterfaceName);
            uint32 sharingRuleHigherPriorityDominates (char *pszInterfaceName, uint8 ui8Importance);
            uint32 sharingRuleProportionalSharing (char *pszInterfaceName, uint8 ui8Importance);
            uint32 sharingRuleProportionalSharingWithQueueLength (char *pszInterfaceName, uint8 ui8Importance);
            uint32 sharingRuleBandwidthCappedHPD (char *pszInterfaceName, uint8 ui8Importance);
            uint32 sharingRuleBandwidthCappedPSWQL (char *pszInterfaceName, uint8 ui8Importance);
            uint32 sharingRuleNoSharing (char *pszInterfaceName);

            // Utility functions

            /*
             * Given a queue length in the range 0-255, returns a priority value
             * 0 is highest, 7 is lowest.
             */
            uint8 getQueuePriorityRange (uint8 ui8QueueLength);

            /*
             * Returns the weight of the node for the calculation of the bandwidth
             * shares when the rule is ProportioanlSharing
             */
            inline uint32 getNodeWeightPS (uint8 ui8NodeImportance);

            /*
             * Returns the weight of the node for the calculation of the bandwidth
             * shares when the rule is ProportioanlSharingWithQueueLength.
             */
            inline uint32 getNodeWeightPSWQL (uint8 ui8QueueLength, uint8 ui8NodeImportance);

            /*
             * Obtain from PeerState's bandwidth an indication of what could be the
             * RateLimitCap of the node.
             */
            uint32 getRateCapFromPeerStateBandwidth (uint8 ui8Bandwidth);

            SharingRule _activeRule;
            bool _bRequireQueueSizes;       // true if _activeRule requires queue length to work
            uint32 _ui32MinGuaranteedBw;    // the lowest bw amount a node can get

            PeerState *_pPeerState;
            TransmissionService *_pTrSvc;

            struct BandwidthCappedNodes {
                uint32 _ui32MaxRate;
                uint32 _ui32Rate;
            };

            NOMADSUtil::Mutex _m;
    };

}
#endif /* INCL_BANDWIDTH_SHARING_H */
