/*
 * FGraphInfo.java
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

import us.ihmc.ds.fgraph.FGraphException;

import java.util.Hashtable;
import java.util.Enumeration;
import java.io.Serializable;
import java.util.Vector;
/**
 * FGraphInfo
 * Handles and maintains the state of the local fginfo structure
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Apr 21, 2004 at 6:13:46 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphInfo implements Serializable, Cloneable
{
    protected FGraphInfo ()
    {
        _vertexIdCounter = 0;
        _edgeIdCounter = 0;
        _htEdge = new Hashtable();
        _htVertex = new Hashtable();
    }

    /**
     * Creates and adds a new FGInfoVertex to the fginfo.
     * The method automaticaly generates an ID for the vertex and
     * returns it to the user.
     *
     * @return vertexID a string representing the unique ID of the vertex
     */
    protected String addVertex()
    {
        String sID = "V" + _vertexIdCounter++;
        while (_htVertex.containsKey(sID)) {
            sID = "V" + _vertexIdCounter++;
        }
        FVertex v = new FVertex (sID);
        _htVertex.put (sID, v);
        return sID;
    }

    /**
     * Creates a new FGInfoVertex with the spcified ID and adds it to
     * the fginfo.
     * @param vertexID
     * @throws us.ihmc.ds.fgraph.FGraphException
     */
    protected void addVertex (String vertexID)
        throws FGraphException
    {
        if (!_allowDuplicates) {
            if (_htVertex.containsKey(vertexID)) {
                throw new FGraphException ("Duplicate FGInfoVertex ID (" + vertexID + ")");
            }
        }
        _vertexIdCounter++;
        FVertex v = new FVertex (vertexID);
        _htVertex.put (vertexID, v);
    }

    /**
     * Creates a new FGInfoVertex with the spcified ID and adds it to
     * the fginfo. The vertex is marked as volatileWith the associated string (ofthen a connHandlerID)
     * @param vertexID
     * @param commHandler the string to wich this vertex is associated as volatile
     * @throws us.ihmc.ds.fgraph.FGraphException
     */
    protected void addVertexVolatileWith (String vertexID, String commHandler)
        throws FGraphException
    {
        if (!_allowDuplicates) {
            if (_htVertex.containsKey(vertexID)) {
                throw new FGraphException ("Duplicate FGInfoVertex ID (" + vertexID + ")");
            }
        }
        _vertexIdCounter++;
        FVertex v = new FVertex (vertexID);
        v.setVertexVolatileWith(commHandler);
        _htVertex.put (vertexID, v);
    }


    /**
     * Checks if the vertex (vertexID) is present in the current data-structure
     * @param vertexID
     * @return true if present, false otherwise
     */
    protected boolean hasVertex (String vertexID)
    {
        return (_htVertex.containsKey(vertexID));
    }

    /**
     * Adds a new edge to the fginfo without specified the edgeID
     *
     * @param sourceID
     * @param targetID
     * @return the automatically create edgeID
     * @throws FGraphException
     */
    protected synchronized String addEdge (String sourceID, String targetID)
        throws FGraphException
    {
        String sID = "E" + _edgeIdCounter++;
        while (_htEdge.containsKey(sID)) {
            sID = "E" + _edgeIdCounter++;
        }

        FVertex src = (FVertex) _htVertex.get (sourceID);
        FVertex trg = (FVertex) _htVertex.get (targetID);
        if (src == null) {
            throw new FGraphException ("Source FGInfoVertex (" + sourceID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("Destination FGInfoVertex (" + targetID + ") is unknown");
        }

        FEdge edge = new FEdge (sID, src, trg);
        src.addOutgoingEdge(edge);
        trg.addIncomingEdge(edge);
        _htEdge.put (sID, edge);
        return (sID);
    }

    /**
     * Adds a new DIRECTED edge to the grpah data structure.
     *
     * @param edgeID
     * @param sourceID
     * @param targetID
     * @throws FGraphException
     */
    protected synchronized void addEdge (String edgeID, String sourceID, String targetID)
        throws FGraphException
    {
        if (!_allowDuplicates) {
            if (_htEdge.containsKey(edgeID)) {
                throw new FGraphException ("Duplicate FGInfoEdge ID (" + edgeID + ")");
            }
        }
        FVertex src = (FVertex) _htVertex.get (sourceID);
        FVertex trg = (FVertex) _htVertex.get (targetID);
        if (src == null) {
            throw new FGraphException ("Source FGInfoVertex (" + sourceID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("Destination FGInfoVertex (" + targetID + ") is unknown");
        }

        FEdge edge = new FEdge (edgeID, src, trg);
        src.addOutgoingEdge(edge);
        trg.addIncomingEdge(edge);
        _htEdge.put (edgeID, edge);
    }

    /**
     * Checks if the edge specified by (edgeID) exists
     *
     * @param edgeID
     * @return true if exists and false otherwise
     */
    protected boolean hasEdge (String edgeID)
    {
        return (_htEdge.containsKey(edgeID));
    }

    /**
     * Adds a new UNDIRECED adge to the fginfo without specifying the edgeID. Technically this is
     * bidrected edge, that is, two edges get added in this case referring to the same edge object.
     *
     * @param firstEndPointID
     * @param secondEndPointID
     * @return the automatically created ID of the recently added ege
     * @throws FGraphException
     */
    protected synchronized String addUndirectedEdge (String firstEndPointID, String secondEndPointID)
        throws FGraphException
    {
        String sID = "E" + _edgeIdCounter++;
        while (_htEdge.containsKey(sID)) {
            sID = "E" + _edgeIdCounter++;
        }

        FVertex src = (FVertex) _htVertex.get (firstEndPointID);
        FVertex trg = (FVertex) _htVertex.get (secondEndPointID);
        if (src == null) {
            throw new FGraphException ("Source FGInfoVertex (" + firstEndPointID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("Destination FGInfoVertex (" + secondEndPointID + ") is unknown");
        }

        FEdge edge = new FEdge (sID, src, trg, true);
        src.addOutgoingEdge(edge);
        src.addIncomingEdge(edge);
        trg.addOutgoingEdge(edge);
        trg.addIncomingEdge(edge);
        _htEdge.put (sID, edge);
        return (sID);
    }

    /**
     * Adds an UNDIRECTED edge where the edge ID is specified with the endpoints.
     *
     * @param edgeID
     * @param firstEndPointID
     * @param secondEndPointID
     * @throws FGraphException
     */
    protected synchronized void addUndirectedEdge (String edgeID, String firstEndPointID, String secondEndPointID)
        throws FGraphException
    {
        if (!_allowDuplicates) {
            if (_htEdge.containsKey(edgeID)) {
                throw new FGraphException ("Duplicate FGInfoEdge ID (" + edgeID + ")");
            }
        }
        FVertex src = (FVertex) _htVertex.get (firstEndPointID);
        FVertex trg = (FVertex) _htVertex.get (secondEndPointID);
        if (src == null) {
            throw new FGraphException ("first FGInfoVertex (" + firstEndPointID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("Destination FGInfoVertex (" + secondEndPointID + ") is unknown");
        }

        FEdge edge = new FEdge (edgeID, src, trg, true);
        src.addOutgoingEdge(edge);
        src.addIncomingEdge(edge);
        trg.addOutgoingEdge(edge);
        trg.addIncomingEdge(edge);
        _htEdge.put (edgeID, edge);
    }

     /**
       * Removes alls the edges that have the "source" as the source vertex (for directed edges, the default case)  or
       * as the firstEndPoint or secondEdPoints (for undirected edges)
       * @param source
       * @throws FGraphException
      */
    protected synchronized void removeAllEdgesFrom (FVertex source)
        throws FGraphException
    {
        try {
            Enumeration en= this.getOutgoingEdgeIDs (source.getID());
            while (en.hasMoreElements()) {
                removeEdge ((String)en.nextElement());
            }
            source.clearOutgoingEdges();
        }
        catch (Exception e) {
            throw new FGraphException (e);
        }
    }

    /**
       * Removes alls the edges that have the "target" as the target vertex (for directed edges, the default case)  or
       * as the firstEndPoint or secondEdPoints (for undirected edges)
       * @param target
       * @throws FGraphException
      */
     protected synchronized void removeAllEdgesTo (FVertex target)
        throws FGraphException
    {
        try {
            Enumeration en= this.getIncomingEdgeIDs (target.getID());
            while (en.hasMoreElements()) {
                removeEdge ((String)en.nextElement());
            }
            target.clearIncomingEdges();
        }
        catch (Exception e) {
            throw new FGraphException (e);
        }
    }

    /**
     * Returns the ID of the vertex that is the source for edge (edgeID). Note that this method will also work
     * on undirected edges, as for FGraph, undirected edges behave as bi-directed edges. For undirected edges this
     * method will return the ID of the firstEndPoint
     *
     * @param edgeID
     * @return ID of the source vertex (for directed edges) of the ID of the firstEndPoint (for undirected edges)
     * @throws FGraphException
     */
    protected String getEdgeSource (String edgeID)
        throws FGraphException
    {
        FEdge fedge = (FEdge) _htEdge.get(edgeID);
        if (fedge == null) {
            throw new FGraphException ("FGInfoEdge (" + edgeID + ") is unknown");
        }
        return fedge.getSource().getID();
    }

    /**
     * Returns the ID of the vertex that is the target for edge (edgeID). Note that this method will also work
     * on undirected edges, as for FGraph, undirected edges behave as bi-directed edges. For undirected edges this
     * method will return the ID of the secondEndpoint
     *
     * @param edgeID
     * @return ID of the source vertex (for directed edges) of the ID of the firstEndPoint (for undirected edges)
     * @throws FGraphException
     */
    protected String getEdgeTarget (String edgeID)
        throws FGraphException
    {
        FEdge fedge = (FEdge) _htEdge.get(edgeID);
        if (fedge == null) {
            throw new FGraphException ("FGInfoEdge (" + edgeID + ") is unknown");
        }
        return fedge.getTarget().getID();
    }

    /**
     * Returns a refernce to an vertex object, given its ID
     *
     * @param vertexID
     * @return  an vertex objcet
     */
    protected FVertex getVertex (String vertexID)
    {
        return ((FVertex) _htVertex.get(vertexID));
    }

    /**
     * Returns an instance to an edge object given its ID.
     * @param edgeID
     * @return an edge object
     */
    protected FEdge getEdge (String edgeID)
    {
        return ((FEdge) _htEdge.get(edgeID));
    }

    protected boolean isEdgeUndirected (String edgeID)
            throws FGraphException
    {
        FEdge edge = (FEdge) _htEdge.get(edgeID);
        if (edge.isUndirected()) {
            return true;
        }
        return false;
    }

    /**
     * Removing a vertex
     */
    protected FVertex removeVertex (String vertexID)
        throws FGraphException
    {
        FVertex vr = (FVertex) _htVertex.get (vertexID);
        if (vr != null) {
            removeAllEdgesFrom (vr);
            removeAllEdgesTo (vr);
            _htVertex.remove (vertexID);
        }
        return vr;
    }

    /**
     * Counts all the edges, directed (in the direction sourceID -> targetID) and undirected between
     * the specified vertices.
     *
     * @param sourceID
     * @param targetID
     * @return the number of vertices between the two nodes.
     * @throws FGraphException
     */
    protected int countEdgesFromTo (String sourceID, String targetID)
        throws FGraphException
    {
        int count = 0;
        FVertex src = (FVertex) _htVertex.get (sourceID);
        FVertex trg = (FVertex) _htVertex.get (targetID);
        if (src == null) {
            throw new FGraphException ("FGInfoVertex (" + sourceID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("FGInfoVertex (" + targetID + ") is unknown");
        }

        debugMsg ("\tCounting edges between (" + sourceID + ":" + targetID + ")");
        Enumeration en= src.getOutgoingEdges();
        while (en.hasMoreElements()) {
            FEdge edge = (FEdge) en.nextElement();
            if (trg.hasIncomingEdge(edge)) {
                count++;
                debugMsg ("\t\t" + src.getID() + ".out:" + edge.getID() + " NOT "+ trg.getID() + ".in (count:" + count + ")");
            }
            else {
                debugMsg ("\t\t" + src.getID() + ".out:" + edge.getID() + " IS "+ trg.getID() + ".in (count:" + count + ")");
            }
        }
        return (count);
    }

    /**
     * Returns an enumaration of all edges (directed or undirected) between nodes sourceID and targetID.
     *
     * @param sourceID
     * @param targetID
     * @return  an enumbartion of edges
     * @throws FGraphException
     */
    protected Enumeration getEdgesFromTo (String sourceID, String targetID)
        throws FGraphException
    {
        Vector edgeList = new Vector();
        FVertex src = (FVertex) _htVertex.get (sourceID);
        FVertex trg = (FVertex) _htVertex.get (targetID);
        if (src == null) {
            throw new FGraphException ("FGInfoVertex (" + sourceID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("FGInfoVertex (" + targetID + ") is unknown");
        }

        Enumeration en= src.getOutgoingEdges();
        while (en.hasMoreElements()) {
            FEdge edge = (FEdge) en.nextElement();
            if (trg.hasIncomingEdge(edge)) {
                edgeList.addElement(edge);
            }
        }
        return (edgeList.elements());
    }

    /**
     * Returns an enumaration of all edge-IDs (directed or undirected) between nodes sourceID and targetID.
     *
     * @param sourceID
     * @param targetID
     * @return  an enumbartion of edges IDs
     * @throws FGraphException
     */
    protected Enumeration getEdgeIDsFromTo (String sourceID, String targetID)
        throws FGraphException
    {
        Vector edgeList = new Vector();
        FVertex src = (FVertex) _htVertex.get (sourceID);
        FVertex trg = (FVertex) _htVertex.get (targetID);
        if (src == null) {
            throw new FGraphException ("FGInfoVertex (" + sourceID + ") is unknown");
        }
        if (trg == null) {
            throw new FGraphException ("FGInfoVertex (" + targetID + ") is unknown");
        }

        Enumeration en= src.getOutgoingEdges();
        while (en.hasMoreElements()) {
            FEdge edge = (FEdge) en.nextElement();
            if (trg.hasIncomingEdge(edge)) {
                edgeList.addElement(edge.getID());
            }
        }
        return (edgeList.elements());
    }

    /**
     * Returns the number of edges that point to the especified vertex plus the number of undirected
     * edges connected to the vertex. The method returns the number (int) of vertices.
     *
     * @param vertexID
     * @return  number of incoming edges IDs plus the number of undirected edges connected to the vertex
     * @throws FGraphException
     */
    protected int getDegreeIn (String vertexID)
        throws FGraphException
    {
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        return (vtx.getDegreeIn());
    }

    protected Enumeration getIncomingEdges (String vertexID)
        throws FGraphException
    {
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        return (vtx.getIncomingEdges());
    }

    protected Enumeration getIncomingEdgeIDs (String vertexID)
        throws FGraphException
    {
        Vector edgeList = new Vector();
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        Enumeration en= vtx.getIncomingEdges();
        while (en.hasMoreElements()) {
            edgeList.addElement(((FEdge) en.nextElement()).getID());
        }
        return (edgeList.elements());
    }

    protected int getDegreeOut (String vertexID)
        throws FGraphException
    {
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        return (vtx.getDegreeOut());
    }

    /**
     * Returns a vector with all edges connected to this vertex (incoming or outgoing)
     *
     * @param vertexID
     * @return Vector of edges
     * @throws FGraphException
     */
    protected Vector getConnectedEdges (String vertexID)
            throws FGraphException
    {
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        return (vtx.getConnectedEdges());
    }

    /**
     * Returns an enumeration of all edges that are outgoing from vertex vertexID
     *
     * @param vertexID
     * @return  enumeration of outgoing edges
     * @throws FGraphException
     */
    protected Enumeration getOutgoingEdges (String vertexID)
        throws FGraphException
    {
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        return (vtx.getOutgoingEdges());
    }

    protected Enumeration getOutgoingEdgeIDs (String vertexID)
        throws FGraphException
    {
        Vector edgeList = new Vector();
        FVertex vtx = (FVertex) _htVertex.get (vertexID);
        if (vtx == null) {
            throw new FGraphException ("FGInfoVertex (" + vertexID + ") is unknown");
        }
        Enumeration en= vtx.getOutgoingEdges();
        while (en.hasMoreElements()) {
            edgeList.addElement(((FEdge) en.nextElement()).getID());
        }
        return (edgeList.elements());
    }

    protected FEdge removeEdge (String edgeID)
    {
        FEdge edge = (FEdge) _htEdge.get (edgeID);
        FVertex src = (FVertex) edge.getSource();
        FVertex trg = (FVertex) edge.getTarget();
        if (src != null) {
            src.removeOutgoingEdge(edge);
            if (edge.isUndirected()) {
                src.removeIncomingEdge(edge);
            }
        }
        if (trg != null) {
            trg.removeIncomingEdge(edge);
            if (edge.isUndirected()) {
                trg.removeOutgoingEdge(edge);
            }
        }
        _htEdge.remove (edgeID);
        return (edge);
    }

    //Querying fginfo info ..................................
    protected int countEdges ()
    {
        return (_htEdge.size());
    }

    protected Enumeration getEdges ()
    {
        return (_htEdge.elements());
    }

    protected Enumeration getEdgeIDs ()
    {
        return (_htEdge.keys());
    }

    protected Enumeration getVertices ()
    {
        return (_htVertex.elements());
    }

    protected Enumeration getVertexIDs ()
    {
        return (_htVertex.keys());
    }

    protected int countVertices ()
    {
        return (_htVertex.size());
    }

    public String toString ()
    {
        String sinfo = "";
        try {
            sinfo = sinfo + "\n";
            Enumeration en= _htVertex.keys();
            while (en.hasMoreElements()) {
                String vertexID = (String) en.nextElement();
                sinfo = sinfo + "VERTEX: " + vertexID + "\n";
                FVertex vertex =  (FVertex) _htVertex.get(vertexID);
                Enumeration enum2 = vertex.listAttributeKeys();
                while (enum2.hasMoreElements()) {
                    String skey = (String) enum2.nextElement();
                    sinfo = sinfo + "\t[" + skey + "]: " + vertex.getAttribute(skey).toString() + "\n";
                }
            }

            en= _htEdge.keys();
            while (en.hasMoreElements()) {
                FEdge edge = (FEdge) _htEdge.get(en.nextElement());
                FVertex src =  edge.getSource();
                FVertex dst =  edge.getTarget();
                sinfo = "EDGE:   " + sinfo + edge.getID() + " < " + src.getID() + " : " + dst.getID() + " >\n";
                Enumeration enum2 = edge.listAttributeKeys();
                while (enum2.hasMoreElements()) {
                    String skey = (String) enum2.nextElement();
                    sinfo = sinfo + "\t[" + skey + "]: " + edge.getAttribute(skey).toString() + "\n";
                }
            }
            sinfo = sinfo + "\n";
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return (sinfo);
    }

    //[ToDO] This method should be fixed to handle cloneable attributes.
    public Object clone()
    {
        FGraphInfo fgClone = null;
        try {
            synchronize: {
                fgClone = new FGraphInfo();
                Enumeration vertices = getVertices();
                while (vertices.hasMoreElements()) {
                    FVertex vtx = (FVertex) vertices.nextElement();
                    fgClone.addVertex (vtx.getID());
                    FVertex vtxClone = fgClone.getVertex(vtx.getID());
                    Enumeration vtxAttList = vtx.listAttributeKeys();
                    while (vtxAttList.hasMoreElements()) {
                        String attName = (String) vtxAttList.nextElement();
                        Object attObject = vtx.getAttribute(attName);
                        vtxClone.setAttribute(attName, vtx.getAttribute(attName));
                    }
                }
                Enumeration edges = getEdges();
                while (edges.hasMoreElements()) {
                    FEdge edge = (FEdge) edges.nextElement();
                    FVertex src = edge.getSource();
                    FVertex trg = edge.getTarget();
                    if (edge.isUndirected()) {
                        fgClone.addUndirectedEdge (edge.getID(), src.getID(), trg.getID());
                    }
                    else {
                        fgClone.addEdge (edge.getID(), src.getID(), trg.getID());
                    }
                    FEdge edgeClone = fgClone.getEdge (edge.getID());
                    Enumeration edgeAttList = edge.listAttributeKeys();
                    while (edgeAttList.hasMoreElements()) {
                        String attName = (String) edgeAttList.nextElement();
                        edgeClone.setAttribute(attName, (edge.getAttribute(attName)));
                    }
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return fgClone;
    }

    public void setAllowDuplicates (boolean flag)
    {
        _allowDuplicates = flag;
    }

    public boolean areDuplicatesAllowed()
    {
        return (_allowDuplicates);
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println (msg);
        }
    }

    ////////////////// Private Methods ////////////////////
    private boolean _debug = false;
    private Hashtable _htVertex;
    private Hashtable _htEdge;
    private boolean _allowDuplicates = false;
    private int _vertexIdCounter;
    private int _edgeIdCounter;
}
