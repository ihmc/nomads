/*
 * StackAllocator.cpp
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

#include "StackAllocator.h"

#include <stddef.h>
#include <stdio.h>

using namespace NOMADSUtil;

StackAllocator::StackAllocator (unsigned long ulMemBlockSize)
{
    _pMemBlock = new char[ulMemBlockSize];
    _ulBlockSize = ulMemBlockSize;
    _pNextFree = _pMemBlock;
}

StackAllocator::~StackAllocator (void)
{
    delete[] _pMemBlock;
}

void * StackAllocator::alloc (size_t size)
{
    if (((_pNextFree-_pMemBlock) + size) > _ulBlockSize) {
        return NULL;
    }
    void *pMem = _pNextFree;
    _pNextFree += size;
    return pMem;
}

void * StackAllocator::reserve (size_t size)
{
    // For now, there is no difference between alloc and reserve
    return alloc (size);
}

void StackAllocator::dealloc (void *pMem)
{
    _pNextFree = (char*) pMem;
}
