/*
 * FGraphServer.java
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

import us.ihmc.ds.fgraph.message.*;
import us.ihmc.ds.fgraph.comm.MessageListener;
import us.ihmc.ds.fgraph.comm.MessageHandler;
import us.ihmc.ds.fgraph.comm.ServerSocketHandler;
import us.ihmc.ds.fgraph.comm.ServerMocketHandler;

import java.net.URI;
import java.io.Serializable;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

/**
 * FGraph
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 * Created on Apr 21, 2004 at 6:17:12 PM
 * $Date$
 * Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphServer extends FGraph implements MessageListener
{
    /**
     * @see us.ihmc.ds.fgraph.FGraph#getServer()
     */
    protected FGraphServer (int serverPort)
        throws Exception
    {
        initialize (serverPort);
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ADDED);
        fgVtxMessage.setVertexID (vertexID);
        _msgHandler.sendMessage((FGraphMessage) fgVtxMessage);
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ADDED_WITH_ATTRIBUTE_LIST);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObject (attributeList);
        _msgHandler.sendMessage((FGraphMessage) fgVtxMessage);
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_REMOVED);
        fgVtxMessage.setVertexID (vertexID);
        _msgHandler.sendMessage((FGraphMessage) fgVtxMessage);
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_SET);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObjectKey (attributeKey);
        fgVtxMessage.setObject (attribute);
        _msgHandler.sendMessage((FGraphMessage) fgVtxMessage);
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_LIST_SET);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObject (attributeList);
        _msgHandler.sendMessage((FGraphMessage) fgVtxMessage);
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
     * @throws FGraphException
     */
    public void removeVertexAttribute (String vertexID,
                                       String attributeKey)
        throws FGraphException
    {
        debugMsg ("Remove FGInfoVertex Attribute (" + vertexID + " : " + attributeKey + ")");
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        vertex.removeAttribute (attributeKey);
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_REMOVED);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObjectKey (attributeKey);
        _msgHandler.sendMessage((FGraphMessage) fgVtxMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (sourceVertexID);
        fgEdgeMessage.setTargetID (targetVertexID);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (sourceVertexID);
        fgEdgeMessage.setTargetID (targetVertexID);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
    }

    public boolean isEdgeUndirected(String edgeID)
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_REMOVED);
        fgEdgeMessage.setEdgeID (edgeID);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_SET);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObjectKey (attributeKey);
        fgEdgeMessage.setObject (attribute);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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

        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_LIST_SET);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_REMOVED);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObjectKey (attributeKey);
        _msgHandler.sendMessage((FGraphMessage) fgEdgeMessage);
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
            throws FGraphException
    {
        return (_localGraph.hasVertex (vertexID));
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertices()
     *
     * @return Enumeration of vertice IDs
     */
    public Enumeration getVertices ()
            throws FGraphException
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
            throws FGraphException
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
    public Enumeration getEdgesFromTo (String sourceID, String targetID, Hashtable filterList)
        throws FGraphException
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

    public String getOtherEdgeVertex (String edgeID, String oneVertexID)
        throws FGraphException
    {
        throw new FGraphException ("Operation not yet supported");
    }

    //////////////////////////// Listener Registration Methods ////////////////////////////
    /**
     * Register an fginfo event listener to receive updates about changes in remote fginfo structures that this
     * fginfo is connected to, either as one client or as the server.
     *
     * @param listener An object the implements the <code>FGraphEventListener</code> interface
     */
    public void addGraphEventListener (FGraphEventListener listener)
    {
        if (listener != null) {
            if (!_eventListeners.contains (listener)) {
                _eventListeners.addElement (listener);
            }
        }
    }

    /**
     * Removes an event listener from the list
     *
     * @param listener An object that implements the FGraphEventListener Interface.
     */
    public void removeGraphEventListener (FGraphEventListener listener)
    {
        if (listener != null) {
            _eventListeners.removeElement(listener);
        }
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#terminate() 
     */
    public void terminate()
    {
        _msgHandler.close();
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getURI()
     *
     * @return The URI of the FGraph
     */
    public URI getURI ()
            throws FGraphException
    {
        return (_serverURI);
    }

    /**
     * @param uri
     */
    public void setURI (URI uri)
    {
        _serverURI = uri;
    }

    //////////////////////// MessageListener interface methods////////////////////////////////////////////////////
    public void lostConnection (String connHandlerID)
    {
        debugMsg ("Lost Connection with client (" + connHandlerID + ") - Removing associated volatile vertices.");
        removeVolatileVertices (connHandlerID);
        for (int i=0; i<_eventListeners.size(); i++) {
            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
            eventListener.connectionLost();
        }
    }

    public void connected(String connHandlerID)
    {
        for (int i=0; i<_eventListeners.size(); i++) {
            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
            eventListener.connected();
        }
    }

    /**
     * Implementation of the "messageArrived" method in the MessageListener interface.
     * Will receive notificatio every time a new message arrives from a remote structure
     * @param graphMessage
     */
    public void messageArrived (FGraphMessage graphMessage)
    {
        if (graphMessage == null) {
            return;
        }

        String localCommHandler = graphMessage.getLocalConnHandlerID();
        try {
            if (graphMessage instanceof FGVertexMessage) {
                debugMsg("FGraphServer - Message is an instance of FGVertexMessage");
                FGVertexMessage fgVtxMessage = (FGVertexMessage) graphMessage;
                String msgVertexID = fgVtxMessage.getVertexID();
                int actionType = fgVtxMessage.getActionType();
                switch (actionType) {
                    case FGVertexMessage.VERTEX_ADDED:
                        debugMsg("FGraph: Got VERTEX_ADDED (VertexID: " + msgVertexID + ")");
                        if (msgVertexID == null) {
                            String vertexID = _localGraph.addVertex();
                            fgVtxMessage.setVertexID (vertexID);
                        }
                        else {
                            if (!fgVtxMessage.isPersistentModeOn()) {
                                _localGraph.addVertexVolatileWith (msgVertexID, localCommHandler);
                            }
                            else {
                                _localGraph.addVertex (msgVertexID);
                            }
                        }
                        if (!graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgVertexID);
                        }
                        _msgHandler.sendMessage (graphMessage);
                        for (int i=0; i<_eventListeners.size(); i++) {
                            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                            eventListener.vertexAdded(fgVtxMessage.getVertexID());
                        }
                        if (graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgVertexID);
                        }
                        return;
                    case FGVertexMessage.VERTEX_ADDED_WITH_ATTRIBUTE_LIST:
                        debugMsg("FGraph: Got VERTEX_ADDED WITH ATTRIBUTES (VertexID: " + msgVertexID + ")");
                        if (msgVertexID == null) {
                            String vertexID = _localGraph.addVertex();
                            fgVtxMessage.setVertexID (vertexID);
                        }
                        else {
                            if (!fgVtxMessage.isPersistentModeOn()) {
                                _localGraph.addVertexVolatileWith (msgVertexID, localCommHandler);
                            }
                            else {
                                _localGraph.addVertex (msgVertexID);
                            }
                        }
                        Object attListObject = (Object) fgVtxMessage.getObject();
                        FVertex vertex = (FVertex) _localGraph.getVertex (msgVertexID);
                        if (attListObject != null &&
                                attListObject instanceof Hashtable &&
                                vertex != null) {
                            Hashtable attList = (Hashtable) attListObject;
                            Enumeration en= attList.keys();
                            while (en.hasMoreElements()) {
                                String attKey = (String) en.nextElement();
                                vertex.setAttribute (attKey, attList.get (attKey));
                            }
                        }
                        if (!graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgVertexID);
                        }
                        _msgHandler.sendMessage (graphMessage);
                        for (int i=0; i<_eventListeners.size(); i++) {
                            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                            eventListener.vertexAdded(fgVtxMessage.getVertexID());
                        }
                        if (graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgVertexID);
                        }
                        return;
                    case FGVertexMessage.VERTEX_REMOVED:
                        debugMsg("FGraph: Got VERTEX_REMOVED (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        _localGraph.removeVertex (msgVertexID);
                        if (!graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage);
                        }
                        _msgHandler.sendMessage (graphMessage);
                        for (int i=0; i<_eventListeners.size(); i++) {
                            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                            eventListener.vertexRemoved(msgVertexID, vertex.getAllAttributes());
                        }
                        if (graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage);
                        }
                        return;
                    case FGVertexMessage.VERTEX_ATTRIBUTE_SET:
                        debugMsg("FGraph: Got VERTEX_ATTRIBUTE_SET (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        if (vertex != null) {
                            vertex.setAttribute(fgVtxMessage.getObjectKey(),fgVtxMessage.getObject());
                            if (!graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                            _msgHandler.sendMessage (graphMessage);
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.vertexAttribSet ( msgVertexID,
                                                                fgVtxMessage.getObjectKey(),
                                                                fgVtxMessage.getObject());
                            }
                            if (graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                        }
                        else {
                            acknowledgeFailure (localCommHandler, graphMessage,
                                                new Exception ("FGInfoVertex (" + msgVertexID + ") is unknonw"));
                        }
                        return;
                    case FGVertexMessage.VERTEX_ATTRIBUTE_LIST_SET:
                        debugMsg("FGraph: Got VERTEX_ATTRIBUTE_LIST_SET (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        if (vertex != null) {
                            Hashtable attList = (Hashtable) fgVtxMessage.getObject();
                            if (attList != null) {
                                Enumeration en= attList.keys();
                                while (en.hasMoreElements()) {
                                    String skey = (String) en.nextElement();
                                    vertex.setAttribute(skey,attList.get(skey));
                                }
                                if (!graphMessage.isCommitRequired()) {
                                    acknowledge (localCommHandler, graphMessage);
                                }
                                _msgHandler.sendMessage (graphMessage);
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.vertexAttribListSet (msgVertexID, (Hashtable) fgVtxMessage.getObject());
                                }
                                if (graphMessage.isCommitRequired()) {
                                    acknowledge (localCommHandler, graphMessage);
                                }
                            }
                            else {
                                acknowledgeFailure (localCommHandler, graphMessage,
                                                    new Exception ("AttributeList for edge (" + msgVertexID + ") is null"));
                            }
                        }
                        else {
                            acknowledgeFailure (localCommHandler, graphMessage,
                                                new Exception ("FGInfoEdge (" + msgVertexID + ") is unknonw"));
                        }
                        return;
                    case FGVertexMessage.VERTEX_ATTRIBUTE_REMOVED:
                        debugMsg("FGraph: Got VERTEX_ATTRIBUTE_REMOVED (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        if (vertex != null) {
                            vertex.removeAttribute(fgVtxMessage.getObjectKey());
                            if (!graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                            _msgHandler.sendMessage (graphMessage);
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.vertexAttribRemoved(msgVertexID,
                                                                  fgVtxMessage.getObjectKey());
                            }
                            if (graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                        }
                        else {
                            acknowledgeFailure (localCommHandler, graphMessage,
                                                new Exception ("FGInfoVertex (" + msgVertexID + ") is unknonw"));
                        }
                        return;
                }
            }
            else if (graphMessage instanceof FGEdgeMessage) {
                debugMsg("FGraphServer - Message is an instance of FGEdgeMessage");
                FGEdgeMessage fgEdgeMessage = (FGEdgeMessage) graphMessage;
                String msgEdgeID = fgEdgeMessage.getEdgeID();
                int actionType = fgEdgeMessage.getActionType();
                switch (actionType) {
                    case FGEdgeMessage.EDGE_ADDED:
                        debugMsg("FGraph: Got EDGE_ADDED (EdgeID: " + msgEdgeID + ") - msgID: " + fgEdgeMessage.getMessageID());
                        if (fgEdgeMessage.isEdgeUndirected()) {
                            _localGraph.addUndirectedEdge(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                          fgEdgeMessage.getTargetID());
                        }
                        else {
                            _localGraph.addEdge (msgEdgeID, fgEdgeMessage.getSourceID(), fgEdgeMessage.getTargetID());
                        }
                        if (!graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgEdgeID);
                        }
                        _msgHandler.sendMessage (graphMessage);
                        for (int i=0; i<_eventListeners.size(); i++) {
                            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                            eventListener.edgeAdded(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                    fgEdgeMessage.getTargetID());
                        }
                        if (graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgEdgeID);
                        }
                        return;
                    case FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST:
                        debugMsg("FGraph: Got EDGE_ADDED WITH ATTRIBUTES (EdgeID: " + msgEdgeID + ")");
                        if (msgEdgeID == null) {
                            String edgeID = _localGraph.addEdge (fgEdgeMessage.getSourceID(),
                                                                 fgEdgeMessage.getTargetID());
                            fgEdgeMessage.setEdgeID(edgeID);
                        }
                        else {
                            _localGraph.addEdge(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                fgEdgeMessage.getTargetID());
                        }
                        Object attListObject = (Object) fgEdgeMessage.getObject();
                        FEdge edge = (FEdge) _localGraph.getEdge (msgEdgeID);
                        if (attListObject != null &&
                                attListObject instanceof Hashtable &&
                                edge != null) {
                            Hashtable attList = (Hashtable) attListObject;
                            Enumeration en= attList.keys();
                            while (en.hasMoreElements()) {
                                String attKey = (String) en.nextElement();
                                edge.setAttribute (attKey, attList.get (attKey));
                            }
                        }
                        if (!graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgEdgeID);
                        }
                        _msgHandler.sendMessage (graphMessage);
                        for (int i=0; i<_eventListeners.size(); i++) {
                            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                            eventListener.edgeAdded(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                    fgEdgeMessage.getTargetID());
                        }
                        if (graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage, msgEdgeID);
                        }
                        return;
                    case FGEdgeMessage.EDGE_REMOVED:
                        debugMsg("FGraph: Got EDGE_REMOVED (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        _localGraph.removeEdge (msgEdgeID);
                        if (!graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage);
                        }
                        _msgHandler.sendMessage (graphMessage);
                        for (int i=0; i<_eventListeners.size(); i++) {
                            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                            eventListener.edgeRemoved(msgEdgeID, edge.getSource().getID(),
                                                      edge.getTarget().getID(), edge.getAllAttributes());
                        }
                        if (graphMessage.isCommitRequired()) {
                            acknowledge (localCommHandler, graphMessage);
                        }
                        return;
                    case FGEdgeMessage.EDGE_ATTRIBUTE_SET:
                        debugMsg("FGraph: Got EDGE_ATTRIBUTE_SET (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        if (edge != null) {
                            edge.setAttribute(fgEdgeMessage.getObjectKey(),fgEdgeMessage.getObject());
                            if (!graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                            _msgHandler.sendMessage (graphMessage);
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.edgeAttribSet ( msgEdgeID, fgEdgeMessage.getObjectKey(),
                                                              fgEdgeMessage.getObject());
                            }
                            if (graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                        }
                        else {
                            acknowledgeFailure (localCommHandler, graphMessage,
                                                new Exception ("FGInfoEdge (" + msgEdgeID + ") is unknonw"));
                        }
                        return;
                    case FGEdgeMessage.EDGE_ATTRIBUTE_LIST_SET:
                        debugMsg("FGraph: Got EDGE_ATTRIBUTE_LIST_SET (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        if (edge != null) {
                            Hashtable attList = (Hashtable) fgEdgeMessage.getObject();
                            if (attList != null) {
                                Enumeration en= attList.keys();
                                while (en.hasMoreElements()) {
                                    String skey = (String) en.nextElement();
                                    edge.setAttribute(skey,attList.get(skey));
                                }
                                if (!graphMessage.isCommitRequired()) {
                                    acknowledge (localCommHandler, graphMessage);
                                }
                                _msgHandler.sendMessage (graphMessage);
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.edgeAttribListSet (msgEdgeID, (Hashtable) fgEdgeMessage.getObject());
                                }
                                if (graphMessage.isCommitRequired()) {
                                    acknowledge (localCommHandler, graphMessage);
                                }
                            }
                            else {
                                acknowledgeFailure (localCommHandler, graphMessage,
                                                    new Exception ("AttributeList for edge (" + msgEdgeID + ") is null"));
                            }
                        }
                        else {
                            acknowledgeFailure (localCommHandler, graphMessage,
                                                new Exception ("FGInfoEdge (" + msgEdgeID + ") is unknonw"));
                        }
                        return;
                    case FGEdgeMessage.EDGE_ATTRIBUTE_REMOVED:
                        debugMsg("FGraph: Got EDGE_ATTRIBUTE_REMOVED (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        if (edge != null) {
                            edge.removeAttribute (fgEdgeMessage.getObjectKey());
                            if (!graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                            _msgHandler.sendMessage (graphMessage);
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.edgeAttribRemoved(msgEdgeID,
                                                                fgEdgeMessage.getObjectKey());
                            }
                            if (graphMessage.isCommitRequired()) {
                                acknowledge (localCommHandler, graphMessage);
                            }
                        }
                        else {
                            acknowledgeFailure (localCommHandler, graphMessage,
                                                new Exception ("FGInfoEdge (" + msgEdgeID + ") is unknonw"));
                        }
                        return;
                }
            }
            else if (graphMessage instanceof FGSyncMessage) {
                FGSyncMessage fgSyncMsg = (FGSyncMessage) graphMessage;
                int actionType = fgSyncMsg.getActionType();
                if (actionType == FGSyncMessage.REQUEST_GRAPH_SYNC) {
                    debugMsg("FGraph: Got REQUEST_GRAPTH_COPY");
                    String localCommHandlerID = fgSyncMsg.getLocalConnHandlerID();
                    FGSyncMessage fgsync = new FGSyncMessage (FGSyncMessage.REPLY_GRAPH_SYNC);
                    fgsync.setGraphObject (_localGraph);
                    _msgHandler.sendReplyTo(fgsync, localCommHandlerID);
                }
                acknowledge (localCommHandler, graphMessage);
            }
        }
        catch (Exception e) {
            System.out.println ("\n\nReturning Exception to Client: " + e.getMessage() + "\n\n");
            acknowledgeFailure (localCommHandler, graphMessage, e);
        }
    }

    //////////////////////// Private Methods //////////////////////////////////////////////////////
    private void acknowledge (String localCommHandler, FGraphMessage fgMessage)
            throws FGraphException
    {
        acknowledge (localCommHandler, fgMessage, null);
    }

    private void acknowledge (String localCommHandler, FGraphMessage fgMessage, String message)
            throws FGraphException
    {
        FGACKMessage ackMsg = new FGACKMessage (FGACKMessage.SUCCESS);
        ackMsg.setReferenceMessageID (fgMessage.getMessageID());
        ackMsg.setAckMessage (message);
        _msgHandler.sendReplyTo(ackMsg, localCommHandler);
    }

    private void acknowledgeFailure (String localCommHandler, FGraphMessage fgMessage, Exception e)
    {
        try {
            FGACKMessage ackMsg = new FGACKMessage (FGACKMessage.ERROR);
            ackMsg.setReferenceMessageID (fgMessage.getMessageID());
            ackMsg.setException (e);
            _msgHandler.sendReplyTo(ackMsg, localCommHandler);
        }
        catch (Exception excp) {
            debugMsg ("Unable to send ack-failure. Exception: " + excp.getMessage());
        }
    }

    /**
     * Initializes the fginfo structure and communication facilites for the fginfo.
     * @param portNumber PortNumber to be used when FGraph will work as a server (if zero, FGrpahHandler will
     *                      find a port available to be used).
     * @throws Exception If a socket failure occurs
     */
    private void initialize (int portNumber)
            throws Exception
    {
        _eventListeners = new Vector();
        _localGraph = new FGraphInfo();

        _msgHandler = new MessageHandler (this);
        debugMsg ("SERVER_MODE:");
        ServerSocketHandler ssHandler = new ServerSocketHandler (portNumber, _msgHandler);
        _serverURI = ssHandler.getServerURI();
        System.out.println ("URI: " + _serverURI.toASCIIString());
        ServerMocketHandler ssMocketHandler = new ServerMocketHandler (portNumber, _msgHandler);
        _serverMocketURI = ssMocketHandler.getServerURI();;
        System.out.println ("URI: " + _serverMocketURI.toASCIIString());
        System.out.println();
    }

    /**
     * @see FGraph#getClone()
     * @return
     * @throws FGraphException
     */
    public FGraphLocal getClone ()
            throws FGraphException
    {
        FGraphInfo localGraphClone = (FGraphInfo) _localGraph.clone();
        FGraphLocal fgLocal = new FGraphLocal (localGraphClone);
        return (fgLocal);
    }

    private void removeVolatileVertices (String connHandlerID)
    {
        try {
            Enumeration en= _localGraph.getVertices();
            while (en.hasMoreElements()) {
                FVertex vertex = (FVertex) en.nextElement();
                if (vertex.isVolatileWith(connHandlerID)) {
                    debugMsg ("Lost Connection: Removing FGInfoVertex (" + vertex.getID() + ")");
                    removeVertex(vertex.getID());
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void setAllowDuplicates (boolean allowDuplicates)
    {
        _localGraph.setAllowDuplicates(allowDuplicates);
    }

    public boolean areDuplicatesAllowed()
    {
        return (_localGraph.areDuplicatesAllowed());
    }

    /**
     * Handles debug messages. This method can be modified used to
     * easily hide debug messages or to redirect them to a log file.
     * @param msg
     */
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[FGraphServer] " + msg);
        }
    }

    public String toString()
    {
        return (_localGraph.toString());
    }

    private boolean _debug = false;
    private URI _serverURI = null;
    private URI _serverMocketURI = null;
    private MessageHandler _msgHandler;
    private Vector _eventListeners;
    private FGraphInfo _localGraph = null;
}



