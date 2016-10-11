/*
 * FEdge.java
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

import java.util.Hashtable;
import java.io.Serializable;
import java.util.Enumeration;

/**
 * FEdge
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.11 $
 * Created on Apr 21, 2004 at 6:13:46 PM
 * $Date: 2016/06/09 20:02:46 $
 * Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FEdge implements Serializable
{
    FEdge (String sID, FVertex source, FVertex target, boolean undirected)
    {
        _htObjects  = new Hashtable();
        _undirected = undirected;
        _source = source;
        _target = target;
        _sID = sID;
    }

    FEdge (String sID, FVertex source, FVertex target)
    {
        _htObjects  = new Hashtable();
        _undirected = false;
        _source = source;
        _target = target;
        _sID = sID;
    }

    protected String getID()
    {
        return (_sID);
    }

    protected void setAttribute (String key, Object obj)
            throws FGraphException
    {
        _htObjects.put (key, obj);
    }
    
    protected Object getAttribute (String key)
    {
        return (_htObjects.get (key));
    }
    
    protected Object removeAttribute (String key)
    {
        return (_htObjects.remove (key));
    }

    protected FVertex getSource ()
    {
        return (_source);
    }
    
    protected FVertex getTarget ()
    {
        return (_target);
    }

    protected Enumeration listAttributeKeys()
    {
        return (_htObjects.keys());
    }

    protected Hashtable getAllAttributes()
    {
        return (_htObjects);
    }

    protected boolean isUndirected()
    {
        return (_undirected);
    }


    private boolean _undirected;
    private String _sID;
    private FVertex _source;
    private FVertex _target;
    private Hashtable _htObjects;
}
