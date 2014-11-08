/*
 * MSPAlgorithm.cpp
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

#include "MSPAlgorithm.h"

#include "StringHashtable.h"

#include "Graph.h"
#include "PtrLList.h"
#include "Thing.h"

#if !defined (ANDROID)
	#include <algorithm>
#endif

#include <string.h>

using namespace NOMADSUtil;

//==============================================================================
// Edge
//==============================================================================
struct Edge {
    public:
        Edge (const char * vertex1, const char * vertex2, double weight);
        virtual ~Edge();

        bool operator > (Edge &edge);
        bool operator < (Edge &edge);
        bool operator == (Edge &edge);

        const char * _vertex1;
        const char * _vertex2;
        double _weight;
};

Edge::Edge (const char * vertex1, const char * vertex2, double weight)
{
    _vertex1 = vertex1;
    _vertex2 = vertex2;
    _weight = weight;
}

Edge::~Edge ()
{
}

bool Edge::operator > (Edge &edge)
{
    return (_weight > edge._weight);
}

bool Edge::operator < (Edge &edge)
{
    return (_weight < edge._weight);
}

bool Edge::operator == (Edge &edge)
{
    return ((strcmp (_vertex1, edge._vertex1) == 0) && (strcmp (_vertex2, edge._vertex2) == 0));
}

//==============================================================================

bool isSpanningTree (Graph *pInitialGraph, Graph *pGraph);
void addLightEdges (PtrLList<Edge> &lightEdges, const char * pDepartureVertexId, double dDeparturVertexWeight, StringHashtable<Thing>::Iterator &iterator, Graph &mpsGraph);
void removeLightEdges (PtrLList<Edge> &lightEdges, Thing *pAddedVertex);

void MSPAlgorithm::MSPPrimAlgorithm (Graph *pInitialGraph, Graph &mpsGraph, Thing * pStartingVertex)
{
    Thing * pTDeparture, * pTArrival;
    Edge * pLightEdge;

    // add the first node to the MPS Graph
    pTDeparture = pStartingVertex->clone();
    mpsGraph.put (pTDeparture->getId(), pTDeparture);
    pTDeparture->setGraph(&mpsGraph);

    // add the addLightEdges
    PtrLList<Edge> lightEdges;
    StringHashtable<Thing>::Iterator neighbors = pStartingVertex->iterator();
    addLightEdges (lightEdges, pStartingVertex->getId(), pStartingVertex->getWeight(), neighbors, mpsGraph);

    do {
        // Select the light edge with minimum weight
       pLightEdge = lightEdges.getFirst ();
       if (pLightEdge == NULL) {
           break;
       }

       // Get the departure and the arrival vertexes
       pTDeparture = mpsGraph.get (pLightEdge->_vertex1);
       pTArrival = pInitialGraph->get (pLightEdge->_vertex2);

       // Add light edges departing from the new added node.
       neighbors = pTArrival->iterator();
       addLightEdges (lightEdges, pTArrival->getId(), pTArrival->getWeight(), neighbors, mpsGraph);

       pTArrival = pTArrival->clone();
       pTArrival->setGraph (&mpsGraph);

       // Add the Arrival node to the MST Graph
       mpsGraph.put (pTArrival->getId(), pTArrival);
       // Add the edge
       pTDeparture->put (pTArrival->getId(), pTArrival);

       // A new vertex has been added, thus light edges may not be light anymore.
       // Seek and remove them.
       removeLightEdges (lightEdges, pTArrival);
    } while (!isSpanningTree(pInitialGraph, &mpsGraph));
}

bool isSpanningTree (Graph *pInitialGraph, Graph *pGraph)
{
    // Every node in the iterator must be contained in the passed Graph
    for (StringHashtable<Thing>::Iterator iterator = pInitialGraph->thingIterator(); !iterator.end(); iterator.nextElement()) {
        if (!pGraph->contains(iterator.getKey())) {
            return false;
        }
    }
    return true;
}

void addLightEdges (PtrLList<Edge> &lightEdges, const char * pszDepartureVertexId, double dDeparturVertexWeight, StringHashtable<Thing>::Iterator &iterator, Graph &mpsGraph)
{
    if (pszDepartureVertexId == NULL) {
        return;
    }
    double dArrivalVertexWeight;
    for (const char * pszArrivalVertex = NULL; !iterator.end(); iterator.nextElement()) {
        // check whether the edge is in the subset of the MPS Graph
        pszArrivalVertex = iterator.getKey();
        if ((pszArrivalVertex != NULL) && (!mpsGraph.contains(pszArrivalVertex))) {
            // it's a light edge - add it
            dArrivalVertexWeight = iterator.getValue()->getWeight();

            //-----------------------------------------
            lightEdges.insert (new Edge (pszDepartureVertexId, pszArrivalVertex, (dDeparturVertexWeight > dArrivalVertexWeight ? dDeparturVertexWeight : dArrivalVertexWeight)));
        }
    }
}

void removeLightEdges (PtrLList<Edge> &lightEdges, Thing *pAddedVertex)
{
    const char * pszAddedVertex = pAddedVertex->getId();
    Edge *pTmpNext;
    for (Edge *pEdge = lightEdges.getFirst(); pEdge != NULL;) {
        if (strcmp (pEdge->_vertex2, pszAddedVertex) == 0) {
            // TODO: check if remove actually deletes the object
            pTmpNext = lightEdges.getNext();

            lightEdges.remove (pEdge);

            delete pEdge;
            pEdge = pTmpNext;
            continue;
        }
        pEdge = lightEdges.getNext();
    }
}
