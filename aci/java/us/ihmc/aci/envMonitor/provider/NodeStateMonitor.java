package us.ihmc.aci.envMonitor.provider;

import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.AttributeList;
import us.ihmc.util.ConfigLoader;

import java.util.Hashtable;
import java.util.Enumeration;
import java.net.URI;

/**
 * NodeStateMonitor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Oct 25, 2004 at 4:01:39 PM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class NodeStateMonitor extends AsyncProvider implements FGraphEventListener
{
    public void init() throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cloader.getProperty("gss.server.default.host");
        int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
        _timeout = cloader.getPropertyAsInt("node.active.state.timeout", _timeout);
        if (gssServerHost == null) {
            throw new Exception ("Uknown gssServer host - Faild to locate property (gss.server.default.host)");
        }
        _nodeList = new Hashtable();
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));
    }

    public void run()
    {
        try {
            _envName = _environmentalMonitor.getNodeName();
            _fgraph.addGraphEventListener(this);
            while (_running) {
                synchronized (this) {
                    Thread.sleep(_timeout);
                    checkExpiredVertices();
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void terminate()
    {
        _running = false;
        synchronized (this) {
            _fgraph.terminate();
            notifyAll();
        }
    }

    /////////////////////// Private Methods /////////////////////////////
    private synchronized void checkExpiredVertices ()
    {
        Enumeration en = _nodeList.keys();
        long currTime = System.currentTimeMillis();
        while (en.hasMoreElements()) {
            try {
                String nodeID = (String) en.nextElement();
                long ltime = ((Long) _nodeList.get(nodeID)).longValue();
                if ((currTime - ltime) > _timeout) {
                    _fgraph.setVertexAttribute(nodeID, AttributeList.NODE_STATUS, "UNKNOWN");
                }
                else {
                    _fgraph.setVertexAttribute(nodeID, AttributeList.NODE_STATUS, "ACTIVE");
                }
            }
            catch (Exception e) {
                debugMsg ("Got exception when checking for expiration (" + e.getMessage() + ")");
            }
        }
    }


    /////////////////////// FGraph Event Listener - Interface methods ////////////////////////
    public void vertexAdded(String vertexID)
    {
        _nodeList.put (vertexID, new Long(System.currentTimeMillis()));
        try {
            _fgraph.setVertexAttribute(vertexID, AttributeList.NODE_STATUS, "ACTIVE");
        }
        catch (Exception e) {
            System.out.println("Failed to set ACTIVE status on node (" + vertexID + ") - " + e.getMessage());            
        }
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        _nodeList.remove(vertexID);
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        _nodeList.put (vertexID, new Long(System.currentTimeMillis()));
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        _nodeList.put (vertexID, new Long(System.currentTimeMillis()));
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        _nodeList.put (vertexID, new Long(System.currentTimeMillis()));
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        _nodeList.put (sourceID, new Long(System.currentTimeMillis()));
        _nodeList.put (destID, new Long(System.currentTimeMillis()));
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        _nodeList.put (sourceID, new Long(System.currentTimeMillis()));
        _nodeList.put (destID, new Long(System.currentTimeMillis()));
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
    }

    public void connectionLost()
    {
        debugMsg ("Lost Connectivity with the FGraph Server");
    }

    public void connected()
    {
        debugMsg ("Connected with FGraph Server");
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[NodeStateMonitor] " + msg);
        }
    }

    private String _envName;
    private int _timeout = 2500;       //timeout = 2.5 seconds (2500)
    private boolean _debug = true;
    private boolean _running = false;
    private Hashtable _nodeList;
    private FGraph _fgraph;
}
