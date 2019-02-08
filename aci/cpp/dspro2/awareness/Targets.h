/**
 * Targets.h
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
 * Created on July 25, 2012, 2:30 PM
 */

#ifndef INCL_TARGETS_H
#define INCL_TARGETS_H

#include "Defs.h"

#include "DArray.h"


namespace IHMC_ACI
{
    class Targets
    {
        public:
            Targets (void);
            Targets (unsigned int uiSize);
            Targets (const Targets & targets);
            ~Targets (void);

            Targets & operator= (Targets targets);

            int firstFreeTargetNodeId (void);
            int firstFreeInterface (void);

            char ** getTargetNodeIds (void);
            char ** getInterfaces (void);

            /**
             * Return true if the target passed as argument was subsumed, false
             * otherwise
             */
            bool subsume (const Targets & target);

            static void deallocateTarget (Targets * pTargets);
            static void deallocateTargets (Targets ** ppTargets);

            void log (void);

        private:
            int firstFreeInternal (NOMADSUtil::DArray<char *> & array);
            char ** toArray (NOMADSUtil::DArray<char *> & array);

        public:
            AdaptorId adaptorId;
            NOMADSUtil::DArray<char *> aTargetNodeIds;
            NOMADSUtil::DArray<char *> aInterfaces;
    };


    typedef Targets* TargetPtr;

    inline Targets::Targets() { }

    inline Targets::Targets (unsigned int uiSize) :
        aTargetNodeIds (static_cast<char *> (nullptr), uiSize),
        aInterfaces (static_cast<char *> (nullptr), uiSize)
    { }

    inline int Targets::firstFreeTargetNodeId (void)
    {
        return firstFreeInternal (aTargetNodeIds);
    }

    inline int Targets::firstFreeInterface (void)
    {
        return firstFreeInternal (aInterfaces);
    }

    inline char ** Targets::getTargetNodeIds (void)
    {
        return toArray (aTargetNodeIds);
    }

    inline char ** Targets::getInterfaces (void)
    {
        return toArray (aInterfaces);
    }

    inline char ** Targets::toArray (NOMADSUtil::DArray<char *> & array)
    {
        if (array[array.getHighestIndex()] != nullptr) {
            array[array.size()] = nullptr;  // Makes sure that the last element is nullptr
        }
        return array.getData();
    }
}

#endif    /* INCL_TARGETS_H */
