/*
 * FGraphThinClient.java
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

import us.ihmc.ds.fgraph.comm.MessageHandler;
import us.ihmc.ds.fgraph.comm.MessageListener;
import us.ihmc.ds.fgraph.message.*;
import us.ihmc.mockets.Mocket;
import us.ihmc.util.ConfigLoader;

import java.net.InetSocketAddress;
import java.net.URI;
import java.net.Socket;
import java.net.InetAddress;
import java.io.Serializable;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

/**
 * FGraphThinClient
 * [TODO] The following improvements are pending on this class.
 *  - Change it from using a TCP socket to a SocketCache class in the (us.ihmc.net) library.
 *  - Implement the fginfo querying method to operate at the server side and get the response from the server
 *    (that's necessary because thin-client does not have a local fginfo for efficient querying). Currently these
 *     methos are throwing an exception)
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.41 $
 *
 * Created on May 17, 2004 at 5:51:36 PM
 * $Date: 2016/06/09 20:02:46 $
 * Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphThinClient extends FGraph implements MessageListener
{
    private FGraphThinClient (URI remoteURI)
            throws Exception
    {
        _serverURI = remoteURI;
        initialize();
    }

    public static FGraphThinClient getInstance(URI remoteURI)
            throws Exception
    {
        if (_fgraphThinClient == null) {
            _fgraphThinClient = new FGraphThinClient(remoteURI);
        }
        return (_fgraphThinClient);

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
     * @throws FGraphException An exceptino is thrown if the ID is duplicate or
     *                          if the fginfo is unable to add the vertex.
     */
    public void addVertex (String vertexID)
        throws FGraphException
    {
        debugMsg ("Add FGInfoVertex (" + vertexID + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ADDED);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setPersistentMode(_persistentVertexMode);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addVertex(java.lang.String, java.util.Hashtable)
     *
     * @param vertexID the specified vertexID
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException An exceptino is thrown if the ID is duplicate or
     *                          if the fginfo is unable to add the vertex.
     */
    public void addVertex (String vertexID, Hashtable attributeList)
        throws FGraphException
    {
        debugMsg ("Add FGInfoVertex (" + vertexID + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ADDED_WITH_ATTRIBUTE_LIST);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObject (attributeList);
        fgVtxMessage.setPersistentMode(_persistentVertexMode);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
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
    public void setVertexAttribute (String vertexID,
                                    String attributeKey,
                                    Serializable attribute)
        throws FGraphException
    {
        debugMsg ("Set FGInfoVertex Attribute (" + vertexID + " : " + attributeKey + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_SET);
        fgVtxMessage.setVertexID (vertexID);
        fgVtxMessage.setObjectKey (attributeKey);
        fgVtxMessage.setObject (attribute);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgVtxMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setVertexAttributeList(java.lang.String, java.util.Hashtable)
     *
     * @param vertexID the ID of the vertex where the attribute will be set.
     * @param attributeList set of attributes (key-value pairs)
     * @throws FGraphException if the vertexID if not found
     */
    public void setVertexAttributeList (String vertexID, Hashtable attributeList)
        throws FGraphException
    {
        debugMsg ("Set FGInfoVertex Attribute_List (" + vertexID + ")");
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_LIST_SET);
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertexAttributeList(java.lang.String)
     *
     * @param vertexID The ID of the vertex
     * @return Hashtable The set of attribtues
     * @throws FGraphException If vertexID is unknown
     */
    public Hashtable getVertexAttributeList (String vertexID)
        throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        FGVertexMessage fgVtxMessage = new FGVertexMessage (FGVertexMessage.VERTEX_ATTRIBUTE_REMOVED);
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#addEdge(java.lang.String, java.lang.String)
     *
     * @param sourceVertexID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param targetVertexID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @return vertexID the unique of this VERTEX.
     * @throws FGraphException If the source of the target vertices are unknown
     */
    //public String addEdge (String sourceVertexID, String targetVertexID)
    //    throws FGraphException
    //{
    //    throw new FGraphException ("An edge ID must be specified when adding an edge from a client.");
    //}

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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
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
    public void addEdge (String edgeID, String sourceVertexID, String targetVertexID, Hashtable attributeList)
        throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (sourceVertexID);
        fgEdgeMessage.setTargetID (targetVertexID);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    public String addUndirectedEdge(String firstEndPointID, String secondEndPointID) throws FGraphException
    {
        throw new FGraphException ("An edge ID must be specified when adding an edge from a client.");
    }

    public void addUndirectedEdge(String edgeID, String firstEndPointID, String secondEndPointID)
            throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    public void addUndirectedEdge(String edgeID, String firstEndPointID, String secondEndPointID,
                                  Hashtable attributeList) throws FGraphException
    {
        debugMsg ("Add FGInfoEdge (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ADDED_WITH_ATTRIBUTE_LIST);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setSourceID (firstEndPointID);
        fgEdgeMessage.setTargetID (secondEndPointID);
        fgEdgeMessage.setUndirectedEdgeFlag(true);
        fgEdgeMessage.setObject (attributeList);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    public boolean isEdgeUndirected(String edgeID)
            throws FGraphException
    {
        throw new FGraphException ("Method unavailable in the ThinClient");
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_SET);
        fgEdgeMessage.setEdgeID (edgeID);
        fgEdgeMessage.setObjectKey (attributeKey);
        fgEdgeMessage.setObject (attribute);
        _msgHandler.sendBlockingMessage ((FGraphMessage) fgEdgeMessage);
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setEdgeAttributeList(java.lang.String, java.util.Hashtable)
     *
     * @param edgeID The ID of the edge
     * @param attributeList  List of attributes (key-value pairs)
     * @throws FGraphException If the edgeID is unknown
     */
    public void setEdgeAttributeList (String edgeID, Hashtable attributeList)
        throws FGraphException
    {
        debugMsg ("Set FGInfoEdge Attribute_List (" + edgeID + ")");
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_LIST_SET);
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgeAttributeList(java.lang.String)
     *
     * @param edgeID The ID of the edge
     * @return Hashtable The set of attribtues
     * @throws FGraphException If edgeID is unknown
     */
    public Hashtable getEdgeAttributeList (String edgeID)
        throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        FGEdgeMessage fgEdgeMessage = new FGEdgeMessage (FGEdgeMessage.EDGE_ATTRIBUTE_REMOVED);
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getVertices()
     *
     * @return Enumeration of vertice IDs
     */
    public Enumeration getVertices ()
            throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    public Enumeration getVertices(Hashtable filterList) throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdges()
     *
     * @return Enumeration of edge IDs.
     */
    public Enumeration getEdges ()
            throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    public Enumeration getEdges(Hashtable filterList) throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#hasEdge(java.lang.String)
     *
     * @param edgeID
     * @return true if edge is present and false otherwise
     * @throws FGraphException
     */
    public boolean hasEdge (String edgeID)
            throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    public Enumeration getIncomingEdges(String vertexID, Hashtable filterList) throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    public Enumeration getOutgoingEdges(String vertexID, Hashtable filterList) throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#getEdgesFromTo(java.lang.String, java.lang.String)
     *
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @return An enumeration of all the edges going from sourceID to targetID
     */
    public Enumeration getEdgesFromTo (String sourceID, String targetID)
           throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    public Enumeration getEdgesFromTo (String sourceID, String targetID, Hashtable filterList)
        throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
    }

    public String getOtherEdgeVertex (String edgeID, String oneVertexID)
        throws FGraphException
    {
        throw new FGraphException ("Operation not supported on a Thin-Client.");
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
     * getURI
     *
     * @return The URI of the FGraph
     */
    public URI getURI ()
    {
        return (_serverURI);
    }

    /**
     * @see FGraph#getClone()
     * @return
     * @throws FGraphException
     */
    public FGraphLocal getClone ()
            throws FGraphException
    {
        throw new FGraphException ("Clonning operation is not support by ThinClients (fgraphThinClient)");
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
    /**
     * Notifies listener that connection with the server has been lost.
     *
     * @param connHandlerID
     */
    public void lostConnection (String connHandlerID)
    {
        debugMsg ("Lost Connection with server");
        try {
            for (int i=0; i<_eventListeners.size(); i++) {
                FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                eventListener.connectionLost();
            }
            initialize();
        }
        catch (Exception e) {
            e.printStackTrace();
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
     *
     * @param graphMessage new FGraphMessage
     */
    public void messageArrived (FGraphMessage graphMessage)
    {
        if (graphMessage == null) {
            return;
        }

        try {
            if (graphMessage instanceof FGACKMessage) {
                debugMsg("FGraphServer - Message is an instance of FGACKMessage");
                FGACKMessage fgAckMsg = (FGACKMessage) graphMessage;
                if (fgAckMsg.getACKType()==FGACKMessage.ERROR) {
                    System.out.println ("GOT AN EXCEPTION FROM SERVER");
                    fgAckMsg.getException().printStackTrace();
                }
                //_fgACKHandler.acknowledge (fgAckMsg);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
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
        _eventListeners = new Vector();

        _msgHandler = new MessageHandler (this);
        debugMsg ("FULL_CLIENT_MODE");
        System.out.println ("Connecting to fgraph server: " + _serverURI.toASCIIString());
        Exception connException = null;
        boolean usingMockets = false;
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        //while (connAttempts < 80) {
        while (true) {
            try {
                usingMockets = cloader.hasProperty("fgraph.use.mockets");
                if (System.getProperty("use.mockets")!=null) {
                    usingMockets = true;
                }

                if (usingMockets) {
                    debugMsg ("Creating a Mocket");
                    Mocket mocket = new Mocket ();
                    debugMsg ("Connecting Mocket to (" + _serverURI.toASCIIString() + ")");
                    mocket.connect(new InetSocketAddress (_serverURI.getHost(), _serverURI.getPort()));
                    _msgHandler.createConnHandler (mocket);
                }
                else {
                    debugMsg ("Creating a Socket connected to (" + _serverURI.toASCIIString() + ")");
                    Socket sock = new Socket (_serverURI.getHost(), _serverURI.getPort());
                    _msgHandler.createConnHandler (sock);
                }
                FGControlMessage fgControl = new FGControlMessage (FGControlMessage.SET_CLIENT_MODE_THIN);
                _msgHandler.sendMessage(fgControl);
                for (int i=0; i<_eventListeners.size(); i++) {
                    FGraphEventListener eventListener = (FGraphEventListener) _eventListeners.elementAt(i);
                    eventListener.connected();
                }
                connected = true;
                break;
            }
            catch (Exception e) {
                connException = e;
                Thread.sleep(250);
                connAttempts++;
            }
        }
        System.out.println();

        if (!connected) {
            throw connException;
        }

    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#setEcho(boolean)
     * @param echo ON (true) or OFF (false)
     */
    public void setEcho (boolean echo)
    {
        _echo = echo;
    }

    /**
     * @see us.ihmc.ds.fgraph.FGraph#isEchoON()
     * @return TRUE if ehco is ON and FALSE otherwise
     */
    public boolean isEchoON ()
    {
        return (_echo);
    }

    /**
     * Handles debug messages. This method can be modified used to
     * easily hide debug messages or to redirect them to a log file.
     *
     * @param msg message to be displayed
     */
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[FGraphThinClient] " + msg);
        }
    }

    private Vector _eventListeners;
    private static FGraphThinClient _fgraphThinClient;
    private MessageHandler _msgHandler;
    private boolean _debug = false;
    private URI _serverURI = null;
}
