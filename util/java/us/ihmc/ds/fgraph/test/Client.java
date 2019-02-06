/*
 * Client.java
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

package us.ihmc.ds.fgraph.test;

import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.ds.fgraph.FGraph;

import java.util.Hashtable;
import java.net.URI;

/**
 * Client
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Jul 13, 2004 at 11:41:12 PM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class Client implements FGraphEventListener
{
    public Client()
    {
        try {
            _fgraph = FGraph.getClient(new URI("tcp://localhost:7495"));
            _fgraph.addGraphEventListener(this);
            _fgraph.setEcho(true);
            _fgraph.addVertex("V" + _vertexCounter++);
            _fgraph.addVertex("V" + _vertexCounter++);
            System.out.println("done");
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    public static void main (String[] args)
    {
        Client clt = new Client();
    }

    public void vertexAdded(String vertexID)
    {
        System.out.println ("\n[CLIENT]\tReceived: VERTEX_ADDED(" + vertexID + ")");
        if (_vertexCounter < 2) {
            return;
        }

        if (_vertexCounter < 100) {
            String sprev1 = "V" + (_vertexCounter - 2);
            String sprev2 = "V" + (_vertexCounter - 1);
            try {
                String edgeID = "E"+_edgeCounter++;
                System.out.println ("[CLIENT]\tRequesting: ADD_EDGE(" + edgeID + ": ["+ sprev1 + "," + sprev2 + "]");
                _fgraph.addEdge(edgeID, sprev1, sprev2);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        try {
            String svname = "V" + _vertexCounter++;
            System.out.println ("\n[CLIENT]\nReceived: EDGE_ADDED(" + edgeID + ")");
            System.out.println ("[CLIENT]\tRequesting: ADD_VERTEX(" + svname + ")");
            _fgraph.addVertex(svname);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connectionLost()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    private FGraph _fgraph;
    private long _vertexCounter = 0;
    private long _edgeCounter = 0;
}
