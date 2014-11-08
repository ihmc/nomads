/*
 * FGraph.java
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

package us.ihmc.ds.fgraph;

import java.net.URI;
import java.io.Serializable;
import java.util.Enumeration;
import java.util.Hashtable;

/**
 * FGraph
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.39 $
 *
 *          Created on Apr 21, 2004 at 6:17:12 PM
 *          $Date: 2014/11/07 17:58:06 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public abstract class FGraph
{
    public static FGraph getServer()
        throws Exception
    {
        return ((FGraph) new FGraphServer (0));
    }

    public static FGraph getServer (int portNumber)
        throws Exception
    {
        return ((FGraph) new FGraphServer (portNumber));
    }

    public static FGraph getClient (URI remoteURI)
        throws Exception
    {
        if (remoteURI == null) {
            throw new NullPointerException ("Invalid remote URI");
        }
        //return ((FGraph) new FGraphClient (remoteURI));
        return ((FGraph) FGraphClient.getInstance (remoteURI));
    }

    public static FGraphClientProxy getClientProxy (URI remoteURI, int localServerPort)
        throws Exception
    {
        if (remoteURI == null) {
            throw new NullPointerException ("Invalid remote URI");
        }
        return FGraphClientProxy.getInstance (remoteURI, localServerPort);
    }

    public static FGraph getThinClient (URI remoteURI)
        throws Exception
    {
        if (remoteURI == null) {
            throw new NullPointerException ("Invalid remote URI");
        }
        return ((FGraph) FGraphThinClient.getInstance (remoteURI));
    }

    //////////////////////////// FGInfoGraph Manipulation ////////////////////////////
    /**
     * Adds a new vertex to the fginfo with the specified ID (vertexID). Fgraph clients support only
     * this vertion of the addVertex method. This method requires a UNIQUE as argument.
     *
     * @param vertexID the specified vertexID
     * @throws FGraphException  An exceptino is thrown if the ID is duplicate or if the fginfo is
     *                          unable to add the vertex.
     */
    public abstract void addVertex (String vertexID)
            throws FGraphException;

    /**
     * Adds a new vertex with a set of attributes.
     * The vertex is added with the set of attributes specified by attributeList.
     * The attribute list is a hashtable containing key-value pairs where keys are always Strings and the values
     * are serializable objects.
     *
     * @param vertexID the specified vertexID
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException  An exceptino is thrown if the ID is duplicate or if the fginfo is
     *                          unable to add the vertex.
     */
    public abstract void addVertex (String vertexID, Hashtable attributeList)
            throws FGraphException;

    /**
     * Removes the vertex with the specified vertexID
     *
     * @param vertexID vertex ID
     * @throws FGraphException
     */
    public abstract void removeVertex (String vertexID)
            throws FGraphException;

    /**
     * Sets an attribute to this vertex. Attributes are always in the form (key, value) where (key) is
     * always a string, and (value) is always a Serializable object.
     *
     * @param vertexID the ID of the vertex where the attribute will be set.
     * @param attributeKey the key (a string)
     * @param attribute the attribute (a seriablizable object)
     * @throws FGraphException if the vertexID if not found
     */
    public abstract void setVertexAttribute (String vertexID, String attributeKey, Serializable attribute)
            throws FGraphException;

    /**
     * Sets a list of attributes for a specific vertex. All the attributes are changes at the same time.
     * If any of the attributes are unknown, they are automatically added to as a new attribute to the vertex.
     * An exception will be thrown if the vertex is unknown.
     *
     * @param vertexID  the vertex ID
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException An exception is thrown if the vertexID is unknwon
     */
    public abstract void setVertexAttributeList (String vertexID, Hashtable attributeList)
            throws FGraphException;

    /**
     * Returns an attribute of the vertex specified by vertex ID (vertexID).
     * The attribute is a seriablizabel object.
     *
     * @param vertexID The ID of the vertex
     * @param attributeKey  The key of the attribute
     * @return Object The attribute value
     * @throws FGraphException If vertexID is unknown
     */
     public abstract Object getVertexAttribute (String vertexID, String attributeKey)
            throws FGraphException;

    /**
     * Returns a hashtable containing a set of ALL the attributes in the vertex specified by vertex ID (vertexID).
     * Each entry in the hashtable has a String as key and a serializable object as element.
     *
     * @param vertexID The ID of the vertex
     * @return Hashtable The attribute value
     * @throws FGraphException If vertexID is unknown
     */
     public abstract Hashtable getVertexAttributeList (String vertexID)
            throws FGraphException;

    /**
     * Removes a previously set attribute from a vertex.
     *
     * @param vertexID the ID of the vertex
     * @param attributeKey the key of the attribute
     * @throws FGraphException
     */
    public abstract void removeVertexAttribute (String vertexID, String attributeKey)
            throws FGraphException;

    /**
     * Returs an enumeration with the Keys of all attributes in the
     * vertex specified by vertexID
     *
     * @param vertexID  The ID of the vertex
     * @return An Enumeration containing all the attribute keys
     * @throws FGraphException If the vertexID is unknown
     */
    public abstract Enumeration listVertexAttributes (String vertexID)
            throws FGraphException;

    /**
     * Returns the ID of the vertex that is the source of the provided edge
     * @param edgeID edge ID
     *
     * @return targetID, the ID of the source FGInfoVertex
     * @throws FGraphException
     */
    public abstract String getEdgeSource (String edgeID)
            throws FGraphException;

    /**
     * Returns the ID of the vertex that is the target of the provided edge
     * @param edgeID edge ID
     *
     * @return targetID, the ID of the target FGInfoVertex
     * @throws FGraphException
     */
    public abstract String getEdgeTarget (String edgeID)
            throws FGraphException;

    /**
     * Returns the ID of the other vertex that this edge connects to, given the ID
     * of the first vertex
     * @param edgeID edge ID
     * @param oneVertexID the ID of one of the vertices
     *
     * @return the ID of the other vertex of the edge
     *
     * @throws FGraphException, in case the edge is not found or if the vertex ID
     * specified is not one of the vertices of the edge
     */
    public abstract String getOtherEdgeVertex (String edgeID, String oneVertexID)
            throws FGraphException;

    /**
     * Adds a new edge to the fginfo, between vertices (sourceVertexID, targetVertexID) with the specified ID (edgeID)
     * @param edgeID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param sourceVertexID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param targetVertexID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @throws FGraphException if the edgeID is already in use or if the source or target verticeIDs are unknown.
     */
    public abstract void addEdge (String edgeID, String sourceVertexID, String targetVertexID)
            throws FGraphException;

    /**
     * Adds a new edge with a set of attributes.
     * The edge is added with the set of attributes specified by attributeList.
     * The attribute list is a hashtable containing key-value pairs where keys are always Strings and the values
     * are serializable objects.
     *
     * @param edgeID the specified edgeID
     * @param sourceVertexID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param targetVertexID The ID (String) of the target FGInfoVertex, at the destination of the edge
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException  An exceptino is thrown if the ID is duplicate or if the fginfo is
     *                          unable to add the vertex.
     */
    public abstract void addEdge (String edgeID, String sourceVertexID,
                                  String targetVertexID, Hashtable attributeList)
            throws FGraphException;

    /**
     * Adds a new UNDIRECTED edge to the fginfo, between vertices (sourceVertexID, targetVertexID).
     * The fginfo chooses a name (edgeID) as a unique identifier for this edge and
     * returns that name upon sucessfull termination. The name of the edge is an important reference that must
     * be kept for futher changes in the edge (add/remove or change attributes).
     * @param firstEndPointID The ID (String) of one of the endpoints
     * @param secondEndPointID The ID (String) of one of the endpoints
     * @return vertexID the unique of this VERTEX.
     * @throws FGraphException If the source of the target vertices are unknown
     */
    public abstract String addUndirectedEdge (String firstEndPointID, String secondEndPointID)
            throws FGraphException;

    /**
     * Adds a new UNDIRECTED edge to the fginfo, between vertices (firstEndPointID, secondEndPointID) with the
     * specified ID (edgeID)
     * @param edgeID The ID (String) of the source FGInfoVertex, at the origin of the edge
     * @param firstEndPointID The ID (String) of one of the endpoints
     * @param secondEndPointID The ID (String) of one of the endpoints
     * @throws FGraphException if the edgeID is already in use or if the source or target verticeIDs are unknown.
     */
    public abstract void addUndirectedEdge (String edgeID, String firstEndPointID, String secondEndPointID)
            throws FGraphException;

    /**
     * Adds a new UNDIRECTED edge with a set of attributes.
     * The edge is added with the set of attributes specified by attributeList.
     * The attribute list is a hashtable containing key-value pairs where keys are always Strings and the values
     * are serializable objects.
     *
     * @param edgeID the specified edgeID
     * @param firstEndPointID The ID (String) of one of the endpoints of the edge
     * @param secondEndPointID The ID (String) of one of the endpoints of the edge
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException  An exceptino is thrown if the ID is duplicate or if the fginfo is
     *                          unable to add the vertex.
     */
    public abstract void addUndirectedEdge (String edgeID, String firstEndPointID,
                                  String secondEndPointID, Hashtable attributeList)
            throws FGraphException;


    public abstract boolean isEdgeUndirected (String edgeID)
            throws FGraphException;

    /**
     * Removes the edge with ID (edgeID)
     * @param edgeID The ID of the edge to be removed
     * @throws FGraphException If the edgeID is unknown
     */
    public abstract void removeEdge (String edgeID)
            throws FGraphException;

    /**
     * Sets an attribute to this edge. Attributes are always in the form (key, value) where (key) is
     * always a string. Any key-value pair can be used with the only restrictions that the "keys" must be Strings
     * and the "values" (or attributes) must be serializable.
     *
     * @param edgeID The ID of the edge
     * @param attributeKey  The attribute key (String)
     * @param attribute The attribute (Serializable)
     * @throws FGraphException If the edgeID is unknown
     */
    public abstract void setEdgeAttribute (String edgeID, String attributeKey, Serializable attribute)
            throws FGraphException;

    /**
     * Set a list of attributes for a specific edge. All attributes are changes at the same time.
     * If any of the attributes are unknown, they are automatically added to as a new attribute to the edge.
     * An exception will be thrown if the edge is unknown.
     *
     * @param edgeID  the edge ID
     * @param attributeList A property-value hashtable, All keys in the hashtable must be strings and all
     *                      values in the hashtable must be serializable objects
     * @throws FGraphException An exception is thrown if the edgeID is unknwon
     */
   public abstract void setEdgeAttributeList (String edgeID, Hashtable attributeList)
            throws FGraphException;

    /**
     * Returns an attribute of the edge specified by edge ID (edgeID).
     * The attribute is a seriablizabel object.
     *
     * @param edgeID The ID of the edge
     * @param attributeKey  The key of the attribute
     * @return Object The attribute value
     * @throws FGraphException If edgeID is unknown
     */
    public abstract Object getEdgeAttribute (String edgeID, String attributeKey)
            throws FGraphException;

    /**
     * Returns a hashtable containing a set of ALL the attributes in the edge specified by edge ID (edgeID).
     * Each entry in the hashtable has a String as key and a serializable object as element.
     *
     * @param edgeID The ID of the edge
     * @return Hashtable The attribute value
     * @throws FGraphException If vertexID is unknown
     */
     public abstract Hashtable getEdgeAttributeList (String edgeID)
            throws FGraphException;

    /**
     * Removes the attributed identified by the key (attributeKey) from edge (edgeID)
     * @param edgeID The ID of the edge
     * @param attributeKey  The Key of the attribute to be removed
     * @throws FGraphException If the edge is unknown
     */
    public abstract void removeEdgeAttribute (String edgeID, String attributeKey)
            throws FGraphException;

    /**
     * Returs an enumeration with the Keys of all attributes in the
     * edge specified by edgeID
     *
     * @param edgeID  The ID of the edge
     * @return An Enumeration containing all the attribute keys
     * @throws FGraphException If the edgeID is unknown
     */
    public abstract Enumeration listEdgeAttributes (String edgeID)
            throws FGraphException;

    /**
     * Reports if a vertex (vertexID) exists in the fginfo.
     *
     * @param vertexID
     * @return  true if the vertex is present, and false otherwise
     */
    public abstract boolean hasVertex (String vertexID)
        throws FGraphException;

    /**
     * Returns an enumeration of Strings containing the IDs of all the vertices in the fginfo.
     *
     * @return Enumeration of vertice IDs
     */
    public abstract Enumeration getVertices ()
            throws FGraphException;

    /**
     * Returns an enumeration of Strings containing the IDs of all the vertices in the fginfo that satisfy
     * the constraints provided by the hashable filterList. filterList contains a list of attributes that
     * the element MUST have in order to be selected. This method will return all the vertices in the fginfo
     * that have ALL attributes listed in the hashtable with values mathing those specified in the hashtable.
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return Enumeration of vertice IDs
     */
    public abstract Enumeration getVertices (Hashtable filterList)
            throws FGraphException;

    /**
     * Returns an enumeration of all Strings coantining the IDs of all the edges in the fginfo.
     *
     * @return Enumeration of edge IDs.
     */
    public abstract Enumeration getEdges ()
            throws FGraphException;

    /**
     * Returns an enumeration of all Strings coantining the IDs of all the edges in the fginfo that satisfy
     * the constraints provided by the hashable filterList. filterList contains a list of attributes that
     * the element MUST have in order to be selected. This method will return all the edges in the fginfo
     * that have ALL attributes listed in the hashtable with values mathing those specified in the hashtable.
     *
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return Enumeration of edge IDs.
     */
    public abstract Enumeration getEdges (Hashtable filterList)
            throws FGraphException;

    /**
     * Reports if an edge (edgeID) exists in the fginfo.
     *
     * @param edgeID
     * @return  true if the edge is present, and false otherwise
     */
    public abstract boolean hasEdge (String edgeID)
            throws FGraphException;

    /**
     * Returns the number of incoming edges to vertexID (degree in). It is important to note that
     * multiple edges can exist between the same pair of vertices. This way, the degreeIn of a vertex can only
     * suggest the MAXIMUM number of upstream vertices, not necessarily the actual number of upstream vertices.
     *
     * @param vertexID ID of the vertex
     * @return number of incoming edges
     */
    public abstract int getDegreeIn (String vertexID)
           throws FGraphException;

    /**
     * Returns an enumeration of all the IDs of the incoming edges to the FGInfoVertex.
     * @param vertexID ID of the FGInfoVertex
     * @return Enumeration of the IDs of the Incoming edges
     */
    public abstract Enumeration getIncomingEdges (String vertexID)
           throws FGraphException;

    /**
     * Returns an enumeration of all the IDs of the incoming edges to the FGInfoVertex that comply to the constrains
     * specified by the filterList. Refer to the comments of method getEdges(Hashtable filterList)
     * for more details.
     *
     * @param vertexID ID of the FGInfoVertex
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return Enumeration of the IDs of the Incoming edges
     */
    public abstract Enumeration getIncomingEdges (String vertexID, Hashtable filterList)
           throws FGraphException;

    /**
     * Returns the number of outgoing edges to vertexID (degree out).
     * @param vertexID
     * @return FGraphExceptino
     */
    public abstract int getDegreeOut (String vertexID)
           throws FGraphException;

    /**
     * Returns an enumeration with the IDs of all outgoig edges for vertexID
     * @param vertexID The ID of the vertex
     * @return An enumeration with all the IDs of the outgoing edges
     */
    public abstract Enumeration getOutgoingEdges (String vertexID)
           throws FGraphException;

    /**
     * Returns an enumeration with the IDs of all outgoig edges for vertexID that comply to the constrains
     * specified by the filterList. Refer to the comments of method getEdges(Hashtable filterList)
     * for more details.
     *
     * @param vertexID The ID of the vertex
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return An enumeration with all the IDs of the outgoing edges
     */
    public abstract Enumeration getOutgoingEdges (String vertexID, Hashtable filterList)
           throws FGraphException;

    /**
     * Returns the total number of edges between vertices sourceID an targetID
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @return The number of edges between the source and the target vertices.
     */
    public abstract int countEdgesBetween (String sourceID, String targetID)
           throws FGraphException;

    /**
     * Returns and enumeration with the IDs of all the edges from the vertex sourceID to targetID
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @return An enumeration of all the edges going from sourceID to targetID
     */
    public abstract Enumeration getEdgesFromTo (String sourceID, String targetID)
           throws FGraphException;

    /**
     * Returns and enumeration with the IDs of all the edges from the vertex sourceID to targetID that comply to
     * the constrains specified by the filterList. Refer to the comments of method getEdges(Hashtable filterList)
     * for more details.
     *
     * @param sourceID The ID of the source vertex
     * @param targetID The ID of the target vertex
     * @param filterList A hashtable containing attributes that MUST exist in the element for selection.
     * @return An enumeration of all the edges going from sourceID to targetID
     */
    public abstract Enumeration getEdgesFromTo (String sourceID, String targetID, Hashtable filterList)
           throws FGraphException;

    /**
     * Returns the URI of this fginfo structure. If the structure is working as a client (that is, it is connected to
     * a remote server structure) this method will return the URI of the server where this fginfo is connected to.
     * Conversely, if this fginfo is working as a server, this method will return the URI of the local server, where
     * the class is listening form connections.
     *
     * @return The URI of the FGraph
     */
    public abstract URI getURI ()
            throws FGraphException;


    /**
     * Provides a local copy of the fginfo Structure that is no longer synchronized with the server.
     * IMPORTANT: The clone method will recursively clone all attributes added to each of the fginfo components (edges
     * or vertices) hoewever, only immutable objects are clonned (STRING, DOUBLE, INTEGER, etc.). If other types of
     * obejcts were added to fgraph, the cloned copy will maintain a REFERENCE to the original objects inserted in
     * the syncrhonized copy.  
     *
     * @return
     * @throws FGraphException
     */
    public abstract FGraphLocal getClone ()
            throws FGraphException;

    /**
     * Register an fginfo event listener to receive updates about changes in remote fginfo structures that this
     * fginfo is connected to, either as one client or as the server.
     *
     * @param listener An object the implements the <code>FGraphEventListener</code> interface
     */
    public abstract void addGraphEventListener (FGraphEventListener listener);

    /**
     * Removes an event listener from the list
     *
     * @param listener An object that implements the FGraphEventListener Interface.
     */
    public abstract void removeGraphEventListener (FGraphEventListener listener);

    /**
     * Attemps to terminate all threads associated with this instance of FGraph
     */
    public abstract void terminate();

    /**
     * Sets FGraphClient inBlocking mode (if true). Meaning that all methods that
     * Sets FGraphClient in CommitRequired mode (if true). Meaning that all methods that
     * results in modifications in the fginfo will only return AFTER the modification
     * has been actually performed at the server and propagated to all the clients.
     * The DEFAULT state is commitRequired ON.
     * IMPORTANT: When commitRequired mode is set to ON, the echo mode is automatically set
     * to OFF. However, echo can be explecitely turned back on by calling
     * <code>setEcho(boolean echo)</code> and the blocking mode will remain ON.
     *
     * @param commitRequired sets the client in coom.Req. mode (TRUE) or not (FALSE)
     */
    public void setCommitRequiredMode (boolean commitRequired)
    {
        _commitRequiredMode = commitRequired;
    }
    

    /**
     * Checks if FGraphClient is is commitRequied MOde or not
     *
     * @return true if the client is in commitRequired mode, and false otherwise.
     */

    public boolean isCommitRequired()
    {
        return (_commitRequiredMode);
    }

    /**
     * Sets the client/server in echo mode or not (true of false). When echo is ON,
     * the client will hear back (will call all it's FGraphEventListeners on every change
     * that occurs in the fginfo, including those originated by one of the listeners.
     * When echo is OFF, the listeners of an FGraphClient will NOT be notified about changes
     * in the fginfo that were created by themselves.
     * NOTE: Note that if echo is ON, NONE of the listeners of this particular FGraph client
     * will be notified about updates created by any of the listeners in the same client.
     *
     * @param echo a boolean flag, turning echo ON (TRUE) or OFF (FALSE).
     */
    public void setEcho (boolean echo)
    {
        _echo = echo;
    }


    /**
     * Reports if client has echo set to ON (true) or OFF (false)
     * @return true if echo is ON and false otherwise.
     */
    public boolean isEchoON()
    {
        return (_echo);
    }

    /**
     * Sets FGraph in PersistentVertexMode. This operation, although valid in all Fgraph implementations only
     * makes sense in FGraph Clients. Every vertex added to into FGraph by the client when the flag
     * "persistentVertexMode" is set to TRUE will persist at the server even after the client disconnects or
     * terminates.  Conversely, vertices added when the flag is set to FALSE, will be automatically removed
     * by the server if the client disconnects. The default Mode is "persistentVertexMode" = FALSE.
     *
     * @param pVertexMode
     */
    public void setPersistentVertexMode (boolean pVertexMode)
    {
        _persistentVertexMode = pVertexMode;
    }

    public boolean isPersistentVertexModeOn ()
    {
        return (_persistentVertexMode);
    }


    protected boolean _persistentVertexMode = false;
    protected boolean _commitRequiredMode = true;
    protected boolean _echo = false;
}
