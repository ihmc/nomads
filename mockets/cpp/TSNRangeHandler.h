#ifndef INCL_TSN_RANGE_HANDLER_H
#define INCL_TSN_RANGE_HANDLER_H

/*
 * TSNRangeHandler.h
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

 * NOTE: The TSNRangeHandler and its subclasses do not use a mutex
 * Therefore, the caller must handle mutual exclusion and ensure that only one thread
 * invokes methods in these classes at any given time.
 */

#include "FTypes.h"

#include <stddef.h>

#include "ObjectDefroster.h"
#include "ObjectFreezer.h"

class TSNChunkMutator;

class TSNRangeHandler
{
    public:
        TSNRangeHandler (void);
        virtual ~TSNRangeHandler (void);

        // Returns true if there are any ranges, false if the list is empty
        bool haveInformation (void);

        int appendTSNInformation (TSNChunkMutator *pTCM);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    protected:
        // Returns 0 in case of a duplicate TSN or 1 if the TSN was successfully added
        virtual int addTSN (uint32 ui32TSN);

    protected:
        struct Node
        {
            Node (void);
            Node *pPrev;
            Node *pNext;
            uint32 ui32Begin;
            uint32 ui32End;
        };

    protected:
        Node *_pFirstNode;
        Node *_pLastNode;
};

class SAckTSNRangeHandler : public TSNRangeHandler
{
    //TSN means Transmission Sequence Number
    public:
        SAckTSNRangeHandler (void);

        int setCumulativeTSN (uint32 ui32CumulativeTSN);
        uint32 getCumulativeTSN (void);

        // Returns 0 in case of a duplicate TSN or 1 if the TSN was successfully added
        int addTSN (uint32 ui32TSN);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    protected:
        uint32 _ui32CumulativeTSN;
};

class CancelledTSNRangeHandler : public TSNRangeHandler
{
    public:
        // Returns 0 in case of a duplicate TSN or 1 if the TSN was successfully added
        int addTSN (uint32 ui32TSN);

        // Removes any TSNs that are in the list upto (and including) the specified TSN
        int deleteTSNsUpTo (uint32 ui32TSN);
};

// The following class is used to keep track of the sequence numbers of the messages
// that have already been received and delivered for the reliable/unsequenced flow
class ReceivedTSNRangeHandler : public TSNRangeHandler
{
    public:
        ReceivedTSNRangeHandler (void);

        // Add the sequence number of a message that has been reassembled and dequeued
        // for delivery to the application
        int addTSN (uint32 ui32TSN);

        // Check to see if the specified TSN has already been added in the past, indicating
        // that it has already been reassembled and dequeued for delivery to the application
        // Returns true if it has already been received, false otherwise
        bool alreadyReceived (uint32 ui32TSN);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    protected:
        bool _bAdded0TSN;    // Keeps track of whether the 0 TSN value has been added
        uint32 _ui32CumulativeTSN;
};

inline bool TSNRangeHandler::haveInformation (void)
{
    return (_pFirstNode != nullptr);
}

inline TSNRangeHandler::Node::Node (void)
{
    pPrev = pNext = nullptr;
    ui32Begin = ui32End = 0;
}

inline int SAckTSNRangeHandler::setCumulativeTSN (uint32 ui32CumulativeTSN)
{
    _ui32CumulativeTSN = ui32CumulativeTSN;
    return 0;
}

inline uint32 SAckTSNRangeHandler::getCumulativeTSN (void)
{
    return _ui32CumulativeTSN;
}

inline int CancelledTSNRangeHandler::addTSN (uint32 ui32TSN)
{
    return TSNRangeHandler::addTSN (ui32TSN);
}

#endif   // #ifndef INCL_TSN_RANGE_HANDLER_H
