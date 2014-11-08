/*
 * FIFOQueue.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "FIFOQueue.h"
#include <stddef.h>

using namespace NOMADSUtil;

FIFOQueue::FIFOQueue (void)
{
    pHead = NULL;
    pTail = NULL;
    ui32Count = 0;
}

FIFOQueue::~FIFOQueue (void)
{
    QueueNode *pTemp;
    while (pHead != NULL) {
        pTemp = pHead;
        pHead = pHead->pNextNode;
        delete pTemp;
    }
    pHead = pTail = NULL;
}

bool FIFOQueue::isEmpty (void)
{
    return (pHead == NULL);
}

int FIFOQueue::enqueue (void *pValue)
{    
    QueueNode *pNewNode;
    if (NULL == (pNewNode = new QueueNode)) {
        return -1;
    }
    pNewNode->data = pValue;
    pNewNode->pNextNode = NULL;
    if (pHead == NULL) {
        pHead = pNewNode;
        pTail = pNewNode;
    }
    else {
        pTail->pNextNode = pNewNode;
        pTail = pNewNode;
    }
    ui32Count++;
    return 0;
}

void* FIFOQueue::dequeue (void)
{
    void *pValue;
    if (pHead == NULL) {
        pValue = NULL;
    }
    else if (pHead == pTail) {
        pValue = pHead->data;
        delete pHead;
        pHead = pTail = NULL;
        ui32Count--;
    }
    else {
        QueueNode *pTempNode = pHead;
        pValue = pHead->data;
        pHead = pHead->pNextNode;
        delete pTempNode;
        ui32Count--;
    }
    return pValue;
}
