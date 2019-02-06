/*
 * FGraphLocal.java
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

package us.ihmc.ds.fgraph;

import java.io.Serializable;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

/**
 * FGraphLocal
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Apr 21, 2004 at 6:17:12 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphLocal implements Serializable
{
    /**
     * Constructor to create and use just a local graph without the client-server
     * capabilities of FGraph
     */
    public FGraphLocal()
    {
        _localGraph = new FGraphInfo();
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getServer()
     */
    protected FGraphLocal (FGraphInfo localGraph)
    {
        _localGraph = localGraph;
    }

    //////////////////////////// FGInfoGraph Manipulation ////////////////////////////
    /**
     * @see us.ihmc.ds.fgraph.FGraph#addVertex(java.lang.String)
     *
     * @param vertexID the specified vertexID
     * @throws FGraphException An exceptino is thrown if the ID is duplicate or if the fginfo is unable to add the vertex.
     */
    public void addVertex (String vertexID)
        throws FGraphException
    {
        _localGraph.addVertex(vertexID);
        debugMsg ("Add FGInfoVertex (" + vertexID + ")");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addVertex(java.lang.String, java.util.Hashtable)
     *
     * @param vertexID the specified vertexID
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException An exceptino is thrown if the ID is duplicate or if the fginfo is unable to add the vertex.
     */
    public void addVertex (String vertexID, Hashtable attributeList)
        throws FGraphException
    {
        _localGraph.addVertex(vertexID);
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (attributeList != null && vertex != null) {
            Enumeration en= attributeList.keys();
            while (en.hasMoreElements()) {
                String skey = (String) en.nextElement();
                vertex.setAttribute (skey, attributeList.get(skey));
            }
        }
        debugMsg ("Add FGInfoVertex (" + vertexID + ")");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#removeVertex(java.lang.String)
     *
     * @param vertexID vertex ID
     * @throws FGraphException
     */
    public void removeVertex (String vertexID)
        throws FGraphException
    {
        _localGraph.removeVertex(vertexID);
        debugMsg ("Remove FGInfoVertex (" + vertexID + ")");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setVertexAttribute(java.lang.String, java.lang.String, java.io.Serializable)
     *
     * @param vertexID the ID of the vertex where the attribute will be set.
     * @param attributeKey the key (a string)
     * @param attribute the attribute (a seriablizable object)
     * @throws FGraphException if the vertexID if not found
     */
    public void setVertexAttribute (String vertexID,
                                    String attributeKey,
                                    Serializable attribute)
        throws FGraphException
    {
        debugMsg ("Set FGInfoVertex Attribute (" + vertexID + " : " + attributeKey + ")");
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        vertex.setAttribute (attributeKey, attribute);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setVertexAttributeList(java.lang.String, java.util.Hashtable)
     *
     * @param vertexID the ID of the vertex where the attribute will be set.
     * @param attributeList the attribute (a seriablizable object)
     * @throws FGraphException if the vertexID if not found
     */
    public void setVertexAttributeList (String vertexID, Hashtable attributeList)
        throws FGraphException
    {
        debugMsg ("Set FGInfoVertex Attribute (" + vertexID + ")");
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        Enumeration en= attributeList.keys();
        while (en.hasMoreElements()) {
            String skey = (String) en.nextElement();
            vertex.setAttribute (skey, attributeList.get(skey));
        }
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertexAttribute(java.lang.String, java.lang.String)
     *
     * @param vertexID The ID of the vertex
     * @param attributeKey  The key of the attribute
     * @return Object The attribute value
     * @throws FGraphException If vertexID is unknown
     */
    public Object getVertexAttribute (String vertexID,
                                      String attributeKey)
        throws FGraphException
    {
        debugMsg ("Get FGInfoVertex Attribute (" + vertexID + " : " + attributeKey + ")");
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        return ((Object) vertex.getAttribute(attributeKey));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertexAttributeList(java.lang.String)
     *
     * @param vertexID The ID of the vertex
     * @return Hashtable The attribute value
     * @throws FGraphException If vertexID is unknown
     */
    public Hashtable getVertexAttributeList (String vertexID)
        throws FGraphException
    {
        Hashtable attList = new Hashtable();
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        Enumeration en= vertex.listAttributeKeys();
        while (en.hasMoreElements()) {
            String skey = (String) en.nextElement();
            attList.put(skey, vertex.getAttribute(skey));
        }
        return (attList);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#removeVertexAttribute(java.lang.String, java.lang.String)
     *
     * @param vertexID the ID of the vertex
     * @param attributeKey the key of the attribute
     * @return the value of the attribute, or null if the attribute did not exist
     * @throws FGraphException
     */
    public Object removeVertexAttribute (String vertexID,
                                         String attributeKey)
        throws FGraphException
    {
        debugMsg ("Remove FGInfoVertex Attribute (" + vertexID + " : " + attributeKey + ")");
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        return vertex.removeAttribute (attributeKey);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#listEdgeAttributes(java.lang.String)
     *
     * @param vertexID  The ID of the vertex
     * @return An Enumeration containing all the attribute keys
     * @throws FGraphException If the vertexID is unknown
     */
    public Enumeration listVertexAttributes (String vertexID)
        throws FGraphException
    {
        {
            FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
            if (vertex == null) {
                throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
            }
            return (vertex.listAttributeKeys());
        }
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addEdge(java.lang.String, java.lang.String, java.lang.String)
     *
     * @param edgeID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param sourceVertexID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param targetVertexID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @throws FGraphException if the edgeID is already in use or if the source or target verticeIDs are unknown.
     */
    public void addEdge (String edgeID, String sourceVertexID, String targetVertexID)
        throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        _localGraph.addEdge(edgeID, sourceVertexID, targetVertexID);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addEdge(java.lang.String, java.lang.String,
     *                                       java.lang.String, java.util.Hashtable)
     *
     * @param edgeID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param sourceVertexID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param targetVertexID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException if the edgeID is already in use or if the source or target verticeIDs are unknown.
     */
    public void addEdge (String edgeID, String sourceVertexID,
                         String targetVertexID, Hashtable attributeList)
        throws FGraphException
    {
        _localGraph.addEdge (edgeID, sourceVertexID, targetVertexID);
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (attributeList != null && edge != null) {
            Enumeration en= attributeList.keys();
            while (en.hasMoreElements()) {
                String skey = (String) en.nextElement();
                edge.setAttribute (skey, attributeList.get(skey));
            }
        }
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addUndirectedEdge(java.lang.String, java.lang.String)
     *
     * @param firstEndPointID The ID (String) of one of the endpoints
     * @param secondEndPointID The ID (String) of one of the endpoints
     * @return vertexID the unique of this VERTEX.
     * @throws FGraphException If the source of the target vertices are unknown
     */
    public String addUndirectedEdge (String firstEndPointID, String secondEndPointID)
        throws FGraphException
    {
        String edgeID = _localGraph.addUndirectedEdge(firstEndPointID, secondEndPointID);
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        return (edgeID);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addUndirectedEdge(java.lang.String, java.lang.String)
     *
     * @param edgeID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param firstEndPointID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param secondEndPointID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @throws FGraphException if the edgeID is already in use or if the source or target verticeIDs are unknown.
     */
    public void addUndirectedEdge (String edgeID, String firstEndPointID, String secondEndPointID)
        throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        _localGraph.addUndirectedEdge(edgeID, firstEndPointID, secondEndPointID);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addUndirectedEdge(java.lang.String, java.lang.String,
     *                                       java.lang.String, java.util.Hashtable)
     *
     * @param edgeID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param firstEndPointID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param secondEndPointID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException if the edgeID is already in use or if the source or target verticeIDs are unknown.
     */
    public void addUndirectedEdge (String edgeID, String firstEndPointID,
                         String secondEndPointID, Hashtable attributeList)
        throws FGraphException
    {
        _localGraph.addUndirectedEdge (edgeID, firstEndPointID, secondEndPointID);
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (attributeList != null && edge != null) {
            Enumeration en= attributeList.keys();
            while (en.hasMoreElements()) {
                String skey = (String) en.nextElement();
                edge.setAttribute (skey, attributeList.get(skey));
            }
        }
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
    }

    public boolean isEdgeUndirected (String edgeID)
            throws FGraphException
    {
        return _localGraph.isEdgeUndirected(edgeID);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgeSource(java.lang.String)
     *
     * @param edgeID edge ID
     * @return targetID, the ID of the source FGInfoVertex
     * @throws FGraphException
     */
    public String getEdgeSource (String edgeID)
        throws FGraphException
    {
        return (_localGraph.getEdgeSource (edgeID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgeTarget(java.lang.String)
     *
     * @param edgeID edge ID
     * @return targetID, the ID of the target FGInfoVertex
     * @throws FGraphException
     */
    public String getEdgeTarget (String edgeID)
        throws FGraphException
    {
        return (_localGraph.getEdgeTarget (edgeID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getOtherEdgeVertex (java.lang.String, java.lang.String)
     */
    public String getOtherEdgeVertex (String edgeID, String oneVertexID)
        throws FGraphException
    {
        if (_localGraph.getEdgeSource(edgeID).equals (oneVertexID)) {
            return _localGraph.getEdgeTarget (edgeID);
        }
        else if (_localGraph.getEdgeTarget(edgeID).equals (oneVertexID)) {
            return _localGraph.getEdgeSource (edgeID);
        }
        else {
            throw new FGraphException ("edge (" + edgeID + ") does not connect to vertex (" + oneVertexID + ")");
        }
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#removeEdge(java.lang.String)
     *
     * @param edgeID The ID of the edge to be removed
     * @throws FGraphException If the edgeID is unknown
     */
    public void removeEdge (String edgeID)
        throws FGraphException
    {
        debugMsg ("Remove FGInfoEdge (" + edgeID + ")");
        _localGraph.removeEdge(edgeID);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setEdgeAttribute(java.lang.String, java.lang.String, java.io.Serializable)
     *
     * @param edgeID The ID of the edge
     * @param attributeKey  The attribute key (String)
     * @param attribute The attribute (Serializable)
     * @throws FGraphException If the edgeID is unknown
     */
    public void setEdgeAttribute (String edgeID, String attributeKey, Serializable attribute)
        throws FGraphException
    {
        debugMsg ("Set FGInfoEdge Attribute (" + edgeID + " : " + attributeKey + ")");
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (edge == null) {
            throw new FGraphException ("Invalid edge id (" + edgeID + ")");
        }
        edge.setAttribute (attributeKey, attribute);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setEdgeAttributeList(java.lang.String, java.util.Hashtable)
     *
     * @param edgeID The ID of the edge
     * @param attributeList The attribute (Serializable)
     * @throws FGraphException If the edgeID is unknown
     */
    public void setEdgeAttributeList (String edgeID, Hashtable attributeList)
        throws FGraphException
    {
        debugMsg ("Set FGInfoEdge Attribute (" + edgeID + ")");
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (edge == null) {
            throw new FGraphException ("Invalid edge id (" + edgeID + ")");
        }

        Enumeration en= attributeList.keys();
        while (en.hasMoreElements()) {
            String skey = (String) en.nextElement();
            edge.setAttribute (skey, attributeList.get(skey));
        }
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgeAttribute(java.lang.String, java.lang.String)
     *
     * @param edgeID The ID of the edge
     * @param attributeKey  The key of the attribute
     * @return Object The attribute value
     * @throws FGraphException If edgeID is unknown
     */
    public Object getEdgeAttribute (String edgeID,
                                    String attributeKey)
        throws FGraphException
    {
        debugMsg ("Get FGInfoEdge Attribute (" + edgeID + " : " + attributeKey + ")");
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (edge == null) {
            throw new FGraphException ("Invalid edge id (" + edgeID + ")");
        }
        return ((Object) edge.getAttribute(attributeKey));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgeAttributeList(java.lang.String)
     *
     * @param edgeID The ID of the edge
     * @return Object The attribute value
     * @throws FGraphException If edgeID is unknown
     */
    public Hashtable getEdgeAttributeList (String edgeID)
        throws FGraphException
    {
        Hashtable attList = new Hashtable();
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (edge == null) {
            throw new FGraphException ("Invalid edge id (" + edgeID + ")");
        }
        Enumeration en= edge.listAttributeKeys();
        while (en.hasMoreElements()) {
            String skey = (String) en.nextElement();
            attList.put(skey, edge.getAttribute(skey));
        }
        return (attList);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#removeEdgeAttribute(java.lang.String, java.lang.String)
     *
     * @param edgeID The ID of the edge
     * @param attributeKey  The Key of the attribute to be removed
     * @throws FGraphException If the edge is unknown
     */
    public void removeEdgeAttribute (String edgeID, String attributeKey)
        throws FGraphException
    {
        debugMsg ("Remove FGInfoEdge Attribute (" + edgeID + " : " + attributeKey + ")");
        FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
        if (edge == null) {
            throw new FGraphException ("Invalid edge id (" + edgeID + ")");
        }
        edge.removeAttribute (attributeKey);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#listEdgeAttributes(java.lang.String)
     *
     * @param edgeID  The ID of the edge
     * @return An Enumeration containing all the attribute keys
     * @throws FGraphException If the edgeID is unknown
     */
    public Enumeration listEdgeAttributes (String edgeID)
        throws FGraphException
    {
        {
            FEdge edge = (FEdge) _localGraph.getEdge (edgeID);
            if (edge == null) {
                throw new FGraphException ("Invalid edge id (" + edgeID + ")");
            }
            return (edge.listAttributeKeys());
        }
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#hasVertex(java.lang.String)
     *
     * @param vertexID
     * @return  true if the vertex is present, and false otherwise
     */
    public boolean hasVertex (String vertexID)
    {
        return (_localGraph.hasVertex (vertexID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertices()
     *
     * @return Enumeration of vertice IDs
     */
    public Enumeration getVertices ()
    {
        return (_localGraph.getVertexIDs());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertices(java.util.Hashtable)
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return enumeration of vertices
     * @throws FGraphException
     */
    public Enumeration getVertices(Hashtable filterList)
    {
        if (filterList == null) {
            return (getVertices());
        }
        int counter = 0;
        String[] keyList = new String[filterList.size()];
        Object[] objList = new Object[filterList.size()];
        Enumeration filterKeys = filterList.keys();
        while (filterKeys.hasMoreElements()) {
            keyList[counter] = (String) filterKeys.nextElement();
            objList[counter] = filterList.get (keyList[counter]);
            counter++;
        }

        Vector vlist = new Vector();
        Enumeration en= _localGraph.getVertices();
        while (en.hasMoreElements()) {
            boolean control = true;
            FVertex vertex = (FVertex) en.nextElement();
            for (int i=0; i<filterList.size(); i++) {
                Object currObject = vertex.getAttribute(keyList[i]);
                if (currObject == null || !currObject.equals(objList[i])) {
                    control = false;
                    break;
                }
            }
            if (control) {
                vlist.addElement(vertex.getID());
            }
        }
        return (vlist.elements());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdges()
     *
     * @return Enumeration of edge IDs.
     */
    public Enumeration getEdges ()
            throws FGraphException
    {
        return (_localGraph.getEdgeIDs());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdges(java.util.Hashtable)
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return enumeration of egdes
     * @throws FGraphException
     */
    public Enumeration getEdges(Hashtable filterList) throws FGraphException
    {
        if (filterList == null) {
            return (getEdges());
        }
        int counter = 0;
        String[] keyList = new String[filterList.size()];
        Object[] objList = new Object[filterList.size()];
        Enumeration filterKeys = filterList.keys();
        while (filterKeys.hasMoreElements()) {
            keyList[counter] = (String) filterKeys.nextElement();
            objList[counter] = filterList.get (keyList[counter]);
            counter++;
        }

        Vector vlist = new Vector();
        Enumeration en= _localGraph.getEdges();
        while (en.hasMoreElements()) {
            boolean control = true;
            FEdge edge = (FEdge) en.nextElement();
            for (int i=0; i<filterList.size(); i++) {
                Object currObject = edge.getAttribute(keyList[i]);
                if (currObject == null || !currObject.equals(objList[i])) {
                    control = false;
                    break;
                }
            }
            if (control) {
                vlist.addElement(edge.getID());
            }
        }
        return (vlist.elements());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#hasEdge(java.lang.String)
     *
     * @param edgeID
     * @return
     * @throws FGraphException
     */
    public boolean hasEdge (String edgeID)
            throws FGraphException
    {
        return (_localGraph.hasEdge (edgeID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getDegreeIn(java.lang.String)
     *
     * @param vertexID ID of the vertex
     * @return number of incoming edges
     */
    public int getDegreeIn (String vertexID)
           throws FGraphException
    {
        return (_localGraph.getDegreeIn (vertexID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getIncomingEdges(java.lang.String)
     *
     * @param vertexID ID of the FGInfoVertex
     * @return Enumeration of the IDs of the Incoming edges
     */
    public Enumeration getIncomingEdges (String vertexID)
           throws FGraphException
    {
        return (_localGraph.getIncomingEdgeIDs(vertexID) );
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getIncomingEdges(String vertexID, java.util.Hashtable)
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return enumeration of egdes
     * @throws FGraphException
     */
    public Enumeration getIncomingEdges(String vertexID, Hashtable filterList) throws FGraphException
    {
        if (filterList == null) {
            return (getIncomingEdges (vertexID));
        }
        int counter = 0;
        String[] keyList = new String[filterList.size()];
        Object[] objList = new Object[filterList.size()];
        Enumeration filterKeys = filterList.keys();
        while (filterKeys.hasMoreElements()) {
            keyList[counter] = (String) filterKeys.nextElement();
            objList[counter] = filterList.get (keyList[counter]);
            counter++;
        }

        Vector vlist = new Vector();
        Enumeration en= _localGraph.getIncomingEdges (vertexID);
        while (en.hasMoreElements()) {
            boolean control = true;
            FEdge edge = (FEdge) en.nextElement();
            for (int i=0; i<filterList.size(); i++) {
                Object currObject = edge.getAttribute(keyList[i]);
                if (currObject == null || !currObject.equals(objList[i])) {
                    control = false;
                    break;
                }
            }
            if (control) {
                vlist.addElement(edge.getID());
            }
        }
        return (vlist.elements());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getDegreeOut(java.lang.String)
     *
     * @param vertexID
     * @return int
     */
    public int getDegreeOut (String vertexID)
           throws FGraphException
    {
        return (_localGraph.getDegreeOut (vertexID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getOutgoingEdges(java.lang.String)
     *
     * @param vertexID The ID of the vertex
     * @return An enumeration with all the IDs of the outgoing edges
     */
    public Enumeration getOutgoingEdges (String vertexID)
           throws FGraphException
    {
        return (_localGraph.getOutgoingEdgeIDs(vertexID) );
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getOutgoingEdges(String vertexID, java.util.Hashtable)
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return enumeration of egdes
     * @throws FGraphException
     */
    public Enumeration getOutgoingEdges(String vertexID, Hashtable filterList) throws FGraphException
    {
        if (filterList == null) {
            return (getOutgoingEdges (vertexID));
        }
        int counter = 0;
        String[] keyList = new String[filterList.size()];
        Object[] objList = new Object[filterList.size()];
        Enumeration filterKeys = filterList.keys();
        while (filterKeys.hasMoreElements()) {
            keyList[counter] = (String) filterKeys.nextElement();
            objList[counter] = filterList.get (keyList[counter]);
            counter++;
        }

        Vector vlist = new Vector();
        Enumeration en= _localGraph.getOutgoingEdges (vertexID);
        while (en.hasMoreElements()) {
            boolean control = true;
            FEdge edge = (FEdge) en.nextElement();
            for (int i=0; i<filterList.size(); i++) {
                Object currObject = edge.getAttribute(keyList[i]);
                if (currObject == null || !currObject.equals(objList[i])) {
                    control = false;
                    break;
                }
            }
            if (control) {
                vlist.addElement(edge.getID());
            }
        }
        return (vlist.elements());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#countEdgesBetween(java.lang.String, java.lang.String)
     *
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @return The number of edges between the source and the target vertices.
     */
    public int countEdgesBetween (String sourceID, String targetID)
           throws FGraphException
    {
        return (_localGraph.countEdgesFromTo(sourceID, targetID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgesFromTo(java.lang.String, java.lang.String)
     *
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @return An enumeration of all the vertices between sourceID and targetID
     */
    public Enumeration getEdgesFromTo (String sourceID, String targetID)
           throws FGraphException
    {
        return (_localGraph.getEdgesFromTo(sourceID, targetID));

    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgesFromTo(java.lang.String, java.lang.String, java.util.Hashtable)
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return enumeration of egdes
     * @throws FGraphException
     */
    public Enumeration getEdgesFromTo(String sourceID, String targetID, Hashtable filterList) throws FGraphException
    {
        if (filterList == null) {
            return (getEdgesFromTo (sourceID, targetID));
        }
        int counter = 0;
        String[] keyList = new String[filterList.size()];
        Object[] objList = new Object[filterList.size()];
        Enumeration filterKeys = filterList.keys();
        while (filterKeys.hasMoreElements()) {
            keyList[counter] = (String) filterKeys.nextElement();
            objList[counter] = filterList.get (keyList[counter]);
            counter++;
        }

        Vector vlist = new Vector();
        Enumeration en= _localGraph.getEdgesFromTo (sourceID, targetID);
        while (en.hasMoreElements()) {
            boolean control = true;
            FEdge edge = (FEdge) en.nextElement();
            for (int i=0; i<filterList.size(); i++) {
                Object currObject = edge.getAttribute(keyList[i]);
                if (currObject == null || !currObject.equals(objList[i])) {
                    control = false;
                    break;
                }
            }
            if (control) {
                vlist.addElement(edge.getID());
            }
        }
        return (vlist.elements());
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgesFromTo(java.lang.String, java.lang.String)
     *
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @return An enumeration of all the vertices between sourceID and targetID
     */
    public Enumeration getEdgeIDsFromTo (String sourceID, String targetID) throws FGraphException
    {
      return (_localGraph.getEdgeIDsFromTo(sourceID, targetID));
    }

    /**
     * Handles debug messages. This method can be modified used to
     * easily hide debug messages or to redirect them to a log file.
     * @param msg
     */
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println (msg);
        }
    }

    public String toString()
    {
        return (_localGraph.toString());
    }

    private boolean _debug = false;
    private FGraphInfo _localGraph = null;
}



