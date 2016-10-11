/*
 * StringHashgraph.cpp
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

#include "StringHashgraph.h"

#include "Thing.h"

using namespace NOMADSUtil;

StringHashgraph::StringHashgraph (bool bDirect)
    : Graph (bDirect), StringHashtable<Thing> (true, true, true, true)
{
}

StringHashgraph::~StringHashgraph ()
{    
}

Thing * StringHashgraph::put (const char * pszId, Thing * pThing)
{
    Thing * pOld = StringHashtable<Thing>::remove (pszId);
    StringHashtable<Thing>::put (pszId, pThing);
    return pOld;
}

bool StringHashgraph::contains (const char * pszKey)
{
    return StringHashtable<Thing>::containsKey (pszKey);
}

Thing * StringHashgraph::get (const char * pszId)
{
    return StringHashtable<Thing>::get (pszId);
}

Thing * StringHashgraph::remove (const char * pszId)
{
    return StringHashtable<Thing>::remove (pszId);
}

StringHashtable<Thing>::Iterator StringHashgraph::thingIterator(void)
{
    return StringHashtable<Thing>::getAllElements ();
}

bool StringHashgraph::isDirect (void)
{
    return Graph::isDirect ();
}

unsigned short StringHashgraph::getVertexCount ()
{
    return StringHashtable<Thing>::getCount();
}
