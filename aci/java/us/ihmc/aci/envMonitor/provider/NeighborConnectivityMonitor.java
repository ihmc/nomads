/**
 * NeighborConnectivityMonitor
 *
 * @author      Marco Arguedas  <marguedas@ihmc.us>
 * 
 * @version     $Revision$ 
 *              $Date$
 */

package us.ihmc.aci.envMonitor.provider;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.AttributeList;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;

import java.net.URI;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Random;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.InetAddress;

/**
 * 
 */
public class NeighborConnectivityMonitor extends AsyncProvider implements FGraphEventListener
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


        BeaconServer bServer = new BeaconServer();
        bServer.start();

        _beaconSocket = new DatagramSocket(SERVERPORT + 1);
    }

    public void run()
    {
        try {
            _envName = _environmentalMonitor.getNodeName();
            randomizeInitialPosition();
            _fgraph.addGraphEventListener(this);
            advertise();
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
        if (vertexID.compareTo(_envName)==0) {
            advertise();
        }
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        if (vertexID.compareTo(_envName)==0) {
            advertise();
        }
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        if (vertexID.compareTo(_envName)==0) {
            advertise();
        }
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        if (vertexID.compareTo(_envName)==0) {
            advertise();
        }
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        //do nothing
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        //do nothing
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        //do nothing
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

    private synchronized void advertise()
    {
        try {
            Hashtable hfilter = new Hashtable();
            hfilter.put(AttributeList.NODE_TYPE,  "ENVIRONMENT");
            
            Enumeration enVertices = _fgraph.getVertices(hfilter);
            debugMsg (_envName + " - Recalculating");
            
            byte[] packet = _envName.getBytes();
            DatagramPacket advPacket = new DatagramPacket(packet, packet.length);
            
            advPacket.setPort(SERVERPORT);
            
            //getVertexAttribute (String vertexID, String attributeKey)
            while (enVertices.hasMoreElements()) {
                String nodeID = (String) enVertices.nextElement();
                if ( (nodeID != null) && !nodeID.equals(_envName) ) {
                    debugMsg ("\nadvertising to (" + nodeID + ")");
                    
                    String strURI = (String) _fgraph.getVertexAttribute(nodeID, AttributeList.ENVIRONMENT_URI);
                    try {
                        URI uri = new URI(strURI);
                        String host = uri.getHost();
                        InetAddress ina = InetAddress.getByName(host);
                        
                        advPacket.setAddress(ina);
                        _beaconSocket.send(advPacket);
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
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

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[NeighConnMon] " + msg);
        }
    }
    
    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    
    /**
     * NeighborListMonitor.BeaconServer interior class.
     */
    public class BeaconServer extends Thread
    {
        public BeaconServer() throws Exception
        {
            _dpacket = new DatagramPacket(new byte[1024], 1024);
            _dsockRcv = new DatagramSocket(SERVERPORT);
            _htAtts = new Hashtable();
            _htAtts.put(AttributeList.EDGE_TYPE, "REACHABILITY");
        }

        public void run()
        {
            while (_serverRunning) {
                try {
                    _dsockRcv.receive(_dpacket);
                    _exceptionCount = 0;
                    System.out.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
                    String msg = new String(_dpacket.getData());
                    debugMsg("BeaconServer :: got Packet [" + msg + "] ");
                    msg = msg.substring(0, _dpacket.getLength()).trim();
                    if ( !msg.equals(_envName) ) {
                        String edgeID = computeEdgeName(msg, _envName);
                        if ( !_fgraph.hasEdge(edgeID) ) {
                            _fgraph.addUndirectedEdge(edgeID, msg, _envName, _htAtts);
                        }
                    }
                }
                catch (Exception e) {
                    _exceptionCount++;
                    System.out.println("Exception: " + e.getMessage());
                    if (_exceptionCount > 10) {
                        System.out.println("Too many exceptions... Terminating BeaconServer");
                        terminate();
                    }
                }
            }
        }

        public void terminate ()
        {
            _serverRunning = false;
            _dsockRcv.close();
        }

        private String computeEdgeName (String node1, String node2)
        {
            String edge = node1 + "#" + node2;
            if (node1.compareTo(node2)>0) {
                edge = node2 + "#" + node1;
            }
            return edge;
        }

        private Hashtable _htAtts;
        private boolean _serverRunning = true;
        private int _exceptionCount = 0;
        private DatagramPacket _dpacket;
        private DatagramSocket _dsockRcv;
    } // class BeaconServer
    
    // /////////////////////////////////////////////////////////////////////////
    // CLASS CONSTANTS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    public static int SERVERPORT = 6357;

    // /////////////////////////////////////////////////////////////////////////
    
    private boolean _debug = false;
    private Hashtable _edgeFilter;
    private Hashtable _envFilter;
    private FGraph _fgraph;
    private double _maxConnDistance = 400;
    private boolean _running = true;
    private String _envName;
    
    private DatagramSocket _beaconSocket;
}

