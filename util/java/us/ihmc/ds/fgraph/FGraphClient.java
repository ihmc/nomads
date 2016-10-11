/*
 * FGraphClient.java
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
import us.ihmc.util.ConfigLoader;
import us.ihmc.mockets.Mocket;

import java.net.InetSocketAddress;
import java.net.URI;
import java.net.Socket;
import java.net.InetAddress;
import java.io.Serializable;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;
import java.util.Random;
import java.util.zip.CRC32;

/**
 * FGraphClient
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.56 $
 * Created on Apr 21, 2004 at 6:17:12 PM
 * $Date: 2016/06/09 20:02:46 $
 * Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphClient extends FGraph implements MessageListener
{
    private FGraphClient (URI remoteURI)
        throws Exception
    {
        _serverURI = remoteURI;
        _eventListeners = new Vector();
        setFGCInternalUUID();
        initialize ();
    }

    public synchronized static FGraphClient getInstance(URI remoteURI)
            throws Exception
    {
        if (_fgraphClient == null) {
            _fgraphClient = new FGraphClient (remoteURI);
        }
        return (_fgraphClient);
    }

    /**
     * Terminates the thread that listens for new messages. This is a blcoking tread so the close method will attempt
     * to break the server socket and catch the IO exception to cleanly terminate the listening thread. There are
     * no guarantees that the server thread will be sucesfuly terminated.
     */
    public void close()
    {
        _msgHandler.close();
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
        debugMsg ("Add FGInfoVertex (" + vertexID + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ADDED);
        fgVtxMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgVtxMessage.setFGCSenderID(_fgcInternalID);
        }
        fgVtxMessage.setPersistentMode(_persistentVertexMode);
        fgVtxMessage.setVertexID (vertexID);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
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
        debugMsg ("Add FGInfoVertex (" + vertexID + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ADDED_WITH_ATTRIBUTE_LIST);
        fgVtxMessage.setCommitRequiredMode(_commitRequiredMode);
        if (fgVtxMessage.isCommitRequired()) {
            debugMsg ("COMMIT IS REQUIRED for MSG (" + fgVtxMessage.getMessageID() + ")");
        }
        if (!_echo) {
            fgVtxMessage.setFGCSenderID(_fgcInternalID);
        }
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObject (attributeList);
        fgVtxMessage.setPersistentMode(_persistentVertexMode);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
        debugMsg ("Returning from addVertex (" + vertexID + ")");
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
        debugMsg ("Remove FGInfoVertex (" + vertexID + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_REMOVED);
        fgVtxMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgVtxMessage.setFGCSenderID(_fgcInternalID);
        }
        fgVtxMessage.setVertexID (vertexID);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setVertexAttribute(java.lang.String, java.lang.String, java.io.Serializable)
     *
     * @param vertexID the ID of the vertex where the attribute will be set.
     * @param attributeKey the key (a string)
     * @param attribute the attribute (a seriablizable object)
     * @throws FGraphException if the vertexID if not found
     */
    public synchronized  void setVertexAttribute (String vertexID,
                                    String attributeKey,
                                    Serializable attribute)
        throws FGraphException
    {
        debugMsg ("Set FGInfoVertex Attribute (" + vertexID + " : " + attributeKey + ")");
        FVertex vertex = (FVertex) _localGraph.getVertex (vertexID);
        if (vertex == null) {
            throw new FGraphException ("Invalid vertex id (" + vertexID + ")");
        }
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_SET);
        fgVtxMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgVtxMessage.setFGCSenderID(_fgcInternalID);
        }
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObjectKey (attributeKey);
        fgVtxMessage.setObject (attribute);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setVertexAttributeList(java.lang.String, java.util.Hashtable)
     *
     * @param vertexID the ID of the vertex where the attribute will be set.
     * @param attributeList List of attributes key-value pair
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_LIST_SET);
        fgVtxMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgVtxMessage.setFGCSenderID(_fgcInternalID);
        }
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObject (attributeList);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
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
        debugMsg ("getVertexAttribute (" + vertexID + " : " + attributeKey + ")");
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_REMOVED);
        fgVtxMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgVtxMessage.setFGCSenderID(_fgcInternalID);
        }
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObjectKey (attributeKey);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#listVertexAttributes(java.lang.String)
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
        //_localGraph.addEdge(edgeID, sourceVertexID, targetVertexID);
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (sourceVertexID);
        fgEdgeMessage.setTargetID (targetVertexID);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addEdge(java.lang.String, java.lang.String, java.lang.String, java.util.Hashtable)
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
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (sourceVertexID);
        fgEdgeMessage.setTargetID (targetVertexID);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    public String addUndirectedEdge(String firstEndPointID, String secondEndPointID)
            throws FGraphException
    {
        throw new FGraphException ("An edge ID must be specified when adding an edge from a client.");
    }

    public void addUndirectedEdge(String edgeID, String firstEndPointID, String secondEndPointID)
            throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    public void addUndirectedEdge(String edgeID, String firstEndPointID,
                                  String secondEndPointID, Hashtable attributeList)
            throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    public boolean isEdgeUndirected(String edgeID) throws FGraphException
    {
        return _localGraph.isEdgeUndirected(edgeID);
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_REMOVED);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
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
        ///edge.setAttribute (attributeKey, attribute);
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_SET);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObjectKey (attributeKey);
        fgEdgeMessage.setObject (attribute);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setVertexAttributeList(java.lang.String, java.util.Hashtable)
     *
     * @param edgeID The ID of the edge
     * @param attributeList The attribute list (Serializable)
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
        ///edge.setAttribute (attributeKey, attribute);
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_LIST_SET);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
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
        //edge.removeAttribute (attributeKey);
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_REMOVED);
        fgEdgeMessage.setCommitRequiredMode(_commitRequiredMode);
        if (!_echo) {
            fgEdgeMessage.setFGCSenderID(_fgcInternalID);
        }
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObjectKey (attributeKey);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
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
     * @return FGraphException
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
     * @return An enumeration of all the edges bfrom sourceID to targetID
     */
    public Enumeration getEdgesFromTo (String sourceID, String targetID)
           throws FGraphException
    {
        return (_localGraph.getEdgeIDsFromTo(sourceID, targetID));
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

    public String getOtherEdgeVertex (String edgeID, String oneVertexID)
        throws FGraphException
    {
        throw new FGraphException ("Operation not yet supported");
    }

    //////////////////////////// Listener Registration Methods ////////////////////////////
    /**
     * Register an fginfo event listener to receive updates about changes in remote fginfo structures that this
     * fginfo is connected to, either as one client or as the server.
     * @param listener An object the implements the <code>FGraphEventListener</code> interface
     */
    public void addGraphEventListener (FGraphEventListener listener)
    {
        debugMsg ("Got Request to add listener (" + listener + ")");
        if (listener != null) {
            if (!_eventListeners.contains (listener)) {
                debugMsg ("Listener (" + listener + ") has been added");
                _eventListeners.addElement (listener);
            }
        }
        debugMsg ("## Number of listeners (" + _eventListeners.size() + ")");
    }

    /**
     * Removes an event listener from the list
     * @param listener An object that implements the FGraphEventListener Interface.
     */
    public void removeGraphEventListener (FGraphEventListener listener)
    {
        if (listener != null) {
            debugMsg ("%%% Got Request to remove listener (" + listener + ")");
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
    {
        return (_serverURI);
    }

    /**
     * Sets the URI of the FGraph. (package protected)
     *
     * @param uri
     */
    public void setURI (URI uri)
    {
        _serverURI = uri;
    }

    //////////////////////// MessageListener Interface Methods ////////////////////////////////////////////////
    public void lostConnection (String connHandlerID)
    {
        debugMsg ("Lost Connection with server");
        try {
            for (int i=0; i<_eventListeners.size(); i++) {
                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                debugMsg ("Notify eventListener[" + i + "] - Lost Connection");
                eventListener.connectionLost();
            }
            initialize();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void connected (String connHandlerID)
    {
        for (int i=0; i<_eventListeners.size(); i++) {
            FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
            debugMsg ("Notify eventListener[" + i + "] - connected (from connected callback)");
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

        try {
            if (graphMessage instanceof FGACKMessage) {
                debugMsg("Message is an instance of FGACKMessage");
                FGACKMessage fgAckMsg = (FGACKMessage) graphMessage;
                if (fgAckMsg.getACKType()==FGACKMessage.ERROR) {
                    System.out.print ("GOT AN EXCEPTION FROM SERVER: ");
                    System.out.println (fgAckMsg.getException().getMessage());
                }
                //_fgACKHandler.acknowledge (fgAckMsg);
            }
            else if (graphMessage instanceof FGVertexMessage) {
                debugMsg("Message is an instance of FGVertexMessage");
                FGVertexMessage fgVtxMessage = (FGVertexMessage) graphMessage;
                String msgVertexID = fgVtxMessage.getVertexID();
                int actionType = fgVtxMessage.getActionType();
                switch (actionType) {
                    case FGVertexMessage.VERTEX_ADDED:
                        debugMsg("Got VERTEX_ADDED (VertexID: " + msgVertexID + ")");
                        if (msgVertexID == null) {
                            String vertexID = _localGraph.addVertex();
                            fgVtxMessage.setVertexID (vertexID);
                        }
                        else {
                            _localGraph.addVertex (msgVertexID);
                        }
                        if (notifyListeners (graphMessage)) {
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.vertexAdded(fgVtxMessage.getVertexID());
                            }
                        }
                        return;
                    case FGVertexMessage.VERTEX_ADDED_WITH_ATTRIBUTE_LIST:
                        debugMsg("Got VERTEX_ADDED_WITH_ATTRIBUTE_LIST (VertexID: " + msgVertexID + ")");
                        if (msgVertexID == null) {
                            String vertexID = _localGraph.addVertex();
                            fgVtxMessage.setVertexID (vertexID);
                        }
                        else {
                            _localGraph.addVertex (msgVertexID);
                        }
                        FVertex vertex = (FVertex) _localGraph.getVertex (msgVertexID);
                        Object obj = (Object) fgVtxMessage.getObject();
                        if (obj != null && obj instanceof Hashtable && vertex != null) {
                            Hashtable attList = (Hashtable) obj;
                            Enumeration en= attList.keys();
                            while (en.hasMoreElements()) {
                                String skey = (String) en.nextElement();
                                vertex.setAttribute (skey, attList.get(skey));
                            }
                        }
                        if (notifyListeners (graphMessage)) {
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.vertexAdded(fgVtxMessage.getVertexID());
                            }
                        }
                        return;
                    case FGVertexMessage.VERTEX_REMOVED:
                        debugMsg("Got VERTEX_REMOVED (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex (msgVertexID);
                        Vector removedEdges = _localGraph.getConnectedEdges(msgVertexID);
                        _localGraph.removeVertex (msgVertexID);
                        if (notifyListeners (graphMessage)) {
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.vertexRemoved(msgVertexID, vertex.getAllAttributes());
                                for (int j=0; j<removedEdges.size(); j++) {
                                    FEdge edge = (FEdge) removedEdges.elementAt(j);
                                    eventListener.edgeRemoved(edge.getID(), edge.getSource().getID(),
                                                              edge.getTarget().getID(), edge.getAllAttributes());
                                }
                            }
                        }
                        return;
                    case FGVertexMessage.VERTEX_ATTRIBUTE_SET:
                        debugMsg("Got VERTEX_ATTRIBUTE_SET (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        if (vertex != null) {
                            vertex.setAttribute(fgVtxMessage.getObjectKey(),fgVtxMessage.getObject());
                            if (notifyListeners (graphMessage)) {
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.vertexAttribSet ( msgVertexID,
                                                                    fgVtxMessage.getObjectKey(),
                                                                    fgVtxMessage.getObject());
                                }
                            }
                        }
                        return;
                    case FGVertexMessage.VERTEX_ATTRIBUTE_LIST_SET:
                        debugMsg("Got VERTEX_ATTRIBUTE_LIST_SET (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        if (vertex != null) {
                            Hashtable attList = (Hashtable) fgVtxMessage.getObject();
                            if (attList != null) {
                                Enumeration en= attList.keys();
                                while (en.hasMoreElements()) {
                                    String skey = (String) en.nextElement();
                                    vertex.setAttribute(skey,attList.get(skey));
                                }
                                if (notifyListeners (graphMessage)) {
                                    for (int i=0; i<_eventListeners.size(); i++) {
                                        FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                        eventListener.vertexAttribListSet (msgVertexID,attList);
                                    }
                                }
                            }
                        }
                        return;
                    case FGVertexMessage.VERTEX_ATTRIBUTE_REMOVED:
                        debugMsg("Got VERTEX_ATTRIBUTE_REMOVED (VertexID: " + msgVertexID + ")");
                        vertex = (FVertex) _localGraph.getVertex(msgVertexID);
                        if (vertex != null) {
                            vertex.removeAttribute(fgVtxMessage.getObjectKey());
                            if (notifyListeners (graphMessage)) {
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.vertexAttribRemoved(msgVertexID,fgVtxMessage.getObjectKey());
                                }
                            }
                        }
                        return;
                }
            }
            else if (graphMessage instanceof FGEdgeMessage) {
                debugMsg("Message is an instance of FGEdgeMessage");
                FGEdgeMessage fgEdgeMessage = (FGEdgeMessage) graphMessage;
                String msgEdgeID = fgEdgeMessage.getEdgeID();
                int actionType = fgEdgeMessage.getActionType();
                switch (actionType) {
                    case FGEdgeMessage.EDGE_ADDED:
                        if (fgEdgeMessage.isEdgeUndirected()) {
                            debugMsg("Got EDGE_ADDED Undirected (EdgeID: " + msgEdgeID + ") [" + fgEdgeMessage.getSourceID() + ":" + fgEdgeMessage.getTargetID() + "]");
                            _localGraph.addUndirectedEdge(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                          fgEdgeMessage.getTargetID());
                        }
                        else {
                            debugMsg("Got EDGE_ADDED (EdgeID: " + msgEdgeID + ") [" + fgEdgeMessage.getSourceID() + ":" + fgEdgeMessage.getTargetID() + "]");
                            _localGraph.addEdge (msgEdgeID, fgEdgeMessage.getSourceID(), fgEdgeMessage.getTargetID());
                        }
                        if (notifyListeners (graphMessage)) {
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.edgeAdded(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                        fgEdgeMessage.getTargetID());
                            }
                        }
                        return;
                    case FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST:
                        debugMsg("Got EDGE_ADDED (EdgeID: " + msgEdgeID + ") [" + fgEdgeMessage.getSourceID() + ":" + fgEdgeMessage.getTargetID() + "]");
                        if (fgEdgeMessage.isEdgeUndirected()) {
                            _localGraph.addUndirectedEdge(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                          fgEdgeMessage.getTargetID());
                        }
                        else {
                            _localGraph.addEdge (msgEdgeID, fgEdgeMessage.getSourceID(), fgEdgeMessage.getTargetID());
                        }
                        FEdge edge = (FEdge) _localGraph.getEdge (msgEdgeID);
                        Object obj = (Object) fgEdgeMessage.getObject();
                        if (obj != null && obj instanceof Hashtable && edge != null) {
                            Hashtable attList = (Hashtable) obj;
                            Enumeration en= attList.keys();
                            while (en.hasMoreElements()) {
                                String skey = (String) en.nextElement();
                                edge.setAttribute (skey, attList.get(skey));
                            }
                        }
                        if (notifyListeners (graphMessage)) {
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.edgeAdded(msgEdgeID, fgEdgeMessage.getSourceID(),
                                                        fgEdgeMessage.getTargetID());
                            }
                        }
                        return;
                    case FGEdgeMessage.EDGE_REMOVED:
                        debugMsg("Got EDGE_REMOVED (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        _localGraph.removeEdge (msgEdgeID);
                        if (notifyListeners (graphMessage)) {
                            for (int i=0; i<_eventListeners.size(); i++) {
                                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                eventListener.edgeRemoved(msgEdgeID, edge.getSource().getID(),
                                                          edge.getTarget().getID(), edge.getAllAttributes());
                            }
                        }
                        return;
                    case FGEdgeMessage.EDGE_ATTRIBUTE_SET:
                        debugMsg("Got EDGE_ATTRIBUTE_ADDED (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        if (edge != null) {
                            edge.setAttribute(fgEdgeMessage.getObjectKey(),fgEdgeMessage.getObject());
                            if (notifyListeners (graphMessage)) {
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.edgeAttribSet ( msgEdgeID, fgEdgeMessage.getObjectKey(),
                                                                  fgEdgeMessage.getObject());
                                }
                            }
                        }
                        return;
                    case FGEdgeMessage.EDGE_ATTRIBUTE_LIST_SET:
                        debugMsg("Got EDGE_ATTRIBUTE_LIST_SET (VertexID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        if (edge != null) {
                            Hashtable attList = (Hashtable) fgEdgeMessage.getObject();
                            if (attList != null) {
                                Enumeration en= attList.keys();
                                while (en.hasMoreElements()) {
                                    String skey = (String) en.nextElement();
                                    edge.setAttribute(skey,attList.get(skey));
                                }
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.edgeAttribListSet (msgEdgeID,attList);
                                }
                            }
                        }
                        return;
                    case FGEdgeMessage.EDGE_ATTRIBUTE_REMOVED:
                        debugMsg("Got EDGE_ATTRIBUTE_REMOVED (EdgeID: " + msgEdgeID + ")");
                        edge = (FEdge) _localGraph.getEdge(msgEdgeID);
                        if (edge != null) {
                            edge.removeAttribute (fgEdgeMessage.getObjectKey());
                            if (notifyListeners (graphMessage)) {
                                for (int i=0; i<_eventListeners.size(); i++) {
                                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                                    eventListener.edgeAttribRemoved(msgEdgeID,
                                                                    fgEdgeMessage.getObjectKey());
                                }
                            }
                        }
                        return;
                }
            }
            else if (graphMessage instanceof FGSyncMessage) {
                FGSyncMessage fgSyncMsg = (FGSyncMessage) graphMessage;
                int actionType = fgSyncMsg.getActionType();
                if (actionType == FGSyncMessage.REPLY_GRAPH_SYNC) {
                    debugMsg("Got REPLY_GRAPH_SYNC");
                    _localGraph = (FGraphInfo) fgSyncMsg.getGraphObject();
                    //debugMsg ("Received complete fginfo:");
                    //debugMsg (_localGraph.toString());
                    _graph_received = true;
                    return;
                }
            }
        }
        catch (Exception e) {
            System.out.println (e.getMessage());
        }
    }

    //////////////////////// Private Methods //////////////////////////////////////////////////////
    /**
     * Initializes the fginfo structure and communication facilites for the fginfo.
     *
     * @throws Exception If a socket failure occurs
     */
    private void initialize ()
            throws Exception
    {
        int connAttempts = 0;
        boolean connected = false;
        _localGraph = new FGraphInfo();

        _msgHandler = new MessageHandler (this);
        debugMsg ("FULL_CLIENT_MODE");
        Exception connException = null;
        //while (connAttempts < 80) {

        boolean usingMockets = false;
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        usingMockets = cloader.hasProperty("fgraph.use.mockets");
        if (System.getProperty("use.mockets")!=null) {
            usingMockets = true;
        }

        while (true) {
            try {
                if (usingMockets) {
                    debugMsg ("Creating a Mocket");
                    Mocket mocket = new Mocket ();
                    debugMsg ("Connecting Mocket to (rupd://" + _serverURI.getHost() + ":"
                              + _serverURI.getPort() + ")");
                    mocket.connect (new InetSocketAddress (_serverURI.getHost(), _serverURI.getPort()));
                    _msgHandler.createConnHandler (mocket);
                }
                else {
                    debugMsg ("Creating Socket connected to (" + _serverURI.toASCIIString() + ")");
                    Socket sock = new Socket (_serverURI.getHost(), _serverURI.getPort());
                    _msgHandler.createConnHandler (sock);
                }
                FGSyncMessage fgSync = new FGSyncMessage (FGSyncMessage.REQUEST_GRAPH_SYNC);
                debugMsg ("Sending message (fgSync " + fgSync.getMessageID());
                _msgHandler.sendMessage(fgSync);
                debugMsg ("Will notify reconnection to (" + _eventListeners.size() + ") listeners");
                for (int i=0; i<_eventListeners.size(); i++) {
                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                    debugMsg ("Notify eventListener[" + i + "] - connected");
                    eventListener.connected();
                }
                connected = true;
                break;
            }
            catch (Exception e) {
                connException = e;
                debugMsg ("Failed to connect (" + e.getMessage() + ")");
                Thread.sleep(1500);
                connAttempts++;
            }
        }
        System.out.println("");

        if (connected) {
            connAttempts = 0;
            _graph_received = false;
            while (connAttempts < 300 && !_graph_received) {
                Thread.sleep(50);
            }
            System.out.println();
        }

        if (!connected) {
            throw connException;
        }
    }

    private boolean notifyListeners (FGraphMessage graphMessage)
    {
        boolean notify = true;
        if (!_echo) {
            String senderFGCID = graphMessage.getFGCSenderID();
            if (senderFGCID != null && senderFGCID.compareTo(_fgcInternalID)==0) {
                notify = false;
            }
        }
        return notify;
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

    private void setFGCInternalUUID ()
    {
        Random rand = new Random();
        String banner = "FGC" + rand.nextLong() + ":" + System.currentTimeMillis();

        CRC32 crc32 = new CRC32();
        crc32.update(banner.getBytes());
        _fgcInternalID = "FGC" + crc32.getValue();
    }

    /**
     * Handles debug messages. This method can be modified used to
     * easily hide debug messages or to redirect them to a log file.
     * @param msg
     */
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[FGraphClient] " + msg);
        }
    }

    public String toString()
    {
        return (_localGraph.toString());
    }

    private static FGraphClient _fgraphClient;
    private String _fgcInternalID;
    private boolean _debug = false;
    private boolean _graph_received = false;
    private URI _serverURI = null;
    private MessageHandler _msgHandler;
    private Vector _eventListeners;
    private FGraphInfo _localGraph = null;

}



