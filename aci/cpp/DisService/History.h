/*
 * History.h
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
 */

#ifndef INCL_HISTORY_H
#define INCL_HISTORY_H

#include "FTypes.h"

namespace IHMC_ACI
{
    class Message;

    class History
    {
        public:
            History (int64 i64TimeOut);
            virtual ~History (void);

            bool isExpired (void);

            virtual bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender) = 0;

            int64 _i64TimeOut;
            int64 _i64HistoryReqTime;
    };

    class ShiftHistory : public History
    {
        public:
            ShiftHistory (uint16 ui16HistoryLength, int64 i64TimeOut);
            ~ShiftHistory (void);

            bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);

            uint16 _ui16HistoryLength;
    };

    class DiscreteHistory : public History
    {
        public:
            DiscreteHistory (uint32 ui32From, uint32 ui32To, int64 i64TimeOut);
            ~DiscreteHistory (void);

            bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);

            uint32 _ui32From;
            uint32 _ui32To;
    };

    class TimeHistory : public History
    {
        public:
            TimeHistory (int64 i64Since, int64 i64Until, int64 i64TimeOut);
            ~TimeHistory (void);

            bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);

            int64 _i64Since;
            int64 _i64Until;
    };

    /**
     * Wrappers that describe a history request
     */
    struct HistoryRequest
    {
        History *_pHistory;
        const char *_pszGroupName;
    };

    struct HistoryRequestGroupTag : public HistoryRequest
    {
        uint16 _ui16Tag;
    };

    struct HistoryRequestPredicate  : public HistoryRequest
    {
        uint8 ui8PredicateType;
        const char *_pszPredicate;
    };
}

#endif  // INCL_HISTORY_H
