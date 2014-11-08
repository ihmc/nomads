/*
 * StringHashgraph.h
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

#ifndef INCL_STRING_HASH_GRAPH_H
#define INCL_STRING_HASH_GRAPH_H

#include "Graph.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{

    class Thing;

    class StringHashgraph : public StringHashtable<Thing>, public Graph
    {
        public:
            /**
           * NOTE: when put() is called it makes a copy of the key.
           */
            StringHashgraph (bool bDirect=false);
            virtual ~StringHashgraph ();

            virtual Thing * put (const char * pszId, Thing *pThing);
            virtual bool contains (const char * pszKey);
            virtual Thing * get (const char * pszId);
            virtual Thing * remove (const char * pszId);

            virtual StringHashtable<Thing>::Iterator thingIterator(void);

            virtual bool isDirect (void);

            virtual unsigned short getVertexCount (void);

        private:
            bool _bDirect;
    };

}

#endif  // INCL_STRING_HASHGRAPH_H
