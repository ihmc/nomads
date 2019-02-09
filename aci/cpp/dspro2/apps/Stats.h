/*
 * Stats.h
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
 * Created on June 26, 2012, 10:42 PM
*/

#ifndef INCL_DSPRO_STATISTICS_H
#define INCL_DSPRO_STATISTICS_H

#include "ManageableThread.h"
#include "Mutex.h"
#include "StrClass.h"
#include "StringHashtable.h"

#include <map>

namespace IHMC_VOI
{
    class MetadataInterface;
}

namespace IHMC_ACI
{
    struct MsgCounts
    {
        MsgCounts (const uint64 ui64Uni, const uint64 ui64Multi);

        const uint64 ui64UnicastMsgs;
        const uint64 ui64MulticastMsgs;

        static const char * MSG_COUNTS_INTERFACE_NAME;
        static const char * MSG_COUNTS_UNICAST_COUNT_NAME;
        static const char * MSG_COUNTS_MULTICAST_COUNT_NAME;
    };


    typedef std::map<std::string, MsgCounts> MsgCountsByInterface;


    class Stats : public NOMADSUtil::ManageableThread
    {
        private:


        public:
            static const int64 DEFAULT_TIMEOUT;

            ~Stats (void);

            static Stats * getInstance (const char *pszNodeId = nullptr, int64 i64Timeout = 0,
                                        const char *pszFilename = "dsprostats.csv");

            void run (void);

            void addMessage (const char *pszGroup, IHMC_VOI::MetadataInterface *pMetadata);
            void addMessage (const char *pszGroup, const char *pszMimeType = nullptr);

            void addMatch (const char *pszTargetPeer);
            void addMatch (const char *pszGroup, const char *pszMimeType);

            void getData (const char *pszId);
            void requestMoreChunks (const char *pszId);

            void messageCountUpdated (const char *pszPeerNodeId, const char *pszIncomingInterface,
                                      const char *pszPeerIp, uint64 ui64GroumMsgCount,
                                      uint64 ui64UnicastMsgCount);

            MsgCountsByInterface getPeerMsgCountsByInterface (const char * pszPeerNodeId) const;

        private:
            Stats (const char *pszNodeId, const char *pszFilename, int64 i64Timeout = DEFAULT_TIMEOUT);

        private:
            struct Counts
            {
                Counts (void);
                ~Counts (void);

                void update (uint64 ui64GroumMsgCount, uint64 ui64UnicastMsgCount);

                uint64 ui64Multi;
                uint64 ui64Uni;
            };


            typedef std::map<std::string, Counts> CountsByIncomingInterface;
            typedef std::map<std::string, CountsByIncomingInterface> CountsByInterface;


            const int64 _i64Timeout;
            const NOMADSUtil::String _filename;
            const std::string _nodeId;

            std::map<std::string, CountsByInterface> _mCumulativeCountsByPeer;
            std::map<std::string, CountsByInterface> *_pNewCountsByPeer;

            NOMADSUtil::StringHashtable<uint32> _usageById;

            NOMADSUtil::StringHashtable<uint32> _pubByMimeType;
            NOMADSUtil::StringHashtable<uint32> _pubByGroup;

            NOMADSUtil::StringHashtable<uint32> _matchesByMimeType;
            NOMADSUtil::StringHashtable<uint32> _matchesByGroup;
            NOMADSUtil::StringHashtable<uint32> _matchesByPeer;

            mutable NOMADSUtil::Mutex _mCountsByPeer;
            mutable NOMADSUtil::Mutex _mUsageById;
            mutable NOMADSUtil::Mutex _mPub;
            mutable NOMADSUtil::Mutex _mMatchByPeer;
            mutable NOMADSUtil::Mutex _mMatchByType;
    };
}

#endif  /* INCL_DSPRO_STATISTICS_H */
