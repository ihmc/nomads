/*
 * StringHashthing.h
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

#ifndef INCL_STRING_HASHGRAPH_H
#define INCL_STRING_HASHGRAPH_H

#include "Thing.h"

#include "Graph.h"
#include "LList.h"
//#include "StringHashtable.h"

namespace NOMADSUtil
{
    class StringHashthing : public StringHashtable<Thing>, public Thing
    {
        public:
            StringHashthing ();
            StringHashthing (const char * pszId);
            StringHashthing (Graph *pGraph, const char * pszId);
            virtual ~StringHashthing ();

            virtual void setGraph (Graph *pGraph);

            virtual Graph * getGraph (void);
            virtual const char * getId (void);

            virtual Thing * put (const char * pszId, Thing *pThing);

            virtual bool contains (const char * pszKey);
            virtual Thing * getThing (const char * pszKey);
            virtual Thing * getParent (void);

            virtual bool isReachable (const char * pszKey);
            virtual bool isReachable (PtrLList<String> *pList, const char * pszKey);

            virtual Thing * remove (const char * pszKey);

            virtual PtrLList<Thing> * list (void);
            virtual StringHashtable<Thing>::Iterator iterator (void);

            virtual Thing * clone (void);
            virtual Thing * deepClone (void);

            virtual double getWeight ();

            virtual int read (Reader *pReader, uint32 ui32MaxSize);
            virtual int write (Writer *pWriter, uint32 ui32MaxSize);
    };
}

#endif  // INCL_STRING_HASHGRAPH_H
