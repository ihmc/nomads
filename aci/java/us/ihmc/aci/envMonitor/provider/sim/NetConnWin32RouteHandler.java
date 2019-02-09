package us.ihmc.aci.envMonitor.provider.sim;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.AttributeList;
import us.ihmc.ds.fgraph.FGraphClient;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.nomads.spring.SpringManager;
import us.ihmc.nomads.spring.Spring;
import us.ihmc.nomads.EnvInfoService;

import java.net.URI;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Random;

/**
 * NetConnectivityMonitor
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Jul 23, 2004 at 12:37:36 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class NetConnWin32RouteHandler extends AsyncProvider implements FGraphEventListener
{
    public void init()
            throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cloader.getProperty("gss.server.default.host");
        int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
        if (gssServerHost == null) {
            throw new Exception ("Uknown gssServer host - Faild to locate property (gss.server.default.host)");
        }
        _edgeFilter = new Hashtable();
        _edgeFilter.put(AttributeList.EDGE_TYPE, "REACHABILITY");
        _envFilter = new Hashtable();
        _envFilter.put(AttributeList.NODE_TYPE, "ENVIRONMENT");
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));
    }

    public void run()
    {
        try {
            _envName = _environmentalMonitor.getNodeName();
            randomizeInitialPosition();
            _fgraph.addGraphEventListener(this);
            recalculateEdges();
            while (_running) {
                synchronized (this) {
                    wait();
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

    public void vertexAdded(String vertexID)
    {
        recalculateEdges();
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        if (vertexID.compareTo(_envName)==0) {
            recalculateEdges();
        }
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        if (vertexID.compareTo(_envName)==0) {
            recalculateEdges();
        }
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        if (vertexID.compareTo(_envName)==0) {
            recalculateEdges();
        }
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        //do nothing
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        try {
            if (((String)_fgraph.getEdgeAttribute(edgeID, AttributeList.EDGE_TYPE)).equals("REACHABILITY")) {
                if (sourceID.equals(_envName)) {
                    String targetURI = (String) _fgraph.getVertexAttribute(destID,AttributeList.ENVIRONMENT_URI);
                    String targetIP = (new URI(targetURI)).getHost();
                    System.out.println("ADDING ROUTE TO (" + targetIP + ")");
                    addOSRoute (targetIP);
                }
                if (destID.equals(_envName)) {
                    String sourceURI = (String) _fgraph.getVertexAttribute(sourceID,AttributeList.ENVIRONMENT_URI);
                    String sourceIP = (new URI(sourceURI)).getHost();
                    System.out.println("ADDING ROUTE TO (" + sourceIP + ")");
                    addOSRoute (sourceIP);
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        try {
            if (((String)attributes.get(AttributeList.EDGE_TYPE)).equals("REACHABILITY")) {
                if (sourceID.equals(_envName)) {
                    String targetURI = (String) _fgraph.getVertexAttribute(destID,AttributeList.ENVIRONMENT_URI);
                    String targetIP = (new URI(targetURI)).getHost();
                    System.out.println("REMOVING ROUTE TO (" + targetIP + ")");
                    removeOSRoute (targetIP);
                }
                if (destID.equals(_envName)) {
                    String sourceURI = (String) _fgraph.getVertexAttribute(sourceID,AttributeList.ENVIRONMENT_URI);
                    String sourceIP = (new URI(sourceURI)).getHost();
                    System.out.println("REMOVING ROUTE TO (" + sourceIP + ")");
                    removeOSRoute (sourceIP);
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        //do nothing
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        //do nothing
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        //do nothing
    }

    public void connectionLost()
    {
        //do nothing
    }

    public void connected()
    {
        //do nothing
    }

    public double getMaxReconnDistance ()
    {
        return (_maxConnDistance);
    }

    //----------------------------------------------------------------
    private void randomizeInitialPosition()
    {
        try {
            Hashtable hAtt = new Hashtable();
            Random rand = new Random();
            Double dx1 = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_XPOS);
            if (dx1 == null) {
                dx1 = new Double (rand.nextDouble() * 1000);
                hAtt.put(AttributeList.NODE_XPOS, dx1);
            }
            Double dy1 = (Double) _fgraph.getVertexAttribute(_envName, AttributeList.NODE_YPOS);
            if (dy1 == null) {
                dy1 = new Double (rand.nextDouble() * 1000);
                hAtt.put(AttributeList.NODE_YPOS, dy1);
            }
            hAtt.put(AttributeList.NODE_ZPOS, new Double(0));
            _fgraph.setVertexAttributeList(_envName, hAtt);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    private synchronized void recalculateEdges()
    {
        try {
            Hashtable hfilter = new Hashtable();
            hfilter.put(AttributeList.NODE_TYPE,  "ENVIRONMENT");
            Enumeration vAlist = _fgraph.getVertices(hfilter);
            debugMsg (_envName + " - Recalculating");
            while (vAlist.hasMoreElements()) {
                String snode = (String) vAlist.nextElement();
                if (snode!=null && snode.compareTo(_envName) != 0) {
                    debugMsg ("\tComparing with (" + snode + ")");
                    double distance = distanceBetween(_envName, snode);
                    String edgeInPlace = getReachabilityEdgeBetween(_envName, snode);
                    if (distance <= _maxConnDistance) {
                        if (edgeInPlace == null) {
                            try {
                                createConnEdgeBetween (_envName, snode);
                            }
                            catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    }
                    else {
                        if (edgeInPlace != null) {
                            try {
                                removeConnEdgeBetween (_envName, snode);
                            }
                            catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * NOTE THAT THIS METHODS WILL REMOVE THE REACHIBILITY EDGES IF IT FINDS MORE THAN ONE
     */
    private String getReachabilityEdgeBetween (String src, String dest)
    {
        String edgeID = null;
        try {
            Enumeration en = _fgraph.getEdgesFromTo(src, dest, _edgeFilter);
            while (en.hasMoreElements()) {
                String auxEdgeID = (String) en.nextElement();
                if (edgeID == null) {
                    edgeID = auxEdgeID;
                }
                else {
                    debugMsg ("WARNING - Found more than one Reachability edge - Removing (" + auxEdgeID + ")");
                    _fgraph.removeEdge(auxEdgeID);
                }
            }
            return (edgeID);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private double distanceBetween (String src, String dest)
    {
        double distance = 0;
        try {
            Double dx1 = (Double) _fgraph.getVertexAttribute(src, AttributeList.NODE_XPOS);
            Double dy1 = (Double) _fgraph.getVertexAttribute(src, AttributeList.NODE_YPOS);
            Double dx2 = (Double) _fgraph.getVertexAttribute(dest, AttributeList.NODE_XPOS);
            Double dy2 = (Double) _fgraph.getVertexAttribute(dest, AttributeList.NODE_YPOS);
            if (dx1 != null && dx2 != null && dy1!= null && dy2!=null) {
                double dx = Math.abs(dx1.doubleValue() - dx2.doubleValue());
                double dy = Math.abs(dy1.doubleValue() - dy2.doubleValue());
                distance = Math.sqrt(dx * dx + dy * dy);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return distance;
    }

    private void createConnEdgeBetween (String src, String dest)
    {
        String edgeName = src + ":" + dest;
        try {
            Hashtable hAtt = new Hashtable();
            hAtt.put(AttributeList.EDGE_CAPACITY, new Double(54.0));
            hAtt.put(AttributeList.EDGE_TYPE, "REACHABILITY");
            debugMsg ("Adding Undirected FGInfoEdge (" + edgeName + ")");
            _fgraph.addUndirectedEdge(edgeName, src, dest, hAtt);
        }
        catch (Exception e) {
            debugMsg ("Warning: " + e.getMessage());
            //e.printStackTrace();
        }
    }

    private void removeConnEdgeBetween (String src, String dest)
    {
        try {
            Enumeration en = _fgraph.getEdgesFromTo(src, dest, _edgeFilter);
            while (en.hasMoreElements()) {
                String edgeID = (String) en.nextElement();
                try {
                    debugMsg ("Adding FGInfoEdge (" + edgeID + ")");
                    _fgraph.removeEdge(edgeID);
                }
                catch (Exception e) {
                    debugMsg ("Warning: " + e.getMessage());
                }
            }
        }
        catch (Exception e) {
            debugMsg ("Warning: " + e.getMessage());
        }
    }

    private void addOSRoute (String destIP)
    {
        try {
            //to add the route back, just remove the reference to NOWHERE
            String command = "route delete " + destIP;
            System.out.println("\tadding route to " + destIP);
            Runtime.getRuntime().exec(command);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void removeOSRoute (String destIP)
    {
        try {
            //removing a route, actually results in adding a route to NOWHERE
            String command = "route add " + destIP + " mask 255.255.255.255 " + _blackhole;
            System.out.println("\tremoting route to " + destIP);
            Runtime.getRuntime().exec(command);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[NetConMon] " + msg);
        }
    }

    private String _blackhole = "10.2.32.10";
    private boolean _debug = false;
    private Hashtable _edgeFilter;
    private Hashtable _envFilter;
    private FGraph _fgraph;
    private double _maxConnDistance = 400;
    private boolean _running = true;
    private String _envName;
}

