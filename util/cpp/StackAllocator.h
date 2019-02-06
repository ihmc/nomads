/*
 * StackAllocator.h
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

#ifndef INCL_STACK_ALLOCATOR_H
#define INCL_STACK_ALLOCATOR_H

#include <stddef.h>

/*
 * StackAllocator
 *
 * Defines an allocator for memory that is stack based. That is,
 * requests for allocation and deallocation of memory must be in
 * LIFO order.
 *
 */

#include <stddef.h>

namespace NOMADSUtil
{
    class StackAllocator
    {
        public:
            StackAllocator (unsigned long ulMemBlockSize);
            ~StackAllocator (void);

            // Allocate a block of memory of the specified size
            // Returns a pointer to the block of memory
            void * alloc (size_t size);

            // Reserve a block of memory of the specified size beyond the last allocated block
            // Returns a pointer to the block of memory
            // NOTE: A block that has been reserved should not be explictly deallocated
            //     It will automatically be deallocated when the last allocated block is deallocated
            void * reserve (size_t size);

            // Deallocate the specified block of memory
            // NOTE: Must be the last block of memory that was allocated!
            void dealloc (void *pMem);

        protected:
            char *_pMemBlock;
            unsigned long _ulBlockSize;
            char *_pNextFree;
    };
}

#endif   // #ifndef INCL_STACK_ALLOCATOR_H
