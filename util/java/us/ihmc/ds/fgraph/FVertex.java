/*
 * FVertex.java
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
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;

/**
 * FVertex
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.14 $
 * Created on Apr 21, 2004 at 6:13:46 PM
 * $Date: 2016/06/09 20:02:46 $
 * Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FVertex implements Serializable
{
    protected FVertex(String vertexID)
    {
        _id = vertexID;
        _volatileWith = null;
        _incomingEdges = new Vector();
        _outgoingEdges = new Vector();
        _htObjects = new Hashtable();
    }
    
    protected String getID()
    {
        return (_id);
    }

    protected void setAttribute (String key, Object obj)
    {
        _htObjects.put (key, obj);
    }

    protected void setVertexVolatileWith (String connHandlerID)
    {
        _volatileWith = connHandlerID;
    }

    protected boolean isVolatileWith (String connHandlerID)
    {
        if (_volatileWith != null && _volatileWith.compareTo(connHandlerID)==0) {
            return true;
        }
        return false;
    }

    protected Object getAttribute (String key)
    {
        return (_htObjects.get (key));
    }

    protected Object removeAttribute (String key)
    {
        return (_htObjects.remove (key));
    }

    protected Enumeration listAttributeKeys()
    {
        return (_htObjects.keys());
    }

    protected Hashtable getAllAttributes()
    {
        return (_htObjects);
    }

    protected void addIncomingEdge (FEdge edge)
    {
        if (!_incomingEdges.contains(edge)) {
             _incomingEdges.addElement(edge);
        }
    }

    protected void removeIncomingEdge (FEdge edge)
    {
        _incomingEdges.removeElement (edge);
    }

    protected Enumeration getIncomingEdges ()
    {
        return (_incomingEdges.elements());
    }

    protected Enumeration getOutgoingEdges ()
    {
        return (_outgoingEdges.elements());
    }

    protected Vector getConnectedEdges ()
    {
        Vector vlist = new Vector();
        Enumeration en= _incomingEdges.elements();
        while (en.hasMoreElements()) {
            vlist.addElement((FEdge) en.nextElement());
        }
        en= _outgoingEdges.elements();
        while (en.hasMoreElements()) {
            vlist.addElement((FEdge) en.nextElement());
        }
        return (vlist);
    }

    protected boolean hasIncomingEdge (FEdge edge)
    {
        if (_incomingEdges.contains(edge)) {
            return true;
        }
        return false;
    }

    protected boolean hasOutgoingEdge (FEdge edge)
    {
        if (_outgoingEdges.contains(edge)) {
            return true;
        }
        return false;
    }

    protected int getDegreeIn()
    {
        return (_incomingEdges.size());
    }

    protected void addOutgoingEdge (FEdge edge)
    {
        if (!_outgoingEdges.contains(edge)) {
            _outgoingEdges.addElement(edge);
        }
    }

    protected void removeOutgoingEdge (FEdge edge)
    {
        _outgoingEdges.removeElement(edge);
    }

    protected int getDegreeOut ()
    {
        return (_outgoingEdges.size());
    }

    protected void clearIncomingEdges ()
    {
        _incomingEdges.clear();
    }

    protected void clearOutgoingEdges ()
    {
        _outgoingEdges.clear();
    }

    protected void clearAllEdges ()
    {
        _incomingEdges.clear();
        _outgoingEdges.clear();
    }

    private Hashtable _htObjects;
    private Vector _incomingEdges;
    private Vector _outgoingEdges;
    private String _volatileWith;
    private String _id;
}
