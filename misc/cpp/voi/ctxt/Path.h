/*
 * Path.h
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Created on May 6, 2011, 11:37 PM
 */

#ifndef INCL_VOI_PATH_H
#define	INCL_VOI_PATH_H

#include "FTypes.h"

#include <stddef.h>

namespace IHMC_VOI
{
    class NodeContext;

    class Path
    {
        public:
            Path (void);
            virtual ~Path (void);

            virtual int getPathType (void) = 0;
            virtual int getPathLength (void) = 0;
            virtual float getLatitude (int iWaypointIndex) = 0;
            virtual float getLongitude (int iWaypointIndex) = 0;
            virtual uint64 getTimeStamp (int iWaypointIndex) = 0;
    };

    class AdjustedPathWrapper : public Path
    {
        public:
            AdjustedPathWrapper (void);
            AdjustedPathWrapper (NodeContext *pNodeContext);
            virtual ~AdjustedPathWrapper (void);

            int configure (NodeContext *pNodeContext);

            int getAdjustedCurrentWayPoint (void);
            int getPathType (void);
            int getPathLength (void);
            float getLatitude (int iWaypointIndex);
            float getLongitude (int iWaypointIndex);
            uint64 getTimeStamp (int iWaypointIndex);

        private:
            void configureCurrentWayPoint (NodeContext *pNodeContext);
            int getWayPoint (int iWaypointIndex);
            bool isAfterDetour (int iWaypointIndex);
            bool isBeforeDetour (int iWaypointIndex);
            bool isInDetour (int iWaypointIndex);

        private:
            int _iDetourStartIndex;
            int _iDetourLength;
            int _iPathLenght;
            NodeContext *_pNodeContext;
            Path *_pPath;
    };

    inline int AdjustedPathWrapper::getAdjustedCurrentWayPoint()
    {
        /*  A---- B
                 \    . CURRENT_POSITION
                  \
                   \
                    C
        Given the path ABC, if the node is in CURRENT_POSITION it is assumed that the
        node experienced a detour that started in the way point closest CURRENT_POSITION
        (in this case B). Thus, being _iDetourStartIndex set to B, the index of CURRENT_POSITION
        in the adjusted path is B+1*/
        return _iDetourStartIndex + 1;
    }

    inline int AdjustedPathWrapper::getPathType()
    {
        return (_pPath == NULL ? -1 : _pPath->getPathType());
    }

    inline bool AdjustedPathWrapper::isAfterDetour (int iWaypointIndex)
    {
        return iWaypointIndex > (_iDetourStartIndex + _iDetourLength);
    }

    inline bool AdjustedPathWrapper::isBeforeDetour (int iWaypointIndex)
    {
        return iWaypointIndex < _iDetourStartIndex;
    }

    inline bool AdjustedPathWrapper::isInDetour (int iWaypointIndex)
    {
        return (!isAfterDetour (iWaypointIndex) && !isBeforeDetour (iWaypointIndex));
    }
}

#endif	// INCL_VOI_PATH_H

