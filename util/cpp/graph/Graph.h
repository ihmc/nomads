/*
 * Graph.h
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
 *
 * Author: Giacomo Benincasa (gbenincasa@ihmc.us)
 * Created on December 5, 2008, 3:34 PM
 */

#ifndef INCL_GRAPH_H
#define INCL_GRAPH_H

#include "StringHashtable.h"
#include "Thing.h"

namespace NOMADSUtil
{
    class Thing;
}

namespace NOMADSUtil
{
    class Graph
    {
        public:
            Graph (bool bDirect);
            virtual ~Graph (void);

            /**
             * Add/replace the element in the Graph with the new element pointed
             * by pThing.
             * The old element is returned (or NULL in case there were any element
             * with id pszId).
             */
            virtual Thing * put (const char * pszId, Thing *pThing)=0;

            /**
             * Return true if the graph containis a node whose is is pszKey
             */
            virtual bool contains (const char * pszKey)=0;

            /**
             * If the graph contains the node whose id is pszId, it returns
             * the Thing, NULL otherwise
             */
            virtual Thing * get (const char * pszId)=0;

            /**
             * Remove the object with id pszId if present and returns it.
             * NB: Graph DOES NOT free up the memory of the removed Thing,
             * it only free up the memory of the key.
             */
            virtual Thing * remove (const char * pszId)=0;

            virtual StringHashtable<Thing>::Iterator thingIterator(void)=0;

            bool isDirect (void) {return _bDirect;}

            /**
             * Returns the number of the Vertex in the Graph.
             */
            virtual unsigned short getVertexCount (void)=0;

        private:
            bool _bDirect;
    };
}

#endif  // INCL_GRAPH_H
