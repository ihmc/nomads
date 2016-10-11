/*
 * ConfigFileReader.h
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
 * ConfigFileReader offers utility methods to parse
 * and validate the configuration values read from a
 * config file.
 */

#ifndef INCL_CONFIG_FILE_READER_H
#define INCL_CONFIG_FILE_READER_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI 
{
    class TransmissionService;

    class ConfigFileReader 
    {
        public:
            ConfigFileReader (NOMADSUtil::ConfigManager *pCfgMgr);
            ~ConfigFileReader (void);

            void init (NOMADSUtil::ConfigManager *pCfgMgr);

            int64 getConnectivityHistoryWindowSize (void);

            /**
             * NOTE: the caller must de-allocate the returned array
             */
            char ** getNetIFs (void);
            char ** getIgnoredNetIFs (void);
            char ** getAddedNetIFs (void);
            static char ** parseNetIFs (const char *pszPropertyValue);

            /**
             * The possible return values are the following:
             * PUSH_ENABLED: pushed messages are cached and tranmitted to the
             *               default BROADCAST_ADDRESS/MULTICAST_GROUP
             * PUSH_DISABLED: pushed messages are cached
             */
            uint8 getTransmissionMode (void);

            /**
             * Returns true if the node is allowed to send keep alive messages,
             * false otherwise
             */
            bool getKeepAliveMsgEnabled (void);

            /**
             * Returns the keep alive interval in milliseconds
             */
            uint16 getKeepAliveInterval (void);

            /**
             * Returns the number of milliseconds after which a peer is not
             * considered in communication range anymore
             */
            uint32 getDeadPeerInterval (void);

            bool getQueryDataCacheEnabled (void);
            uint8 getQueryDataCacheReply (void);

            bool getSubscriptionStateExchangeEnabled (void);
            uint16 getSubscriptionStatePeriod (void);

            //=====================================================
            // Missing Fragment Requests handling
            //=====================================================

            // Generating the missing fragment request

            enum FragReqGenMod {FIXED, PRIORITY_DEPENDENT};
            FragReqGenMod getMissingFragReqProbabilityMode (uint8 ui8Probability);

            /**
             * Returns the probability with which a request for missing fragments
             * that belong to a mesaage with probability ui8Probability, will be
             * sent.
             */
            float getMissingFragReqProbability (void);

            /**
             * Returns the probability with which a request for missing fragments
             * that belong to a mesaage with probability ui8Probability, will be
             * sent.
             */
            float getMissingFragReqProbabilityByPriority (uint8 ui8Probability);

            // Serving the missing fragment request

            /**
             * Returns the interval of time (in milliseconds) starting from the
             * last time a certain data was transmitted, in which further requests
             * forthe same data should be ignored.
             *
             * It is orthogonal to the missing fragment request reply mode.
             */
            uint16 getIgnoreRequestInterval (void);

            //=====================================================
            // Node Configuration
            //=====================================================

            uint8 getMemorySpace (void);

            /**
             * Returns the speed of the interface
             */
            uint32 getBandwidth (void);
            uint8 getBandwidthIndex (uint32 ui32Bandwidth);

            bool isDataCacheStateEnabled (void);
            bool isSubscriptionStateEnabled (void);
            bool isTopologyStateEnabled (void);

            //=====================================================
            // Chunk Replication Policy methods Configuration
            //=====================================================
            uint32 getChunkSize (void);
            uint8 getPolicyID (void);

            uint8 getBandwidthThreshold (void);
            uint8 getMemoryThreshold (void);
            uint8 getNumOfClasses (void);
            uint8 getMemoryFactor (void);
            uint8 getBandwidthFactor (void);
            uint8 getActiveNeighborsFactor (void);
            uint8 getNodesInConnectivityHistoryFactor (void);

            //=====================================================
            // Transmission Service Listener Configuration
            //=====================================================

            bool isTargetIDFilteringEnabled (void);

            //=====================================================
            // Data Request Handler Configuration
            //=====================================================

            int64 getDataRequestHandlerSleepTime (void);
            int64 getDataRequestHandlerBaseTime (void);
            uint16 getDataRequestHandlerOffsetRange (void);
            
            //=====================================================
            // Topology
            //=====================================================
             
            bool getSubscriptionsExchangeEnabled (void);
            uint32 getSubscriptionsExchangePeriod (void);
            bool getTopologyExchangeEnabled (void);
            uint32 getTopologyExchangePeriod (void);

            float getProbContact (void);
            float getProbThreshold (void);
            float getAddParam (void);
            float getAgeParam (void);
            
        private:
            NOMADSUtil::ConfigManager *_pCfgMgr;
    };
}

#endif  // INCL_CONFIG_FILE_READER_H
