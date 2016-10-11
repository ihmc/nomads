/*
 * HTGraph.h
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
 * Author: Mirko Gilioli (mgilioli@i@ihmc.us)
 * Created on June 13, 2012, 11:00 AM
 */

#ifndef INCL_GRAPH_H
#define INCL_GRAPH_H

#include <stdlib.h>
#include <stdio.h>

#include "DArray2.h"
#include "NLFLib.h"
#include "PtrLList.h"
#include "PtrQueue.h"
#include "Reader.h"
#include "StrClass.h"
#include "StringHashtable.h"
#include "Writer.h"

namespace NOMADSUtil
{        
    template <class C> struct Edge
    {        
        /*
         * The Edge is composed by three elements:
         * 1) pszDestKey that keeps track of the Destination Vertex Key
         * 2) pszEdgeLabel that keeps track of the label of the Edge.
         * 3) pEdgeCost that keeps the reference of the cost of the Edge
         * 
         * Edges are identified uniquely by the pszDestKey and pszEdgeLabel.
         * This means that a Vertex A can NOT have two Edges with the same label
         * pointing toward the same destination.
         * 
         * Edge labels can have several application. It can be used to distinguish
         * on edge from another by its network interface. Nodes in the network can
         * be connected in different ways using different network interfaces.
         */

        /*
         * It creates an empty Edge
         */
        Edge (void);

        /* 
         * It does make a copy of the Destination Vertex Key, the Edge Label and the Cost.
         */
        Edge (const char *pszDstVertexKey, const char *pszEdgeLabel, C *pEdgeCost);

        /* 
         * It does NOT make a copy of the Destination Vertex Key and the Edge Label neither.
         * The pointer to the Edge Cost is set to NULL.
         * This constructor is called when you need to compare Edges.   
         */
        Edge (const char *pszDstVertexKey, const char *pszEdgeLabel);

        /* Creates a new Edge with a copy of the Destination Vertex Key */
        Edge (const char *pszDstVertexKey);

        /* It does FREE on Destination Vertex Key, Edge Label */
        virtual ~Edge (void);

        /* 
         * Compare two edges. If the Destination Vertex Key AND the Edge Labels are equal 
         * it returns 1. 0 otherwise. 
         */
        int operator == (const Edge &rhsEdge);

        static const char *DEFAULT_EDGE_LABEL;

        C *_pEdgeElement;
        bool _bReferenceKept;
        char *_pszDstVertexKey;
        char *_pszEdgeLabel;
    };

    template <class T, class C> struct Vertex
    {
        Vertex (void);
        virtual ~Vertex (void);

        PtrLList<Edge<C> > _edgeList;
        T *_pElement;
    };

    template <class T, class C> class HTGraph
    {
        public:
            HTGraph (bool bDeleteElements=true, bool bDirected=true);
            virtual ~HTGraph (void);

            /*
             * Adds a new Element T in the Graph. 
             * NOTE : It does NOT make a copy of the element, but it just keeps the reference.
             * Returns 0 if successful, -1 if the Graph contains the Vertex T already.
             */
            int addVertex (const char *pszVertexKey, T *pElement);

            /*
             * Remove a Vertex T from the Graph and its directed Edges and/or undirected Edges. 
             * Return 0 if the operation is successful, -1 if the Vertex is not found in the Graph.
             */
            int removeVertex (const char *pszVertexKey);

            /*
             * Adds an Edge between Source and Destination.
             * Returns 0 if successful, negative value otherwise
             */
            int addEdge (const char *pszSourceVertexKey, const char *pszDestVertexKey,
                         const char *pszEdgeLabel, C *pEdgeElement);

            int removeAllEdgesFromVertex (const char *pszVertexKey);

            /*
             * Removes an Edge from the Graph, given the Source Vertex Key, Destination Vertex key
             * and the EdgeColor.
             * Returns 0 if successful, a negative value otherwise
             */
            int removeEdge (const char *pszSourceVertexKey, const char *pszDestVertexKey,
                            const char *pszEdgeLabel);

            /* 
             * Removes one or more directed/undirected Edges pointing toward a Destination Vertex.
             * Returns 0 if the removal was successful, a negative number otherwise. 
             */
            int removeEdgeByDestination (const char *pszSourceVertexKey, const char *pszDestVertexKey);

            /*
             * Removes one or more directed/undirected Edges with a certain EdgeColor.
             * Returns 0 if the removal was successful, a negative number otherwise. 
             */
            int removeEdgeByLabel (const char *pszSourceVertexKey, const char *pszEdgeLabel);

            /*
             * Returns the number of Edges in the Graph
             */
            uint16 getEdgeCount (void);

            /*
             * Returns a pointer to Element T if the key is found on the Hashtable.
             * Otherwise, it returns NULL
             */
            T * getVertex (const char *pszVertexKey);

            /*
             * Returns a pointer to the Vertex Object if it is contained in the Graph.
             * Returns NULL otherwise
             */
            Vertex<T,C> * getVertexObject (const char *pszVertexKey);
                    
            /*
             * Returns a list containing all the Graph Vertex keys
             * Returns NULL if the Graph is empty
             *
             * NOTE: the PtrLList, but not its elements, have to be deallocated
             *       by the caller
             */
            PtrLList<const char> * getVertexKeys (void);

            /*
             * Returns a pointer linked list of Edges that belongs to a Vertex with pszKey as key.
             * If no Vertex has been found the method returns NULL
             */
            PtrLList<Edge<C> > * getEdgeList (const char *pszVertexKey);

            /*
             * Returns a list of Edges between two Vertex in the pEdgeList.
             * Return 0 if at least one Edge is found, negative value otherwise.
             */
            int getEdgeList (const char *pszSourceVertexKey, const char *pszDestVertexKey,
                             PtrLList<Edge<C> > *pEdgeRetList);

            /*
             * Returns the pointer to the Edge from Source to Destination with the same EdgeColor
             * if it exists, NULL otherwise.
             */
            Edge<C> * getEdge (const char *pszSourceVertexKey, const char *pszDestVertexKey,
                               const char *pszEdgeLabel);

            /*
             * Returns the number of Vertex in the Graph
             */
            uint16 getVertexCount (void);

            /*
             * Returns a list containing the neighbors vertex keys of a certain Vertex.
             * The list is returned in pNeighborsList. So it needs to be instantiated.
             * Return the number of Neighbors if successful, a negative value otherwise.
             */
            int getNeighborsList (const char *pszVertexKey, PtrLList<NOMADSUtil::String> *pNeighborsList);

            /*
             * Returns the minimum number of hops between two nodes. 
             * It is based on Dijkstra algorithm.
             * Note : The class C needs to implement the following methods:
             *        1) C ()
             *        2) void setCost (float fCost);
             *        3) float getCost ();  
             * Returns a negative value if path has not been found.
             */
            int getShortestPath (const char *pszSrcVertexKey, const char *pszDestVertexKey,
                                 PtrLList<const char> *pPath, bool bUseCost=true);

            /*
             * Checks if there is an Edge from Source to Destination with the same EdgeColor.
             * Returns true if it is found. False otherwise.
             */
            bool hasEdge (const char *pszSourceVertexKey, const char *pszDestVertexKey,
                          const char *pszEdgeLabel);

            /*
             * Returns true if a Vertex has the given neighbor. False otherwise
             */
            bool hasNeighbor (const char* pszSourceVertexKey, const char *pszNeighborKey);

            /* 
             * Checks whether the Graph has the Vertex
             * Returns true if it is found. False otherwise  
             */
            bool hasVertex (const char *pszVertexKey);

            /*
             * Compares two Vertexes. In order to used this method the template T need 
             * to implement the '!= operator'. It MUST returns true or false.
             * Return 0 if they are equivalent. A negative value otherwise. 
             */
            int compareVertexes (Vertex<T,C> *pSourceVertex, Vertex<T,C> *pDestVertex);

            /*
             * Given the Vertex key, the method deletes the vertexes and Edges that are 
             * over the hop number given.
             * Returns the number of Vertexes deleted. Negative value in case of errors
             * Note : The class C needs to implement the following methods:
             *        1) C ()
             *        2) void setCost (float fCost);
             *        3) float getCost ();  
             */
            int deleteElements (const char *pszVertexKey, uint16 ui16HopNum);

            /*
             * Returns if the Graph is directed or not
             */
            bool isDirected (void);

            /*
             * Checks whether the Graph deletes the Elements or not
             */
            bool deletesElements (void);

            /*
             * Displays all the graph
             */
            void display (FILE *pOutput);

            /*
             * Displays all the Edges of a Key. Returns a 0 if successful, negative value if not
             */
            int displayEdges (FILE *pOutput, const char *pszVertexKey);

            /*
             * Deletes all the Graph partitions that don't contain the given Vertex.
             * Return 0 if successful, negative in case of error.
             */
            int deletePartitions (const char *pszVertexKey);

            /*
             * Removes all the Vertexes and Edges from the Graph.
             */
            void empty (void);

            /*
             * Returns true if the Graph has no Vertexes. False Otherwise
             */
            bool isEmpty (void);

            /*
             * Given a Vertex key, the Vertex and its neighbors in the Reader, this method
             * updates the graph. Returns the key of the updated vertex.
             * Returns NULL in case of error or no update.
             */
            const char * updateNeighborhood (Reader *pReader, uint32 *pui32BytesRead);            

            /*
             * Given a set of Vertex keys, Vertexes and their neighbors in the Reader,
             * this method updates the graph. Returns keys of the updated Vertexes.
             * Returns NULL in case of error.
             */
            PtrLList<const char> * updateGraph (Reader *pReader, uint32 *pui32BytesRead);

            /*
             * This methods writes the number of Vertex written (1), key of the Vertex, 
             * the Vertex its self and all its neighbors (Neighbor key and its attributes).
             * Returns 0 if successful, negative value otherwise.
             * It also returns the bytes written in the variable ui32BytesWritten 
             */
            int writeNeighborhood (Writer *pWriter, uint32 *pui32BytesWritten, 
                                   const char *pszVertexKey);

            /*
             * It writes all the graph in this form:
             * Vertex key, Vertex, number of Vertex Edges, Neighbor Vertex keys, Neighbor Vertexes
             * return 0 if successful, negative in case of error.
             */
            int writeGraph (Writer *pWriter, uint32 *pui32BytesWritten);

            /*
             * Writes the Graph
             * Returns 0 if successful, negative otherwise
             */
            int writeCompleteGraph (Writer *pWriter, uint32 *pui32BytesWritten);

            /*
             * Reads the Graph
             * Return 0 if successful, negative otherwise
             */
            int readCompleteGraph (Reader *pReader, uint32 *pui32BytesRead);

            /*
             * Performs the deep first traverse of the graph.
             * It returns all the Vertex reachable from a given Vertex in the Hashtable. 
             */
            void traverse (Vertex<T,C> *pVertex, StringHashtable<bool> *pVisited);

            /*
             * Performs the deep first traverse of the Graph for certain hops deep.
             * Returns the Vertexes reachable until a certain deep has been reached. 
             */
            void traverse (Vertex<T,C> *pVertex, StringHashtable<bool> *pVisited, uint16 ui16Hops);

        private:        
            /*
             * Given a Vertex key, the Vertex and its neighbors in the Reader, this method
             * updates the graph. Returns the key of the updated vertex.
             * Returns NULL in case of error or no update.
             * It is used by the public methods updateNeighborhood and updateGraph
             */
            const char * updatePrivateNeighborhood (Reader *pReader, uint32 *pui32BytesRead);

            /*
             * This methods writes the key of the Vertex, the Vertex its self and
             * all its neighbors (Neighbor key and its attributes).
             * Returns 0 if successful, negative value otherwise.
             * It also returns the bytes written in the variable ui32BytesWritten.
             * It is used by the public methods writeNeighborhood and writeGraph. 
             */
            int writePrivateNeighborhood (Writer *pWriter, uint32 *pui32BytesWritten, const char *pszVertexKey);

            /*
             * Returns a new shortest path tree of the Graph given a certain Vertex Key as tree root.
             * It DOES create a new HTGraph. It is based on Dijkstra's algorithm.
             * Returns NULL in case of error.
             */
            HTGraph<T,C> * getShortestPathTree (const char *pszRootKey, bool bUseCost);

            /*
             * This method is used in the getShortestPathTree() and getShortestPath().
             * Returns the closest vertex key given a candidate queue of Vertex and set of distances.
             */
            const char * getClosestVertex (StringHashtable<float> *pDistances, PtrQueue<String> *pQueue);

            bool _bDeleteElements;
            bool _bDirected;
            uint16 _ui16VertexNum;
            uint16 _ui16EdgeNum;
            StringHashtable<Vertex<T,C> > _vertexHashtable;
    };

    template <class C> Edge<C>::Edge()
    {
        _pszDstVertexKey = NULL;
        _pszEdgeLabel = NULL;
        _pEdgeElement = NULL;
        _bReferenceKept = false;
    }

    template <class C> Edge<C>::Edge (const char *pszDstVertexKey, const char *pszEdgeLabel, C *pEdgeCost)
    {
        _pszDstVertexKey = strDup (pszDstVertexKey);
        _pszEdgeLabel = strDup (pszEdgeLabel);
        _pEdgeElement = pEdgeCost;
        _bReferenceKept = false;
    }

    template <class C> Edge<C>::Edge (const char *pszDestVertexKey, const char *pszEdgeLabel) 
    {
        _pszDstVertexKey = (char *) pszDestVertexKey;
        _pszEdgeLabel = (char *) pszEdgeLabel;
        _pEdgeElement = NULL;
        _bReferenceKept = true;
    }
    
    template <class C> Edge<C>::Edge (const char *pszDstVertexKey)
    {
        /* Copy the Destination Key */
        _pszDstVertexKey = strDup (pszDstVertexKey);        
        _pszEdgeLabel = strDup (DEFAULT_EDGE_LABEL);
        _pEdgeElement = NULL;
        _bReferenceKept = false;        
    }

    template <class C> Edge<C>::~Edge() 
    {
        if (_pszEdgeLabel && !_bReferenceKept) {
            free (_pszEdgeLabel);
            _pszEdgeLabel = NULL;
        }

        if (_pszDstVertexKey && !_bReferenceKept) {
            free (_pszDstVertexKey);
            _pszDstVertexKey = NULL;
        }        
    }

    template <class C> int Edge<C>::operator == (const Edge &rhsEdge) 
    {
        if (strcmp (_pszDstVertexKey, rhsEdge._pszDstVertexKey) == 0 &&
            strcmp (_pszEdgeLabel, rhsEdge._pszEdgeLabel) == 0) {

            return 1;
        }
        return 0;
    }

    template <class C> const char * Edge<C>::DEFAULT_EDGE_LABEL = "default_link";

    template <class T, class C> Vertex<T,C>::Vertex()
    {
        _pElement = NULL;
    }

    template <class T, class C> Vertex<T,C>::~Vertex()
    {
    }

    template <class T, class C> HTGraph<T,C>::HTGraph (bool bDeleteElements, bool bDirected)
    {
        _bDeleteElements = bDeleteElements;
        _bDirected = bDirected;
        _ui16VertexNum = 0;
        _ui16EdgeNum = 0;
        _vertexHashtable.configure (true, true, true, bDeleteElements);
    }

    template <class T, class C> HTGraph<T,C>::~HTGraph()
    {
    }

    template <class T, class C> int HTGraph <T,C>::addVertex (const char *pszVertexKey, T *pElement)
    {
        if (pszVertexKey == NULL || pElement == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertexToAdd = _vertexHashtable.get (pszVertexKey);
        if (pVertexToAdd != NULL) {
            // Vertex already exists - no need to add it again
            return -2;
        }

        pVertexToAdd = new Vertex<T,C>();
        if (pVertexToAdd == NULL) {
            return -3;
        }
        
        // Add the Element T to the Vertex
        pVertexToAdd->_pElement = pElement;
        _vertexHashtable.put (pszVertexKey, pVertexToAdd);
        _ui16VertexNum++;
        
        return 0;
    }

    template <class T, class C> int HTGraph<T,C>::removeVertex (const char *pszVertexKey)
    {
        if (pszVertexKey == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertexToDelete = _vertexHashtable.remove (pszVertexKey);
        if (pVertexToDelete == NULL) {
            // Vertex not found - can't be removed
            return -1;
        }

        // Delete the EdgeList of the Vertex to delete
        _ui16EdgeNum = _ui16EdgeNum - pVertexToDelete->_edgeList.getCount();
        Edge<C> *pCurr, *pNext, *pDel;
        pNext = pVertexToDelete->_edgeList.getFirst();
        while ((pCurr = pNext) != NULL) {
            pNext = pVertexToDelete->_edgeList.getNext();
            pDel = pVertexToDelete->_edgeList.remove (pCurr);
            delete pDel;
        }

        // Delete the Vertex itself
        if (_bDeleteElements) {
            delete pVertexToDelete->_pElement;
        }
        delete pVertexToDelete;
        pVertexToDelete = NULL;
        _ui16VertexNum--;

        Vertex<T,C> *pCurrVertex;
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            pCurrVertex = i.getValue();
            for (Edge<C> *pCurrEdge = pCurrVertex->_edgeList.getFirst(); pCurrEdge != NULL;
                 pCurrEdge = pCurrVertex->_edgeList.getNext()) {

                if (strcmp (pCurrEdge->_pszDstVertexKey, pszVertexKey) == 0) {
                    Edge<C> edgeToDel (pszVertexKey, pCurrEdge->_pszEdgeLabel);
                    pDel = pCurrVertex->_edgeList.remove (&edgeToDel);
                    _ui16EdgeNum--;
                    delete pDel;
                }
            }
        }

        return 0;
    }

    template <class T, class C> int HTGraph <T,C>::addEdge (const char *pszSourceVertexKey,
                                                            const char *pszDestVertexKey,
                                                            const char *pszEdgeLabel, C *pEdgeElement)
    {
        if (pszSourceVertexKey == NULL || pszDestVertexKey == NULL ||
            pszEdgeLabel == NULL || pEdgeElement == NULL) {
            return -1;
        }

        // Ensure that both Source and Destination Vertexes exist before adding an Edge
        if (!_vertexHashtable.containsKey (pszSourceVertexKey) ||
            !_vertexHashtable.containsKey (pszDestVertexKey)) {
            
            return -1;
        }

        Vertex<T,C> *pCurrentVertex;
        Edge<C> *pNewEdge;
        if (_bDirected) {
            // Check if there is an Edge from the Source to Destination Vertex
            if (!hasEdge (pszSourceVertexKey, pszDestVertexKey, pszEdgeLabel)) {
                pCurrentVertex = _vertexHashtable.get (pszSourceVertexKey);
                // Create and add a new Edge. Check whether the Edge has been allocated
                pNewEdge = new Edge<C> (pszDestVertexKey, pszEdgeLabel, pEdgeElement);                
                if (pNewEdge == NULL) {
                    return -1;
                }

                pCurrentVertex->_edgeList.append (pNewEdge);
                _ui16EdgeNum++;
                return 0;
            }
            return -2;
        }
        else {
            // if the Graph is not directed and there is no edge between Destination
            // and Source Vertex, add two Edges
            if (!hasEdge (pszSourceVertexKey, pszDestVertexKey, pszEdgeLabel) && 
                !hasEdge (pszDestVertexKey, pszSourceVertexKey, pszEdgeLabel)) {
                
                pCurrentVertex = _vertexHashtable.get (pszSourceVertexKey);
                pNewEdge = new Edge<C> (pszDestVertexKey, pszEdgeLabel, pEdgeElement);
                if (pNewEdge == NULL) {
                    return -1;
                }
                
                pCurrentVertex->_edgeList.append (pNewEdge);
                _ui16EdgeNum++;
 
                // Check if the Source Node and Destination Node are the same. 
                // If so there's no need to add another Edge.
                if (strcmp (pszSourceVertexKey, pszDestVertexKey) != 0) {
                    pCurrentVertex = _vertexHashtable.get (pszDestVertexKey);
                    pNewEdge = new Edge<C> (pszSourceVertexKey, pszEdgeLabel, pEdgeElement);
                    if (pNewEdge == NULL) {
                        return -1;
                    }
                
                    pCurrentVertex->_edgeList.append (pNewEdge);
                    _ui16EdgeNum++;
                }
                
                return 0;
            }
            return -2;
        }
    }

    template <class T, class C> int HTGraph<T,C>::removeAllEdgesFromVertex (const char *pszVertexKey)
    {
        if (pszVertexKey == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszVertexKey);
        if (pVertex == NULL) {
            // Edge not found - can't be removed
            return -2;
        }

        Edge<C> *pNext = pVertex->_edgeList.getFirst();
        for (Edge<C> *pCurr; (pCurr = pNext) != NULL;) {
            pNext = pVertex->_edgeList.getNext();
            delete pVertex->_edgeList.remove (pCurr);
        }

        return 0;
    }

    template <class T, class C> int HTGraph<T,C>::removeEdge (const char *pszSourceVertexKey,
                                                              const char *pszDestVertexKey,
                                                              const char *pszEdgeLabel)
    {
        if (pszSourceVertexKey == NULL || pszDestVertexKey == NULL || pszEdgeLabel == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszSourceVertexKey);
        if (pVertex == NULL) {
            // Edge not found - can't be removed
            return -1;
        }

        /// Search and remove the Edges from the Vertex edge list
        bool bFoundDirected = false;
        bool bFoundUndirected = false;
        Edge<C> *pEdgeToDelete = NULL;
        Edge<C> edgeToSearch (pszDestVertexKey, pszEdgeLabel);
        pEdgeToDelete = pVertex->_edgeList.remove (&edgeToSearch);
        if (pEdgeToDelete != NULL) {
            delete pEdgeToDelete;
            pEdgeToDelete = NULL;
            _ui16EdgeNum--;
            bFoundDirected = true;            
        }

        // If the Graph is not Directed search and delete the inverse Edges
        if (!_bDirected) {
            pVertex = _vertexHashtable.get (pszDestVertexKey);
            if (pVertex == NULL) {
                return -2;
            }
            Edge<C> edgeToSearch (pszSourceVertexKey, pszEdgeLabel);
            pEdgeToDelete = pVertex->_edgeList.remove (&edgeToSearch);
            if (pEdgeToDelete != NULL) {
                delete pEdgeToDelete;
                _ui16EdgeNum--;
                bFoundUndirected = true;
            }

            // Return 0 if at least one Undirected Edge is successfully deleted. -2 Otherwise
            if (bFoundDirected && bFoundUndirected) {
                    return 0;
            }
            return -2;
        }
        else {
            // Return 0 if at least one Directed Edge is successfully deleted. -1 Otherwise
            if (bFoundDirected) {
                return 0;
            }
            return -1;
        }        
    }

    template <class T, class C> int HTGraph<T,C>::removeEdgeByDestination (const char *pszSourceVertexKey, const char *pszDestVertexKey)
    {
        if (pszSourceVertexKey == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszSourceVertexKey);
        if (pVertex == NULL) {
            // Source Vertex not found - there can't be an edge from it, therefore edge can't be removed
            return -2;
        }

        int rc = -3;
        for (Edge<C> *pCurrEdge = pVertex->_edgeList.getFirst(); pCurrEdge != NULL;
             pCurrEdge = pVertex->_edgeList.getNext()) {

            if (strcmp (pszDestVertexKey, pCurrEdge->_pszDstVertexKey) == 0) {
                // Copy the current label to pass to removeEdge. Do NOT pass the pszEdgeLabel as parameter
                String sCurrLabel (pCurrEdge->_pszEdgeLabel);
                if ((rc = removeEdge (pszSourceVertexKey, pszDestVertexKey, sCurrLabel.c_str())) < 0) {
                    return rc;
                }
            }
        }

        return rc;
    }
    
    template <class T, class C> int HTGraph<T,C>::removeEdgeByLabel (const char *pszSourceVertexKey, const char *pszEdgeLabel)
    {
        if (pszSourceVertexKey == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszSourceVertexKey);
        if (pVertex == NULL) {
            // Source Vertex not found - there can't be an edge from it, therefore edge can't be removed
            return -2;
        }

        int rc = -3;
        for (Edge<C> *pCurrentEdge = pVertex->_edgeList.getFirst(); pCurrentEdge != NULL;
             pCurrentEdge = pVertex->_edgeList.getNext()) {
            
            if (strcmp (pCurrentEdge->_pszEdgeLabel, pszEdgeLabel) == 0) {
                // Copy the current label to pass to removeEdge. Do NOT pass the pszEdgeLabel as parameter
                String sCurrDest (pCurrentEdge->_pszDstVertexKey);                
                if ((rc = removeEdge (pszSourceVertexKey, sCurrDest.c_str(), pszEdgeLabel)) < 0) {
                    return rc;
                }
            }
        }

        return rc;
    }    

    template <class T, class C> uint16 HTGraph<T,C>::getEdgeCount()
    {
        return _ui16EdgeNum;
    }
    
    template <class T, class C> T * HTGraph<T,C>::getVertex (const char *pszKey)
    {
        if (pszKey == NULL) {
            return NULL;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszKey);
        if (pVertex == NULL) {
            return NULL;
        }
        
        return pVertex->_pElement;
    }
    
    template <class T, class C> Vertex<T,C> * HTGraph<T,C>::getVertexObject (const char *pszVertexKey)
    {
        return _vertexHashtable.get (pszVertexKey);
    }
    
    template <class T, class C> PtrLList<const char> * HTGraph<T,C>::getVertexKeys()
    {
        if (getVertexCount() == 0) {
            // Graph is empty
            return NULL;
        }

        PtrLList<const char> *pRet = new PtrLList<const char>();
        if (pRet == NULL) {
            return NULL;
        }
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            pRet->prepend (i.getKey());
        }

        return pRet;
    }

    template <class T, class C> PtrLList<Edge<C> > * HTGraph<T,C>::getEdgeList (const char *pszVertexKey)
    {
        if (pszVertexKey == NULL) {
            return NULL;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszVertexKey);
        if (pVertex == NULL) {
            return NULL;
        }

        PtrLList<Edge<C> > *pEdgeListCpy = new PtrLList<Edge<C> >();
        if (pEdgeListCpy == NULL) {
            return NULL;
        }

        Edge<C> *pEdge = pVertex->_edgeList.getFirst();
        while (pEdge != NULL) {
            pEdgeListCpy->prepend (pEdge);
            pEdge = pVertex->_edgeList.getNext();
        }

        return pEdgeListCpy;
    }
    
    template <class T, class C> int HTGraph<T,C>::getEdgeList (const char *pszSourceVertexKey, const char *pszDestVertexKey, PtrLList<Edge<C> > *pEdgeRetList)
    {
        if (pszSourceVertexKey == NULL || pszDestVertexKey == NULL || pEdgeRetList == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszSourceVertexKey);
        if (pVertex == NULL) {
            return -1;
        }

        for (Edge<C> *pCurrEdge = pVertex->_edgeList.getFirst(); pCurrEdge != NULL;
             pCurrEdge = pVertex->_edgeList.getNext()) {

            if (strcmp (pCurrEdge->_pszDstVertexKey, pszDestVertexKey) == 0) {
                pEdgeRetList->append (pCurrEdge);
            }
        }

        if (pEdgeRetList->isEmpty()) {
            return -1;
        }

        return 0;
    }

    template <class T, class C> Edge<C> * HTGraph<T,C>::getEdge (const char *pszSourceVertexKey, const char *pszDestVertexKey, const char *pszEdgeLabel)
    {
        if (pszSourceVertexKey == NULL || pszDestVertexKey == NULL || pszEdgeLabel == NULL) {
            return NULL;
        }

        Vertex<T,C> *pSrcVertex = _vertexHashtable.get (pszSourceVertexKey);
        if (pSrcVertex == NULL) {
            // Source Vertex not found - there can't be an edge from it
            return NULL;
        }

        // Check all the Edges of the Vertex. 
        for (Edge<C> *pCurrEdge = pSrcVertex->_edgeList.getFirst(); pCurrEdge != NULL;
             pCurrEdge = pSrcVertex->_edgeList.getNext()) {

            if (strcmp (pszDestVertexKey, pCurrEdge->_pszDstVertexKey) == 0 &&
                strcmp (pszEdgeLabel, pCurrEdge->_pszEdgeLabel) == 0) {

                return pCurrEdge;
            }
        }

        return NULL;
    }

    template <class T, class C> uint16 HTGraph<T,C>::getVertexCount()
    {
        return _ui16VertexNum;
    }
    
    template <class T, class C> int HTGraph<T,C>::getNeighborsList (const char *pszVertexKey, PtrLList<NOMADSUtil::String> *pNeighborsList)
    {
        if (pszVertexKey == NULL || pNeighborsList == NULL) {
            return -1;
        }

        PtrLList<Edge<C> > *pEdgeList = getEdgeList (pszVertexKey);
        if (pEdgeList == NULL) {
            return -1;
        }

        int iCount = 0;
        for (Edge<C> *pCurrEdge = pEdgeList->getFirst(); pCurrEdge != NULL; 
             pCurrEdge = pEdgeList->getNext()) {

            String *pKey = new String (pCurrEdge->_pszDstVertexKey);
            if (pKey != NULL && pNeighborsList->search (pKey) == NULL) {
                // Element not found, it can be added
                pNeighborsList->prepend (pKey);
                iCount++;
            }          
       }

       delete pEdgeList;
       return iCount;
    }

    template <class T, class C> int HTGraph<T,C>::getShortestPath (const char *pszSrcVertexKey, const char *pszDestVertexKey,
                                                                   PtrLList<const char> *pPath, bool bUseCost)
    {
        if (pszSrcVertexKey == NULL || pszDestVertexKey == NULL || pPath == NULL) {
            return -1;
        }

        uint16 ui16NumOfVertex = getVertexCount();
        if (ui16NumOfVertex == 0) {
            return -1;
        }

        StringHashtable<float> distances;
        float *pInitializationArray = (float *) calloc (ui16NumOfVertex, sizeof (float));
        float fInfinity = 0xFFFFFFFF; 
        StringHashtable<const char> previous;
        PtrQueue<String> candidateQueue;        

        // Initialize data structures
        const char *pszKey;
        int k=0;
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {
            
            pszKey = i.getKey();
            pInitializationArray[k] = fInfinity;
            distances.put (pszKey, (pInitializationArray + k));
            candidateQueue.enqueue (new String (pszKey));
            k++;
        }

        // Assign distance 0 to the root
        (*distances.get (pszSrcVertexKey)) = 0;

        // Until all the candidate have been evaluated
        while (!candidateQueue.isEmpty()) {
            // Get the Vertex with the minimum distance from the Candidate Queue
            const char *pszClosestVertex = getClosestVertex (&distances, &candidateQueue);
            // Search is terminate if the closest vertex is the destination
            if ((pszClosestVertex == NULL) || strcmp (pszClosestVertex, pszDestVertexKey) == 0) {
                break;
            }
            // Remove the closest vertex from the candidate queue
            String sClosest (pszClosestVertex);
            delete candidateQueue.remove (&sClosest);
             // For each neighbors of the closest vertex find the closest neighbor and add it to the MST.
             // It also creates a new Minimum Edge from the closest Vertex to its closest neighbor.
            PtrLList<Edge<C> > *pNeighborsList = getEdgeList (sClosest.c_str());
            if (pNeighborsList != NULL) {
                const char *pszPrev = sClosest.r_str();
                for (Edge<C> *pEdge = pNeighborsList->getFirst(); pEdge != NULL;
                     pEdge = pNeighborsList->getNext()) {

                    float fCost = 1.0f;
                    if (bUseCost){
                        fCost = pEdge->_pEdgeElement->getCost();
                    }
                    float fActualCost = (*distances.get (pszPrev)) + fCost;
                    if (fActualCost < (*distances.get (pEdge->_pszDstVertexKey))) {
                        (*distances.get (pEdge->_pszDstVertexKey)) = fActualCost;
                        previous.put (pEdge->_pszDstVertexKey, pszPrev);
                    }
                }
                delete pNeighborsList;
            }
        }

        // Remove the remaining elements - if any
        while (!candidateQueue.isEmpty()) {
            delete candidateQueue.dequeue();
        }

        while (previous.get (pszDestVertexKey) != NULL) {
            pPath->prepend (pszDestVertexKey);
            pszDestVertexKey = previous.get (pszDestVertexKey);
        }

        pPath->prepend (pszDestVertexKey);

        free (pInitializationArray);
        return pPath->getCount();
    }
    
    template <class T, class C> bool HTGraph<T,C>::hasEdge (const char *pszSourceVertexKey, const char *pszDestVertexKey, const char *pszEdgeLabel)
    {
        if (pszSourceVertexKey == NULL || pszDestVertexKey == NULL || pszEdgeLabel == NULL) {
            return false;
        }

        return getEdge (pszSourceVertexKey, pszDestVertexKey, pszEdgeLabel) != NULL;
    }
    
    template <class T, class C> bool HTGraph<T,C>::hasNeighbor (const char *pszSourceVertexKey, const char *pszNeighborKey)
    {
        if (pszSourceVertexKey == NULL || pszNeighborKey == NULL) {
            return false;
        }

        PtrLList<Edge<C> > *pEdgeList = getEdgeList (pszSourceVertexKey);
        if (pEdgeList != NULL) {
            for (Edge<C> *pEdge = pEdgeList->getFirst(); pEdge != NULL; pEdge = pEdgeList->getNext()) {
                if (strcmp (pEdge->_pszDstVertexKey, pszNeighborKey) == 0) {
                    delete pEdgeList;
                    return true;
                }
            }
            delete pEdgeList;
        }
        return false;
    }
    
    template <class T, class C> bool HTGraph<T,C>::hasVertex (const char *pszKey)
    {
        if (pszKey == NULL) {
            return false;
        }
        
        return _vertexHashtable.containsKey (pszKey);
    }

    template <class T, class C> int HTGraph<T,C>::compareVertexes(Vertex<T,C> *pSourceVertex, Vertex<T,C> *pDestVertex)
    {
        if ((*pSourceVertex->_pElement) != (*pDestVertex->_pElement)) {
            return -1;
        }
        uint8 ui8N = pSourceVertex->_edgeList.getCount();
        uint8 ui8M = pDestVertex->_edgeList.getCount();
        if (ui8N != ui8M) {
            return -1;
        }
        
        for (Edge<C> *pSourceEdge = pSourceVertex->_edgeList.getFirst(); pSourceEdge != NULL;
             pSourceEdge = pSourceVertex->_edgeList.getNext()) {
            
            Edge<C> edgeToSearch (pSourceEdge->_pszDstVertexKey, pSourceEdge->_pszEdgeLabel);
            if (pDestVertex->_edgeList.search (&edgeToSearch) == NULL) {
                return -1;
            }
        }
        
        return 0;
    }
    
    template <class T, class C> int HTGraph<T,C>::deleteElements (const char *pszVertexKey, uint16 ui16HopNum)
    {
        if (pszVertexKey == NULL || ui16HopNum == 0) {
            return -1;
        }
        
        // Get the minimum spanning tree without considering edge cost
        HTGraph<T,C> *pMST = getShortestPathTree (pszVertexKey, false);
        if (pMST == NULL) {
            return -1;
        }

        // Traverse the MST for certain hops
        StringHashtable<bool> reachableVertexes;
        Vertex<T,C> *pSrcVertex = pMST->getVertexObject (pszVertexKey);
        bool bVisited = true;
        reachableVertexes.put (pszVertexKey, &bVisited);
        if (pSrcVertex == NULL) {
            return -1;
        }

        pMST->traverse (pSrcVertex, &reachableVertexes, ui16HopNum);

        int iRet = 0;
        const char *pszVertexToDelete;
		DArray2<String> vertexesToRemove;
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            pszVertexToDelete = i.getKey();
            if (!reachableVertexes.containsKey (pszVertexToDelete)) {
				vertexesToRemove[iRet] = pszVertexToDelete;
                iRet++;
            }
        }

		for (unsigned int i = 0; i < vertexesToRemove.size(); i++) {
			if (vertexesToRemove.used (i)) {
				removeVertex (vertexesToRemove[i].c_str());
			}
		}

        // Destroy the MST
        PtrLList<const char> *pKeysList = pMST->getVertexKeys();
        for (const char *pszKey = pKeysList->getFirst(); pszKey != NULL; pszKey = pKeysList->getNext()) {
            Vertex<T,C> *pCurrentVertex = pMST->getVertexObject (pszKey);
            if (pCurrentVertex != NULL) {
                Edge<C> *pCurr, *pNext, *pDel;
                pNext = pCurrentVertex->_edgeList.getFirst();
                while ((pCurr = pNext) != NULL) {
                    pNext = pCurrentVertex->_edgeList.getNext();
                    pDel = pCurrentVertex->_edgeList.remove (pCurr);
                    delete pDel->_pEdgeElement;
                    delete pDel;
                }
            }
        }
        delete pKeysList;
        pMST->_vertexHashtable.removeAll();

        return iRet;
    }

    template <class T, class C> bool HTGraph<T,C>::isDirected()
    {
        return _bDirected;
    }

    template <class T, class C> bool HTGraph<T,C>::deletesElements()
    {
        return _bDeleteElements;
    }

    template <class T, class C> void HTGraph<T,C>::display (FILE *pOutput)
    {
        if (pOutput == NULL) {
            return;
        }

        Vertex<T,C> *pCurrentVertex;
        Edge<C> *pCurrentEdge;
        // For each Vertex T in the String Hashtable
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {
            
            pCurrentVertex = i.getValue();            
            if (pCurrentVertex != NULL) {
                // Print the Vertex Key
                fprintf (pOutput, "[%s]===>", i.getKey());
                pCurrentEdge = pCurrentVertex->_edgeList.getFirst();
                if (pCurrentEdge != NULL) {
                    // Print all the Vertex Edges
                    for (; pCurrentEdge != NULL; pCurrentEdge = pCurrentVertex->_edgeList.getNext()) {
                        fprintf (pOutput, "    %s, %s    ", pCurrentEdge->_pszDstVertexKey, pCurrentEdge->_pszEdgeLabel);
                    }
                }
                fprintf (pOutput, "\n");
            }
        }
    }
    
    template <class T, class C> int HTGraph<T,C>::displayEdges (FILE *pOutput, const char *pszKey)
    {
        if (pszKey == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszKey);
        if (pVertex == NULL) {
            // Vertex not found
            return -1;
        }

        // Print the Vertex Key
        fprintf (pOutput, "[%s]===>", pszKey);
        for (Edge<C> *pCurrentEdge = pVertex->_edgeList.getFirst();
             pCurrentEdge != NULL; pCurrentEdge = pVertex->_edgeList.getNext()) {
            
            fprintf (pOutput, "\t%s, %s", pCurrentEdge->_pszDstVertexKey, pCurrentEdge->_pszEdgeLabel);            
        }

        return 0;
    }

    template <class T, class C> void HTGraph<T,C>::empty()
    {
        // Remove all the edges from all the elements
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            Vertex<T,C> *pCurrentVertex = i.getValue();
            if (pCurrentVertex != NULL) {
                Edge<C> *pCurr, *pNext, *pDel;
                pNext = pCurrentVertex->_edgeList.getFirst();
                while ((pCurr = pNext) != NULL) {
                    pNext = pCurrentVertex->_edgeList.getNext();
                    pDel = pCurrentVertex->_edgeList.remove (pCurr);
                    delete pDel;
                }
            }
        }

        _vertexHashtable.removeAll();
        _ui16EdgeNum = 0;
        _ui16VertexNum = 0;
    }

    template <class T, class C> int HTGraph<T,C>::deletePartitions (const char *pszVertexKey)
    {
        if (pszVertexKey == NULL) {
            return -1;
        }

        Vertex<T,C> *pVertex = _vertexHashtable.get (pszVertexKey);
        if (pVertex == NULL) {
            return -1;
        }

        StringHashtable<bool> connectedVertexes;
        traverse (pVertex, &connectedVertexes);
        DArray2<String> vertexesToRemove;
        int k = 0;
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            // If the current vertex in the graph is not in the connected hash table vertexes and
            // is different from the Vertex itself, then delete the Vertex and its Edges from the graph
            if (!connectedVertexes.containsKey (i.getKey()) && (strcmp (pszVertexKey, i.getKey()) != 0)) {
                vertexesToRemove[k++] = i.getKey();                
            }
        }
        
        for (unsigned int i = 0; i < vertexesToRemove.size(); i++) {
            if (vertexesToRemove.used (i)) {
                removeVertex (vertexesToRemove[i].c_str());
            }
        }

        return 0;
    }

    template <class T, class C> bool HTGraph<T,C>::isEmpty()
    {
        return (_ui16VertexNum == 0 ? true : false);
    }

    template <class T, class C> const char * HTGraph<T,C>::updateNeighborhood (Reader *pReader, uint32 *pui32BytesRead)
    {
        if (pReader == NULL) {
            return NULL;
        }

        // Read the Num of Vertex
        uint16 ui16NumOfVertex;
        pReader->read16 (&ui16NumOfVertex);
        (*pui32BytesRead) = (*pui32BytesRead) + 2;
        
        return updatePrivateNeighborhood (pReader, pui32BytesRead);
    }

    template <class T, class C> PtrLList<const char> * HTGraph<T,C>::updateGraph (Reader *pReader, uint32 *pui32BytesRead)
    {
        if (pReader == NULL) {
            return NULL;
        }

        // Read the number of Vertexes
        uint16 ui16VertexNum;
        pReader->read16 (&ui16VertexNum);
        (*pui32BytesRead) = (*pui32BytesRead) + 2;

        PtrLList<const char> *pRet = new PtrLList<const char> ();
        const char *pszRet;
        for (int i=0; i < ui16VertexNum; i++) {
            pszRet = updateNeighborhood (pReader, pui32BytesRead);
            if (pszRet != NULL) {
                pRet->append (pszRet);
            }
        }

        return pRet;        
    }

    template <class T, class C> int HTGraph<T,C>::writeNeighborhood (Writer *pWriter, uint32 *pui32BytesWritten, const char *pszVertexKey)
    {
        if (pWriter == NULL || pszVertexKey == NULL) {
            return -1;
        }

        uint16 ui16VertexNum = 1;
        pWriter->write16 (&ui16VertexNum);
        (*pui32BytesWritten) = (*pui32BytesWritten) + 2;

        return writePrivateNeighborhood (pWriter, pui32BytesWritten, pszVertexKey);        
    }
  
    template <class T, class C> int HTGraph<T,C>::writeGraph (Writer *pWriter, uint32 *pui32BytesWritten)
    {
        if (pWriter == NULL) {
            return -1;
        }

        // Write the number of Vertexes in the graph
        uint16 ui16VertexesNo = _ui16VertexNum;
        pWriter->write16 (&ui16VertexesNo);
        (*pui32BytesWritten) = (*pui32BytesWritten) + 2;
        
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            // Call write neighborhood
            if (writeNeighborhood (pWriter, pui32BytesWritten, i.getKey()) < 0) {
                return -1;
            }
        }

        return 0;
    }

    template <class T, class C> int HTGraph<T,C>::writeCompleteGraph (Writer *pWriter, uint32 *pui32BytesWritten)
    {
        if (pWriter == NULL || pui32BytesWritten == NULL) {
            return -1;
        }

        // Write the number of Vertexes in the graph
        uint16 ui16VertexesNo = _ui16VertexNum;
        pWriter->write16 (&ui16VertexesNo);
        (*pui32BytesWritten) = (*pui32BytesWritten) + 2;

        Vertex<T,C> *pCurrVertex;
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {

            // Write the Vertex Key
            uint8 ui8Length = 0;
            String sVertexKey (i.getKey());
            if (sVertexKey.length () == 0) {
                return -1;
            }
            ui8Length = sVertexKey.length ();
            pWriter->write8 (&ui8Length);
            pWriter->writeBytes (sVertexKey.c_str(), ui8Length);
            (*pui32BytesWritten) = (*pui32BytesWritten) + ui8Length + 1;

            // Write the Template T
            pCurrVertex = i.getValue();
            pCurrVertex->_pElement->write (pWriter, pui32BytesWritten);

            // Write the number of Edges of the current Vertex
            uint16 ui16EdgeNum = (uint16)pCurrVertex->_edgeList.getCount();
            pWriter->write16 (&ui16EdgeNum);
            (*pui32BytesWritten) = (*pui32BytesWritten) + 2;

            // Write all the Edges in the edge list
            for (Edge<C> *pCurrEdge = pCurrVertex->_edgeList.getFirst(); pCurrEdge != NULL;
                 pCurrEdge = pCurrVertex->_edgeList.getNext()) {

                // Write length and Destination Vertex Key
                String sDestKey (pCurrEdge->_pszDstVertexKey);
                ui8Length = sDestKey.length();
                if (ui8Length == 0) {
                    return -1;
                }
                pWriter->write8(&ui8Length);
                pWriter->writeBytes (sDestKey.c_str(), ui8Length);
                (*pui32BytesWritten) = (*pui32BytesWritten) + ui8Length +1; 

                // Write length and Edge label
                String sEdgeLabel (pCurrEdge->_pszEdgeLabel);
                ui8Length = sEdgeLabel.length();
                if (ui8Length == 0) {
                    return -1;
                }
                pWriter->write8(&ui8Length);
                pWriter->writeBytes (sEdgeLabel.c_str(), ui8Length);
                (*pui32BytesWritten) = (*pui32BytesWritten) + ui8Length +1;

                // Write the Cost
                pCurrEdge->_pEdgeElement->write (pWriter, pui32BytesWritten);
            }
        }

        return 0;
    }

    template <class T, class C> int HTGraph<T,C>::readCompleteGraph (Reader *pReader, uint32 *pui32BytesRead)
    {
        if (pReader == NULL || pui32BytesRead == NULL) {
            return -1;
        }

        // Read the number of the vertex
        uint16 ui16VertexNum;
        pReader->read16 (&ui16VertexNum);
        (*pui32BytesRead) = (*pui32BytesRead) + 2;

        for (int i=0; i<ui16VertexNum; i++) {

            // Read the Vertex Key
            uint8 ui8Length;
            pReader->read8 (&ui8Length);
            char *pszKey = new char [ui8Length+1];
            pReader->read (pszKey, ui8Length);
            pszKey[ui8Length] = '\0';
            (*pui32BytesRead) = (*pui32BytesRead) + ui8Length +1;

            // Read the Template
            T *pTemp = new T ();
            pTemp->read (pReader, pui32BytesRead);

            // Add Vertex to the Graph
            addVertex (pszKey, pTemp);

            // Read the Edge numbers
            uint16 ui16EdgeNum;
            pReader->read16 (&ui16EdgeNum);
            (*pui32BytesRead) = (*pui32BytesRead) + 2;

            for (int j = 0; j < ui16EdgeNum; j++) {
                // Read Destination Vertex Key
                pReader->read8 (&ui8Length);
                char *pszDestKey = new char [ui8Length+1];
                pReader->read (pszDestKey, ui8Length);
                pszDestKey[ui8Length] = '\0';
                (*pui32BytesRead) = (*pui32BytesRead) + ui8Length +1;
                // Read Edge label
                pReader->read8 (&ui8Length);
                char *pszEdgeLabel = new char [ui8Length+1];
                pReader->read (pszEdgeLabel, ui8Length);
                pszEdgeLabel[ui8Length] = '\0';
                (*pui32BytesRead) = (*pui32BytesRead) + ui8Length +1;
                // Read cost
                C *pCost = new C ();
                pCost->read (pReader, pui32BytesRead);
                // Add the Edge
                addEdge (pszKey, pszDestKey, pszEdgeLabel, pCost);
                delete[] pszDestKey;
                delete[] pszEdgeLabel;
            }
            delete[] pszKey;
        }
        return 0;
    }

    template <class T, class C> const char * HTGraph<T,C>::updatePrivateNeighborhood (Reader *pReader, uint32 *pui32BytesRead)
    {
        char *pszRet = NULL;
        // Read the length and Vertex key    
        uint8 ui8Length;
        pReader->read8 (&ui8Length);
        if (ui8Length == 0) {
            return NULL;
        }

        char *pszVertexKey = new char [ui8Length + 1];
        if (pszVertexKey == NULL) {
            return NULL;
        }
        pReader->readBytes (pszVertexKey, ui8Length);
        pszVertexKey[ui8Length] = '\0';
        (*pui32BytesRead) = (*pui32BytesRead) + ui8Length + 1;

        // Read  the Template T.
        T *pVertexTemp = new T ();
        if (pVertexTemp == NULL) {
            return NULL;
        }
        pVertexTemp->read (pReader, pui32BytesRead);

        // Read the number of neighbors
        uint16 ui16Neighbors;
        pReader->read16 (&ui16Neighbors);
        (*pui32BytesRead) = (*pui32BytesRead) + 2;

        // Read the Neighbors

        PtrLList<Edge<C> > *pEdgeList = new PtrLList<Edge<C> > ();
        if (pEdgeList == NULL) {
            return NULL;
        }
        DArray2<T> *pNeighborsArray = new DArray2<T> ();
        for (int i=0; i < ui16Neighbors; i++) {
            // Read Neighbor Vertex Key
            pReader->read8 (&ui8Length);
            char *pszBuf = new char [ui8Length + 1];
            if (pszBuf == NULL) {
                return NULL;
            }
            pReader->readBytes (pszBuf, ui8Length);
            pszBuf[ui8Length] = '\0';
            (*pui32BytesRead) = (*pui32BytesRead) + ui8Length + 1;

            // Create a new Empty Edge with a copy of the Neighbor Key
            Edge<C> *pEdge = new Edge<C> (pszBuf);
            if (pEdge == NULL) {
                return NULL;
            }
            delete[] pszBuf;
            pEdgeList->append (pEdge);

            // Read the Neighbor template T. Keep it in the Array
            T *pNeighTemp = new T();
            if (pNeighTemp == NULL) {
                return NULL;
            }
            pNeighTemp->read (pReader, pui32BytesRead);
            (*pNeighborsArray)[i] = pNeighTemp;            
        }

        // If the Graph is Empty do as follow:
        // 1) Create a new Vertex containing the Template (pVertexTemp) and the Edges read. 
        //    Then add the Vertex into the Graph. 
        // 2) For each Neighbor read in the Edge list, create a new Vertex that includes the Neighbor Template
        //    and an Edge pointing toward the pVertexTemp (if the Graph is not directed)      
        if (isEmpty()) {
            // Add the Vertex to the Graph
            Vertex<T,C> *pNewVertex = new Vertex<T,C> ();
            if (pNewVertex == NULL) {
                return NULL;
            }
            pNewVertex->_pElement = pVertexTemp;
            // Copy the Edge list
            for (Edge<C> *pCurrEdge = pEdgeList->getFirst(); pCurrEdge != NULL;
                 pCurrEdge = pEdgeList->getNext()) {
                
                pNewVertex->_edgeList.append (pCurrEdge);
            }
            // Insert the Vertex in the Graph
            _vertexHashtable.put (pszVertexKey, pNewVertex);
            // Update Vertex Number and Edge Num
            _ui16VertexNum++;
            _ui16EdgeNum += pEdgeList->getCount();
            
            // Insert all the neighbors vertex along with their Templates in the graph
            int i = 0;
            Edge<C> *pNewEdge;
            for (Edge<C> *pCurrEdge = pEdgeList->getFirst(); pCurrEdge != NULL;
                 pCurrEdge = pEdgeList->getNext()) {
                
                // Add the Neighbor Vertex and its template
                addVertex (pCurrEdge->_pszDstVertexKey, &(*pNeighborsArray)[i++]);
                _ui16VertexNum++;
                // If the Graph is not directed
                if (!_bDirected) {
                    pNewEdge = new Edge<C> (pszVertexKey);
                    if (pNewEdge == NULL) {
                        return NULL;
                    }                   
                    (getEdgeList (pCurrEdge->_pszDstVertexKey))->append (pNewEdge);
                    _ui16EdgeNum++;
                }
            }
            pszRet = pszVertexKey;
        }
        else {
            // If the vertex already is in the Graph
            if (hasVertex (pszVertexKey)) {
                /* Create a new Vertex */
                Vertex<T,C> *pNewVertex = new Vertex<T,C> ();
                if (pNewVertex == NULL) {
                    return NULL;
                }
                pNewVertex->_pElement = pVertexTemp;
                // Copy the Edges read in the new empty Edge list
                for (Edge<C> *pCurrEdge = pEdgeList->getFirst(); pCurrEdge != NULL;
                     pCurrEdge = pEdgeList->getNext()) {
                
                    pNewVertex->_edgeList.append (pCurrEdge);
                }
                // Update new Edges if they are found in the old edge list
                Vertex<T,C> *pOldVertex = _vertexHashtable.get (pszVertexKey);
                PtrLList<Edge<C> > *pDeletedEdges = new PtrLList<Edge<C> >();
                for (Edge<C> *pOldEdge = pOldVertex->_edgeList.getFirst(); pOldEdge != NULL;
                     pOldEdge = pOldVertex->_edgeList.getNext()) {

                    bool bFound = false;
                    // Search the Old Edge in the New List. If it is not found add  it to 
                    // DeletedEdges list.
                    for (Edge<C> *pNewEdge = pNewVertex->_edgeList.getFirst(); pNewEdge != NULL;
                         pNewEdge = pNewVertex->_edgeList.getNext()) {
                        
                        // If the Old Edge is found it, maintain the previous information about EdgeLabel and Cost
                        if (strcmp (pNewEdge->_pszDstVertexKey, pOldEdge->_pszDstVertexKey) == 0) {
                            // Copy the Edge Key
                            char *pszCopyId = (char*) malloc (strlen(pOldEdge->_pszEdgeLabel)+1);
                            strcpy (pszCopyId, pOldEdge->_pszEdgeLabel);
                            pNewEdge->_pszEdgeLabel = pszCopyId;
                            // Maintain the reference of the Cos
                            pNewEdge->_pEdgeElement = pOldEdge->_pEdgeElement;
                            pNewEdge->_bReferenceKept = pOldEdge->_bReferenceKept;
                            bFound = true;
                       }    
                    }

                    if (!bFound) {
                        pDeletedEdges->append (pOldEdge);
                    }
                }

                pOldVertex = _vertexHashtable.put (pszVertexKey, pNewVertex);
                // The Number of Edges needs to be updated
                _ui16EdgeNum = _ui16EdgeNum - pOldVertex->_edgeList.getCount() + pNewVertex->_edgeList.getCount();
                // Compare the New Vertex and the Old Vertex. If they are different update the OldVertex
                if (compareVertexes (pNewVertex, pOldVertex) < 0) {
                    pszRet = pszVertexKey;
                }

                // Update Neighbors
                int i = 0;
                for (Edge<C> *pCurrNeighEdge = pEdgeList->getFirst(); 
                     pCurrNeighEdge != NULL; pCurrNeighEdge = pEdgeList->getNext()) {
                    
                    // If the neighbor does not exist in the Graph add it
                    if (!hasVertex (pCurrNeighEdge->_pszDstVertexKey)) {
                        addVertex (pCurrNeighEdge->_pszDstVertexKey, &(*pNeighborsArray)[i]);
                        _ui16VertexNum++;
                        // Add the Edge if the Graph is not directed
                        if (!_bDirected) {
                            Edge<C> *pNewEdge = new Edge<C> (pszVertexKey);
                            (getEdgeList (pCurrNeighEdge->_pszDstVertexKey))->append(pNewEdge);
                            _ui16EdgeNum++;
                        }
                    }
                    else {
                        if(!_bDirected){
                            // Check if there is an Edge already toward the Vertex
                            PtrLList<Edge<C> > *pNeighborList = getEdgeList (pCurrNeighEdge->_pszDstVertexKey);
                            bool bFound = false;
                            for (Edge<C> *pEdge = pNeighborList->getFirst();
                                 pEdge != NULL && !bFound; pEdge = pNeighborList->getNext()) {
                            
                                if (strcmp(pEdge->_pszDstVertexKey, pszVertexKey) == 0) {
                                    bFound = true;
                                }
                            }

                            if (!bFound) {
                                Edge<C> *pEdge = new Edge<C> (pszVertexKey);
                                pNeighborList->append (pEdge);
                            }

                            for (Edge<C> *pEdge2Delete = pDeletedEdges->getFirst(); pEdge2Delete != NULL;
                                 pEdge2Delete = pDeletedEdges->getNext()) {
                                
                                Edge<C> edgeToSearch (pszVertexKey, pEdge2Delete->_pszEdgeLabel);
                                PtrLList<Edge<C> > *pList = getEdgeList (pEdge2Delete->_pszDstVertexKey);
                                Edge<C> *pRemovedEdge = pList->remove (&edgeToSearch);
                                if (pRemovedEdge != NULL) {
                                    delete pRemovedEdge;
                                }
                            }
                        }
                    }
                    i++;
                }    

                if (pOldVertex != NULL) {
                    delete pOldVertex;
                }

                delete pDeletedEdges;
            }
            else {
                // The source Vertex doesn't exist. Add it to the Graph with its own EdgeList.
                // Then update its neighbors 
                Vertex<T,C> *pNewVertex = new Vertex<T,C> ();
                if (pNewVertex == NULL) {
                    return NULL;
                }
                pNewVertex->_pElement = pVertexTemp;

                // Copy the Edge list
                for (Edge<C> *pCurrEdge = pEdgeList->getFirst(); pCurrEdge != NULL;
                     pCurrEdge = pEdgeList->getNext()) {
                
                    pNewVertex->_edgeList.append (pCurrEdge);
                }

                // Insert the Vertex in the Graph
                _vertexHashtable.put (pszVertexKey, pNewVertex);
                // Update Vertex Number and Edge Num
                _ui16VertexNum++;
                _ui16EdgeNum += pEdgeList->getCount();

                // Update Neighbors
                int i = 0;
                for (Edge<C> *pCurrNeighEdge = pEdgeList->getFirst(); 
                     pCurrNeighEdge != NULL; pCurrNeighEdge = pEdgeList->getNext()) {

                    // If the neighbor does not exist in the Graph add it
                    if (!hasVertex (pCurrNeighEdge->_pszDstVertexKey)) {
                        addVertex (pCurrNeighEdge->_pszDstVertexKey, &(*pNeighborsArray)[i]);
                        _ui16VertexNum++;
                        // Add the Edge if the Graph is not directed
                        if (!_bDirected) {
                            Edge<C> *pNewEdge = new Edge<C> (pszVertexKey);
                            (getEdgeList (pCurrNeighEdge->_pszDstVertexKey))->append (pNewEdge);
                            _ui16EdgeNum++;
                        }
                    }
                    else {
                        // Update the Template
                        if(!_bDirected){
                            // Check if there is an Edge already toward the Vertex
                            PtrLList<Edge<C> > *pNeighList = getEdgeList (pCurrNeighEdge->_pszDstVertexKey); 
                            bool bFound = false;
                            for (Edge<C> *pEdge = pNeighList->getFirst();
                                 pEdge != NULL && !bFound; pEdge = pNeighList->getNext()) {

                                if (strcmp(pEdge->_pszDstVertexKey, pszVertexKey) == 0) {
                                    bFound = true;
                                }
                            }

                            if (!bFound) {
                                Edge<C> *pEdge = new Edge<C> (pszVertexKey);
                                pNeighList->append (pEdge);
                            }
                        }
                    }
                    i++;
                }    
                pszRet = pszVertexKey;
            }
        }

        delete pEdgeList;
        return pszRet;        
    }

    template <class T, class C> int HTGraph<T,C>::writePrivateNeighborhood (Writer *pWriter, uint32 *pui32BytesWritten, const char *pszVertexKey)
    {
        // Check if the Vertex is in the Graph
        Vertex<T,C> *pVertex, *pNeighbor;
        pVertex = _vertexHashtable.get (pszVertexKey);
        if (pVertex == NULL) {
            return -1;
        }

        // Write the Vertex Key
        uint8 ui8Length;
        String sVertexKey (pszVertexKey);
        if (sVertexKey.length () == 0) {
            return -1;
        }
        ui8Length = sVertexKey.length ();
        pWriter->write8 (&ui8Length);
        pWriter->writeBytes (pszVertexKey, ui8Length);
        (*pui32BytesWritten) = (*pui32BytesWritten) + ui8Length + 1;        

        // Write the template. Remember to update the value of ui32BytesWritten
        // with the bytes written in the template
        pVertex->_pElement->write (pWriter, pui32BytesWritten);
        
        // Write all the Neighbors
        PtrLList<String> *pNeighborsList = new PtrLList<String>();
        if (pNeighborsList == NULL) {
            return -1;
        }
        int rc = getNeighborsList (pszVertexKey, pNeighborsList);
        if (rc < 0 || rc > 0xFFFF) {
            return -1;
        }
        uint16 ui16NeighborsNum = (uint16) rc;

        // Write the number of Neighbors
        pWriter->write16 (&ui16NeighborsNum);
        (*pui32BytesWritten) = (*pui32BytesWritten) + 2;

        for (String *pCurrNeighbor = pNeighborsList->getFirst(); pCurrNeighbor != NULL;
             pCurrNeighbor = pNeighborsList->getNext()) {
       
            // Write length and Destination Vertex Key
            String sTemp (pCurrNeighbor->c_str());
            ui8Length = sTemp.length();
            if (ui8Length == 0) {
                return -1;
            }
            pWriter->write8(&ui8Length);
            pWriter->writeBytes (pCurrNeighbor->c_str(), ui8Length);
            (*pui32BytesWritten) = (*pui32BytesWritten) + ui8Length + 1; 

            // Write the neighbor template 
            pNeighbor = _vertexHashtable.get (pCurrNeighbor->c_str());
            if (pNeighbor == NULL) {
                return -1;
            }
            pNeighbor->_pElement->write (pWriter, pui32BytesWritten);
        }
        
        delete pNeighborsList;
        return 0;
    }

    template <class T, class C> void HTGraph<T,C>::traverse(Vertex<T,C> *pVertex, StringHashtable<bool> *pVisited)
    {        
        if (pVertex == NULL || pVisited == NULL) {
            return;
        }
        bool bVisited = true;
        for (Edge<C> *pEdge = pVertex->_edgeList.getFirst(); pEdge != NULL;
             pEdge = pVertex->_edgeList.getNext()) {

            if (pEdge != NULL && !pVisited->containsKey (pEdge->_pszDstVertexKey)) {
                pVisited->put (pEdge->_pszDstVertexKey, &bVisited);
                traverse (_vertexHashtable.get (pEdge->_pszDstVertexKey), pVisited);
            }
        }
    }

    template <class T, class C> void HTGraph<T,C>::traverse (Vertex<T,C> *pVertex, StringHashtable<bool> *pVisited, uint16 ui16Hops)
    {
        if (pVertex == NULL || pVisited == NULL) {
            return;
        }

        bool bVisited = true;
        for (Edge<C> *pEdge = pVertex->_edgeList.getFirst(); pEdge != NULL;
             pEdge = pVertex->_edgeList.getNext()) {
                        
            if (pEdge != NULL && !pVisited->containsKey (pEdge->_pszDstVertexKey) && ui16Hops > 0) {
                pVisited->put (pEdge->_pszDstVertexKey, &bVisited);
                traverse (_vertexHashtable.get (pEdge->_pszDstVertexKey), pVisited, (ui16Hops-1));
            }
        }
    }

    template <class T, class C> HTGraph<T,C> * HTGraph<T,C>::getShortestPathTree (const char *pszRootKey, bool bUseCost)
    {
        if (pszRootKey == NULL) {
            return NULL;
        }
        // Get the number of Vertex
        uint16 ui16NumOfVertex = getVertexCount();
        if (ui16NumOfVertex == 0) {
            return NULL;
        }
        // Allocate data structures
        HTGraph<T,C> * pRet = new HTGraph<T,C> (_bDeleteElements, _bDirected);
        if (pRet == NULL) {
            return NULL;
        }

        StringHashtable<float> distances;
        float *pInitializationArray = (float *) calloc (ui16NumOfVertex, sizeof (float));
        float fInfinity = 0xFFFFFFFF;   
        PtrQueue<String> candidateQueue;         
        PtrLList<String> stringList;

        // Initialize data structures
        int k = 0;
        for (typename StringHashtable<Vertex<T,C> >::Iterator i = _vertexHashtable.getAllElements();
             !i.end(); i.nextElement()) {
            
            pInitializationArray[k] = fInfinity;
            distances.put (i.getKey(), (pInitializationArray + k));
            candidateQueue.enqueue (new String (i.getKey()));
            k++;
        }

        (*distances.get (pszRootKey)) = 0;

        // Until all the candidate have been evaluated
        while (!candidateQueue.isEmpty()) {
            // Get the Vertex with the minimum distance from the Candidate Queue
            const char *pszClosestVertex = getClosestVertex (&distances, &candidateQueue);
            // If the candidate is not reachable then terminate
            if (pszClosestVertex == NULL) {
                break;
            }
            float *fDistance = distances.get (pszClosestVertex);
            if ((fDistance == NULL) || (*fDistance) == fInfinity) {
                break;
            }
            // Add the closest vertex to the MST
            pRet->addVertex (pszClosestVertex, getVertex (pszClosestVertex));
            // Remove the closest vertex from the candidate queue
            String sClosest (pszClosestVertex);
            stringList.prepend (candidateQueue.remove (&sClosest));
            
            // For each neighbors of the closest vertex find the closest neighbor and add it to the MST. 
            // It also creates a new Minimum Edge from the closest Vertex to its closest neighbor.
            PtrLList<Edge<C> > *pNeighborsList = getEdgeList (pszClosestVertex);            
            for (Edge<C> *pEdge = pNeighborsList->getFirst(); pEdge != NULL;
                 pEdge = pNeighborsList->getNext()) {

                float fCost;
                if (bUseCost) {
                    fCost = pEdge->_pEdgeElement->getCost();
                }
                else {
                    fCost = 1;
                }

                float fActualCost = (*distances.get (pszClosestVertex)) + fCost;
                if (fActualCost < (*distances.get (pEdge->_pszDstVertexKey))) {
                    (*distances.get (pEdge->_pszDstVertexKey)) = fActualCost;

                    PtrLList<Edge<C> > edgeList;
                    pRet->getEdgeList(pszClosestVertex, pEdge->_pszDstVertexKey, &edgeList);
                    Edge<C> *pOldEdge = edgeList.getFirst();
                    C *pNewCost = new C ();
                    pNewCost->setCost (fCost);
                    if (pOldEdge == NULL) {
                        pRet->addVertex (pEdge->_pszDstVertexKey, getVertex (pEdge->_pszDstVertexKey));
                        pRet->addEdge (pszClosestVertex, pEdge->_pszDstVertexKey, pEdge->_pszEdgeLabel, pNewCost);
                    }
                    else {
                        String sOldDestVertexKey (pOldEdge->_pszDstVertexKey);
                        String sOldEdgeLabel (pOldEdge->_pszEdgeLabel);
                        pRet->removeEdge (pszClosestVertex, sOldDestVertexKey.c_str(), sOldEdgeLabel.c_str());
                        pRet->addEdge (pszClosestVertex, pEdge->_pszDstVertexKey, pEdge->_pszEdgeLabel, pNewCost);
                    }                    
                }                                
            }            
        }

        String *pCurr, *pNext, *pDel;
        pNext = stringList.getFirst();
        while ((pCurr = pNext) != NULL) {
            pNext = stringList.getNext();
            pDel = stringList.remove (pCurr);
            delete pDel;
        }        

        free (pInitializationArray);
        return pRet;
    }

    template <class T, class C> const char * HTGraph<T,C>::getClosestVertex (StringHashtable<float> *pDistances, PtrQueue<String> *pQueue)
    {
        float fMin = 0xFFFFFFFFf;
        String *pszRet = NULL;
        for (String *pszCandidate = pQueue->getFirst(); pszCandidate != NULL;
             pszCandidate = pQueue->getNext()) {
            
            if ((*pDistances->get (pszCandidate->c_str())) <= fMin) {
                fMin = ((*pDistances->get (pszCandidate->c_str())));
                pszRet = pszCandidate;
            }
        }
        if (pszRet == NULL) {
            return NULL;
        }
        return pszRet->c_str();
    }        
}

#endif  // INCL_GRAPH_H

