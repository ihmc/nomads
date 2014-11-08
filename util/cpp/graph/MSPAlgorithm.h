/*
 * MSPAlgorithm.h
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
 *
 * Author: Giacomo Benincasa (gbenincasa@ihmc.us)
 * Created on December 5, 2008, 3:34 PM
 */

#ifndef INCL_MSP_Algorithm_H
#define INCL_MSP_Algorithm_H

namespace NOMADSUtil
{
    class Graph;
    class Thing;

    class MSPAlgorithm
    {
        /**
         * Generate a Minimum Spanning Tree by the Prim's Algorithm.
         *
         * NOTE: pInitialGraph must be connected.
         * TODO: check for the pInitialGraph to be connected.
         * NOTE: if pInitialGraph contains no vertex, the method returns NULL.
         * NOTE: mpsGraph contains copies of the vertex and of the edges in pInitialGraph
         */
        public:
            static void MSPPrimAlgorithm (Graph *pInitialGraph, Graph &mpsGraph, Thing * pStartingVertex);
    };
}

#endif  //INCL_MSP_Algorithm_H
