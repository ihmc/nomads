/*
 * StringHashthing.cpp
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
 */

#include "StringHashthing.h"

#include "FTypes.h"
#include "NLFLib.h"
#include "Reader.h"
#include "Writer.h"

using namespace NOMADSUtil;

StringHashthing::StringHashthing ()
    : StringHashtable<Thing> (true,     // bCaseSensitiveKeys
                                  true,     // bCloneKeys
                                  true,     // bDeleteKeys
                                  false),   // bDeleteValues
       Thing (NULL, NULL)
{
}

StringHashthing::StringHashthing (const char * pszId)
    : StringHashtable<Thing> (true,     // bCaseSensitiveKeys
                                  true,     // bCloneKeys
                                  true,     // bDeleteKeys
                                  false),   // bDeleteValues
      Thing (pszId)
{
}

StringHashthing::StringHashthing (Graph *pGraph, const char * pszId)
    : StringHashtable<Thing> (true,true, true, false), Thing (pGraph, pszId)
{
}

StringHashthing::~StringHashthing ()
{
}

void StringHashthing::setGraph (Graph *pGraph)
{
    Thing::setGraph (pGraph);
}

Graph * StringHashthing::getGraph (void)
{
    return Thing::getGraph();
}

const char * StringHashthing::getId (void)
{
    return Thing::getId();
}

//Thing * StringHashthing::put (const char * pszKey)
//{
//    Thing *pThing =  new StringHashthing (_pGraph, pszKey);
//    Thing *pOld = _pGraph->remove (pszKey);
//
//    // Add to the Graph
//    _pGraph->put (pszKey, pThing);
//
//    // Add to the Thing (both ways if the graph is not direct)
//    StringHashtable<Thing>::put(pszKey, pThing);
//    if (!_pGraph->isDirect()) {
//        pThing->put (_pszId, this);
//    }
//
//    return pOld;
//}

Thing * StringHashthing::put (const char * pszKey, Thing *pThing)
{
    Thing *pOld = StringHashtable<Thing>::put(pszKey, pThing);
    if ((_pGraph) && (!_pGraph->isDirect()) && (!pThing->contains (_pszId))) {
        pThing->put (_pszId, this);
    }
    return pOld;
}

bool StringHashthing::contains (const char * pszKey)
{
    return StringHashtable<Thing>::containsKey (pszKey);
}

Thing * StringHashthing::getThing (const char * pszKey)
{
    return StringHashtable<Thing>::get (pszKey);
}

Thing * StringHashthing::getParent (void)
{
    // TODO: implement this
    return NULL;
}

//Iterator * StringHashthing::getParents (void)
//{
//    for (StringHashtable<Thing>::Iterator i = _pGraph->getAllElements(); !i.end(); i.nextElement()) {
//        Thing * pThing = i.getValue();
//        if (pThing->get(_pszId)) {
//            // TODO: add to the iterator
//        }
//    }
//    return NULL;
//}

bool StringHashthing::isReachable (const char * pszKey)
{
    bool pRet = false;
    PtrLList<String> *pList = NULL;
    pRet = contains (pszKey);
    if (!pRet) {
        if (!_pGraph->isDirect()) {
            PtrLList<String> *pList = new PtrLList<String>();
            pRet = isReachable (pList, pszKey);
        }
        else {
            for (StringHashtable<Thing>::Iterator iterator = StringHashtable<Thing>::getAllElements(); !iterator.end() && !pRet; iterator.nextElement()) {
                pRet = iterator.getValue()->isReachable (pszKey);
            }
        }
    }
    delete pList;
    return pRet;
}

bool StringHashthing::isReachable (PtrLList<String> *pList, const char * pszKey)
{
    bool pRet = false;
    pRet = contains (pszKey);
    pList->append (&_pszId);
    if (!pRet) {
        for (StringHashtable<Thing>::Iterator iterator = StringHashtable<Thing>::getAllElements(); !iterator.end() && !pRet; iterator.nextElement()) {
            String key (iterator.getKey());
            if (!pList->search (&key)) {
                pRet = iterator.getValue()->isReachable (pList, pszKey);
            }
        }
    }
    return pRet;
}

Thing * StringHashthing::remove (const char * pszKey)
{
    Thing *pRemovedThing = StringHashtable<Thing>::remove (pszKey);
    if ((pRemovedThing) && (_pGraph) && (!_pGraph->isDirect()) && (pRemovedThing->contains (_pszId))) {
        pRemovedThing->remove (_pszId);
    }
    return pRemovedThing;
}

PtrLList<Thing> * StringHashthing::list (void)
{
    // TODO: implement this
    return NULL;
}

StringHashtable<Thing>::Iterator StringHashthing::iterator (void)
{
    return StringHashtable<Thing>::getAllElements();
}

Thing * StringHashthing::clone (void)
{
    return (new StringHashthing (NULL, strDup(_pszId)));
}

Thing * StringHashthing::deepClone (void)
{
    // TODO: implement this
    return  clone();
}

double StringHashthing::getWeight()
{
    // TODO: implement it
    double fRet = 0;
    return fRet;
}

int StringHashthing::read (Reader *pReader, uint32 ui32MaxSize)
{
    // TODO: implement this
    return 0;
}

int StringHashthing::write (Writer *pWriter, uint32 ui32MaxSize)
{
    // TODO: implement this
    // Write the number of Vertex
//    uint16 ui16 = getVertexCount ();
//    pWriter->write16(&ui16);
//
//    for (StringHashtable<Thing>::Iterator iterator = thingIterator(void); !iterator.end(); iterator.nextElement()) {
//
//    }

    return 0;
}
