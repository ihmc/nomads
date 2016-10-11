/*
 * FGraphEventListener.java
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

/**
 * FGraphEventListener
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.12 $
 * Created on Apr 21, 2004 at 6:13:46 PM
 * $Date: 2016/06/09 20:02:46 $
 * Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public interface FGraphEventListener
{
    public void vertexAdded (String vertexID);
    public void vertexRemoved (String vertexID, Hashtable attributes);
    public void vertexAttribSet (String vertexID, String attKey, Object attribute);
    public void vertexAttribListSet (String vertexID, Hashtable attributes);
    public void vertexAttribRemoved (String vertexID, String attKey);

    public void edgeAdded (String edgeID, String sourceID, String destID);
    public void edgeRemoved (String edgeID, String sourceID, String destID, Hashtable attributes);
    public void edgeAttribSet (String edgeID, String attKey, Object attribute);
    public void edgeAttribListSet (String edgeID, Hashtable attributes);
    public void edgeAttribRemoved (String edgeID, String attKey);

    public void connectionLost ();
    public void connected ();
}
