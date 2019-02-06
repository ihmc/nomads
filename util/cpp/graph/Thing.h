/*
 * Thing.h
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

#ifndef INCL_THING_H
#define INCL_THING_H

#include "FTypes.h"
#include "Graph.h"
#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class Graph;
    class Reader;
    class Writer;

    class Thing
    {
        public:
            Thing (const char * pszId);
            Thing (Graph *pGraph, const char * pszId);
            virtual ~Thing ();

            virtual Graph * getGraph (void);
            virtual const char * getId (void);

            virtual void setGraph (Graph *pGraph);

            /**
             * It returns the old value if present, NULL otherwise
             */
            virtual Thing * put (const char * pszId, Thing *pThing)=0;
            virtual bool contains (const char * pszKey)=0;

            /**
             * It returns the neighbor identified by pszKey if present, null otherwise
             */
            virtual Thing * getThing (const char * pszKey)=0;

            virtual Thing * getParent (void)=0;

            /**
             * Return true if the thing with the id pszKey is reachable
             */
            virtual bool isReachable (const char * pszKey)=0;
            // pList is the list of thing that I know for sure that don't contain the thing pszKey
            virtual bool isReachable (PtrLList<String> *pList, const char * pszKey)=0;

            // It returns the neighbor identified by pszKey if the thing is  correctly removed, null otherwise
            virtual Thing * remove (const char * pszKey)=0;

            virtual StringHashtable<Thing>::Iterator iterator (void)=0;

            /**
             * Returns all the Things in the graph ordered by number of hops and weight
             */
            virtual PtrLList<Thing> * list (void)=0;

            virtual double getWeight ()=0;

            /**
             * Return a copy of the object
             */
            virtual Thing * clone (void)=0;

            /**
             * Return a copy of the object and and copy the neighbors recursively
             */
            virtual Thing * deepClone (void)=0;

            virtual int read (Reader *pReader, uint32 ui32MaxSize)=0;
            virtual int write (Writer *pWriter, uint32 ui32MaxSize)=0;

        protected:
            Graph * _pGraph;
            String _pszId;
    };
}

#endif   // INCL_THING_H
