/*
 * ConnectivityHistory.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * ConnectivityHistory maintains statistics on the number
 * of encounters with other nodes.
 *
 * Author: Mirko Gilioli mgilioli@ihmc.us
 * Created on May 21, 2009, 2:57 PM
 */

#ifndef INLC_CONNECTIVITY_HISTORY_H
#define	INLC_CONNECTIVITY_HISTORY_H

#include "ConditionVariable.h"
#include "LoggingMutex.h"
#include "PtrLList.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class ConnectivityHistory
    {
        public:
            ConnectivityHistory (int64 i64WinSize);
            virtual ~ConnectivityHistory (void);

            /*
             * Add a new nodeId to the ConnectivityHistory.
             * Return 0 if the procedure succeed.
             * Return -1 if the pszNodeId is NULL
             */
            int add (const char *pszNodeId);

            /*
             * Return the number of nodes in the Connectivity History with TimeStamp
             * inside the Window. The Window dimension is set when the object is created.
             * Any time this method is called, nodes outside the window are eliminated
             * from the ConnectivityHistory.
             */
            uint8 getCount (void);

            /*
             * This method sets the Window size of the ConnectivityHistory
             */
            void setWindowSize (int64 i64WinSize);

        private:

            struct Entry {
                Entry (const char *pszNodeId, int64 i64CurrentTime);
                ~Entry();
                const NOMADSUtil::String _nodeId;
                const int64 _i64TimeStamp;

                bool operator > (const Entry &rhsEntry);
                bool operator == (const Entry &rhsEntry);
                bool operator < (const Entry &rhsEntry);
            };

            uint8 _ui8CountNodes;
            int64 _i64WinSize;
            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::PtrLList<Entry> _sortedList;
    };

    inline ConnectivityHistory::Entry::Entry (const char *pszNodeId, int64 i64CurrentTime)
        : _nodeId (pszNodeId),
          _i64TimeStamp (i64CurrentTime)
    {
    }

    inline ConnectivityHistory::Entry::~Entry()
    {
    }

    inline bool ConnectivityHistory::Entry::operator < (const Entry &rhsEntry)
    {
        return (_i64TimeStamp < rhsEntry._i64TimeStamp);
    }

    inline bool ConnectivityHistory::Entry::operator > (const Entry& rhsEntry)
    {
        return (_i64TimeStamp > rhsEntry._i64TimeStamp);
    }

    inline void ConnectivityHistory::setWindowSize (int64 i64WinSize)
    {
        _i64WinSize = i64WinSize;
    }
}

#endif  // INLC_CONNECTIVITY_HISTORY_H
