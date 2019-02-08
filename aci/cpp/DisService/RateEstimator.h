/*
 * RateEstimator.h
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
 * This class periodically checks the amount of missing messages for each
 * subscription if they're above a certain threshold, it may ask the sender
 * to slow down or not increase anymore the sending rate.
 *
 *  Author: Andrea Rossi    (arossi@ihmc.us)
 *  Created on: Mar 28, 2011
 */

#ifndef INCL_CC_RECEIVER_SIDE_H
#define INCL_CC_RECEIVER_SIDE_H

#include "FTypes.h"
#include "LoggingMutex.h"
#include "ManageableThread.h"
#include "UInt32Hashtable.h"
#include "StringHashtable.h"

#include "Listener.h"
#include "Services.h"

namespace IHMC_ACI
{
    class DisServiceMsg;
    class TransmissionService;

    class RateEstimator : public NOMADSUtil::ManageableThread, public MessageListener
    {
        public:
            RateEstimator (TransmissionService *pTrSvc, uint8 ui8UpdateFactor, uint8 ui8DecreaseFactor,
                           uint32 ui32StartingCapacity);
            virtual ~RateEstimator (void);

            int init (void);

            /**
             * Update the rate estimation stats if a Rate Control messages from
             * a neighbor arrives
             */
            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                     const char *pszIncomingInterface);

            void run (void);

            /**
             * Returns the capacity of the interface specified
             */
            uint32 getNetworkCapacity (const char *pszInterface);

            /**
             * Returns the capacity of the interface specified
             */
            uint32 getNetworkCapacityToAdvertise (const char *pszInterface);

            /**
             * Specify that the interface has been used in the last interval of
             * time
             */
            void setInterfaceIsActive (const char **ppszInterfaces);

            /**
             * The time between rateControl updates
             */
            static const uint32 DEFAULT_IDLE_TIME = 2000;

        private:
            struct NeighbourStats
            {
                NeighbourStats (void);
                virtual ~NeighbourStats (void);

                uint32 _ui32SendRate;     // the send rate of the node in Kbps
                uint32 _ui32EstimateRate; // the node's estimate of the network
                                          // capacity
                uint8 _ui8MissedUpdates;  // used to keep track of missed
                                          // updates, to remove dead nodes
            };

            struct InterfaceStats
            {
                InterfaceStats (void);
                virtual ~InterfaceStats (void);

                NOMADSUtil::UInt32Hashtable<NeighbourStats> _tStats;  //stats of the various neighbors

                bool _bActive;  // has the interface been used in the last
                                // period of time?
                uint32 _ui32FixedDrop;  // The estimated background constant drop
                                        // rate on the interface
                uint32 _ui32NetworkCapacity;  // estimate of the network capacity
                                              // around the node
                uint32 _ui32MyEstimatedNetworkCapacity;  // //the estimate I'll advertise to others
            };

            /**
             * estimates the conditions of the network.
             * it's based on the following variables.
             * dropPercentage = the % of packets lost, calculated using the receive rate and the sum of all neighbors send rates
             * fixedDropPercentage = the % of packets lost that _we think_ have been lost thanks to poor channel conditions and not because of congestion
             * congestionDropPercentage = % of packets lost because of congestion = dropPercentage - fixedDropPercentage
             * so, if all the neighbors together send 200 packets, this node receives 160 and the estimate for fixedDropPercentage is 5%, the situation will be this
             * dropPercentage = 20%
             * fixedDropPercentage = 5%
             * congestionDropPercentage = 15%
             *
             * using these variables, we can calculate the network capacity using this formula
             * capacity = (recvRate * (100 - congestionDropPercentage) / (100 - dropPercentage)) + sendRate * (100 - congestionDropPercentage) / 100
             *
             * which can be rewritten like this
             * capacity = ((recvRate / (100 - dropPercentage)) * ( 100 - dropPercentage + fixedDropPercentage)) + sendRate * (100 - dropPercentage + fixedDropPercentage) / 100
             *
             * we have 2 main parts in the equation:
             *
             * the first
             * ((recvRate / (100 - dropPercentage)) * ( 100 - dropPercentage + fixedDropPercentage))
             * this adjusts the recvRate, adding to it the part of messages lost due to bad channel conditions
             *
             * the second
             * sendRate * (100 - dropPercentage + fixedDropPercentage) / 100
             * this does the same thing for the sendRate. we adjust it because in pkforward only the successful packets affect the rate limit
             * on a real case we would not need to adjust it
             * (note that here we divide by 100 because the value we have for the sendRate
             * is the real one, not one guessed from the messages arrived, so it's not affected by the drop)
             *
             */
            uint32 estimateLinkCapacity (uint32 ui32SendRate, uint32 ui32Recvrate,
                                         uint32 ui32DropRate, uint32 ui32FixedDrop);

        private:
            const uint32 _ui32IdleTime;     // Time between rate estimations
            uint32 _ui32SequenceNumber;     // Sequence number of the messages
            const uint32 _ui32StartingCapacity;
            float _fUpdateFactor;           // How fast should ui32FixedDrop follow
                                            // the changes in drop rate [0, 1]
            const float _fDecreaseFactor;   // How fast should ui32FixedDrop decrease
                                            // if the network conditions are good.
                                            // Higher values mean less decrease [0, 1]

            TransmissionService *_pTrSvc;
            mutable NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::StringHashtable<InterfaceStats> _tInterfaces;
    };
}


#endif // INCL_CC_RECEIVER_SIDE_H
