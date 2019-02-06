/*
 * FIFOQueue.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_FIFO_QUEUE_H
#define INCL_FIFO_QUEUE_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class FIFOQueue
    {
        public:
            FIFOQueue (void);
            ~FIFOQueue (void);
            bool isEmpty (void);
            uint32 sizeOfQueue (void);
            int enqueue (void *pValue);
            void * dequeue (void);
        private:
            struct QueueNode {
                void* data;
                QueueNode *pNextNode;
            };
            QueueNode *pHead;         // Contained
            QueueNode *pTail;         // Contained
            uint32 ui32Count;
    };

    inline uint32 FIFOQueue::sizeOfQueue (void)
    {
        return ui32Count;
    }

}

#endif   // #ifndef INCL_FIFO_QUEUE_H
