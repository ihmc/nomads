/*
 * NodeIdSet.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on August 22, 2014, 6:22 AM
 */

#ifndef INCL_NODE_ID_SET_H
#define	INCL_NODE_ID_SET_H

#include "StrClass.h"
#include "StringHashset.h"

namespace IHMC_VOI
{
    typedef NOMADSUtil::StringHashset::Iterator NodeIdIterator;

    class NodeIdSet
    {
        public:
            NodeIdSet (void);
            explicit NodeIdSet (const char *pszFirstNode);
            NodeIdSet (NodeIdSet &nodeIdSet);
            ~NodeIdSet (void);

            void add (const char *pszNodeId);
            bool contains (const char *pszNodeId) const;
            bool isEmpty (void) const;
            void remove (const char *pszNodeId);

            NodeIdIterator getIterator (void);

            operator const char * (void);
            bool operator == (NodeIdSet &rhsNodeIdSet);
            NodeIdSet & operator += (NodeIdSet &rhsNodeIdSet);
            NodeIdSet & operator = (NodeIdSet &rhsNodeIdSet);

        private:
            static const char SEPARATOR;
            bool _bUpdateString;
            mutable NOMADSUtil::String _sNodeIds;
            NOMADSUtil::StringHashset _nodeIds;
    };
}

#endif	/* INCL_NODE_ID_SET_H */
