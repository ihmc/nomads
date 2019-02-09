package us.ihmc.aci.proxyudp;

import us.ihmc.util.ConfigLoader;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphException;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.aci.AttributeList;

import java.io.IOException;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.URI;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Enumeration;
import java.util.zip.CRC32;

/**
 * UDPProxy
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 7, 2004 at 11:50:41 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class UDPProxy
{
    public UDPProxy (String[] args)
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String shost = cloader.getProperty("fgraph.proxyudp.host");
        int iport = cloader.getPropertyAsInt("fgraph.proxyudp.port", _defaultPort);
        _lastTimeStamp =  System.currentTimeMillis();
        _totPacketsIn = 0;
        _totBytesIn = 0;
        _packetsIn = 0;
        _bytesIn = 0;

        try {
            String gssHost = cloader.getProperty("gss.server.default.host");
            int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
            _fgraph = FGraph.getClient(new URI("tcp://" + gssHost + ":" + gssPort));
            _fgraph.setEcho(false);
            EdgeRemover edgeRemover = new EdgeRemover(_fgraph);
            edgeRemover.start();
            VertexRemover vertexRemover = new VertexRemover(_fgraph);
            vertexRemover.start();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            for (int i=0; i<args.length; i++) {
                if (args[i].equals("-debug")) {
                    _debug = true;
                }
                if (args[i].equals("-port") && i<(args.length-1)) {
                    iport = Integer.parseInt(args[i+1]);
                }
            }
            startServer (iport);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void startServer(int iPort) throws Exception
    {
        byte[] buf = new byte[PACKET_SIZE];
        try {
            debugMsg ("Listening on port (" + iPort + ")");
            DatagramSocket socket = new DatagramSocket(iPort);
            while (_running)
            {
                synchronized (this) {
                    DatagramPacket packet = new DatagramPacket(buf, buf.length);
                    _packetsIn++;
                    _bytesIn = _bytesIn + packet.getLength();
                    socket.receive (packet);
                    debugMsg (System.currentTimeMillis() + " RECEIVED_PACKET " + packet.getLength() + " bytes (" + packet.getAddress().getHostAddress() + ")");
                    InetAddress address = packet.getAddress();
                    int remotePort = packet.getPort();
                    handleDataPacket (packet);
                    if ((System.currentTimeMillis()- _lastTimeStamp) >= 3000) {
                        showStats();
                    }
                }
            }
            System.out.println ("CLOSING SOCKET");
            socket.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void showStats()
    {
        long elapsed = (System.currentTimeMillis() - _lastTimeStamp);
        double packetsps =  1000 * (_packetsIn / elapsed);
        double bytesps =  1000 * (_bytesIn / elapsed);
        packetsps = (Math.floor(packetsps * 1000))/1000;        //3 decimal places
        bytesps = (Math.floor(packetsps * 1000))/1000;        //3 decimal places
        _totPacketsIn = _totPacketsIn + _packetsIn;
        _totBytesIn = _totBytesIn + _bytesIn;

        double avgSize = 0;
        if (_totPacketsIn >0) {
            avgSize = _totBytesIn / _totPacketsIn;

        }
        String msg = "RECEIVED_PACKET_STATS  ("+_totPacketsIn+") Pack/sec: " + packetsps
                        + ", Bytes/sec: " + bytesps;
        _lastTimeStamp = System.currentTimeMillis();
        _packetsIn = 0;
        _bytesIn = 0;
    }


    private void handleDataPacket (DatagramPacket dpacket)
    {
        byte[] payload = new byte[dpacket.getLength()];
        System.arraycopy(dpacket.getData(),0,payload,0,dpacket.getLength());

        String smsg = new String(payload);
        debugMsg ("Received: " + smsg);
        if (smsg != null && smsg.length()>0) {
            smsg = smsg.trim();
            String scmd = (smsg.substring(0,smsg.indexOf(" ")).trim());
            debugMsg ("Command: " + scmd);
            if (scmd.equals("EDGEINFO")) {
                handleEdgeUpdate (smsg);
            }
            else if (scmd.equals("GPSINFO")) {
                handleGPSUpdate (smsg);
            }
            else if (scmd.equals("REGISTER_ENVIRONMENT")) {
                StringTokenizer st = new StringTokenizer (smsg);
                String cmd = st.nextToken();
                String name = st.nextToken();
                verifySourceNodeRegistration (name);
            }
            else if (scmd.equals("DEREGISTER_ENVIRONMENT")) {
                StringTokenizer st = new StringTokenizer (smsg);
                String cmd = st.nextToken();
                String name = st.nextToken();
                try {
                    _fgraph.removeVertex(name);
                }
                catch (Exception e) {
                    debugMsg ("Failed to de-register environment " + name + ": " + e.getMessage());
                }
            }
        }
    }

    //--------------------------------------------------------------
    private void handleEdgeUpdate (String message)
    {
        try {
            String sourceNodeId = null;
            String sequenceNumber = null;
            StringTokenizer st = new StringTokenizer (message,"\n");
            while (st.hasMoreElements()) {
                String sline = st.nextToken();
                StringTokenizer st2 = new StringTokenizer (sline);
                String scommand = st2.nextToken();
                if (scommand.equals("EDGEINFO")) {
                    sourceNodeId = st2.nextToken();
                    if (sourceNodeId.equals("N130")) {
                        debugMsg ("Ignoring updates from N130....");
                        return;
                    }
                    verifySourceNodeRegistration (sourceNodeId);
                    sequenceNumber = st2.nextToken();
                }
                else if (scommand.equals("ADD_EDGE")) {
                    String ndA = st2.nextToken();
                    String ndB = st2.nextToken();
                    if (_fgraph.hasVertex(ndA) && _fgraph.hasVertex(ndB)) {
                        String minhop = st2.nextToken();
                        String cost = st2.nextToken();
                        String quality = st2.nextToken();
                        String db = st2.nextToken();
                        String utilization = st2.nextToken();
                        Long Ltime = new Long(System.currentTimeMillis());
                        Hashtable attributes = new Hashtable();
                        String edgeID = createReachEdgeID (ndA, ndB);
                        attributes.put(AttributeList.EDGE_TYPE, "REACHABILITY");
                        attributes.put(AttributeList.EDGE_BBN_COST, cost);
                        attributes.put(AttributeList.EDGE_BBN_QUALITY, quality);
                        attributes.put(AttributeList.EDGE_BBN_UTILIZATION, utilization);
                        attributes.put(AttributeList.EDGE_BBN_DB, db);
                        attributes.put("UPDATE_TIMESTAMP", Ltime);
                        try {
                            if (_fgraph.hasEdge(edgeID)) {
                                _fgraph.setEdgeAttributeList(edgeID, attributes);
                            }
                            else {
                                _fgraph.addEdge(edgeID, ndA, ndB, attributes);
                            }
                        }
                        catch (Exception e) {
                            debugMsg ("Exception updating edge Info: " + e.getMessage());
                        }
                    }
                }
            }
        }
        catch (Exception e) {
            debugMsg ("Failed to Register Environment: " + e.getMessage());
        }
    }

    //--------------------------------------------------------------
    private void handleGPSUpdate (String message)
    {
        try {
            String sourceNodeId = null;
            String sequenceNumber = null;
            StringTokenizer st = new StringTokenizer (message);
            while (st.hasMoreElements()) {
                String scommand = st.nextToken();
                if (scommand.equals("GPSINFO")) {
                    sourceNodeId = st.nextToken();
                    if (sourceNodeId.equals("N130")) {
                        debugMsg ("Ignoring updates from N130....");
                        return;
                    }
                    verifySourceNodeRegistration (sourceNodeId);
                    String latitude = st.nextToken();
                    String longitude = st.nextToken();
                    String altitude = st.nextToken();
                    String yaw = st.nextToken();
                    String pitch = st.nextToken();
                    String roll = st.nextToken();

                    double screenX = ((Double.parseDouble(latitude) - _gpsLongLeft) / (_gpsLongRight - _gpsLongLeft)) * 1000.0;
                    double screenY = 1000.0 - (1000.0 * (Double.parseDouble(longitude) - _gpsLatBottom) / (_gpsLatTop - _gpsLatBottom)) ;

                    Hashtable attributes = new Hashtable();
                    attributes.put(AttributeList.NODE_GPS_LATITUDE, new Double(latitude));
                    attributes.put(AttributeList.NODE_GPS_LONGITUDE, new Double(longitude));
                    attributes.put(AttributeList.NODE_GPS_ALTITUDE, new Double(altitude));
                    attributes.put(AttributeList.NODE_XPOS,  new Integer ((int) screenX));
                    attributes.put(AttributeList.NODE_YPOS,  new Integer ((int) screenY));
                    attributes.put(AttributeList.NODE_HEADING, new Double(yaw));
                    attributes.put(AttributeList.NODE_PITCH, new Double(pitch));
                    attributes.put(AttributeList.NODE_ROLL, new Double(roll));
                    Long Ltime = new Long(System.currentTimeMillis());
                    attributes.put("UPDATE_TIMESTAMP", Ltime);
                    try {
                        _fgraph.setVertexAttributeList(sourceNodeId, attributes);
                    }
                    catch (FGraphException e) {
                        debugMsg ("GotException: " + e.getMessage());
                    }
                }
            }
        }
        catch (Exception e) {
            debugMsg ("Failed to Register Environment: " + e.getMessage());
        }
    }

    //--------------------------------------------------------------
    private void verifySourceNodeRegistration (String nodeID)
    {
        try {
            if (!_fgraph.hasEdge(nodeID)) {
                register (nodeID);
            }
            _fgraph.setVertexAttribute(nodeID, "UPDATE_TIMESTAMP", new Double(System.currentTimeMillis()));
        }
        catch (Exception e) {
            debugMsg ("Exception when registering/checking (" + nodeID + "): " + e.getMessage());
        }
    }

    private void register (String nodeID)
            throws Exception
    {
        Hashtable atts = new Hashtable();
        atts.put(AttributeList.NODE_TYPE, "ENVIRONMENT");
        _fgraph.addVertex(nodeID, atts);
    }

    private String createReachEdgeID (String srcNode, String destNode)
    {
        CRC32 crc32 = new CRC32();
        crc32.update(srcNode.getBytes());
        long lsrc = crc32.getValue();
        crc32 = new CRC32();
        crc32.update(destNode.getBytes());
        long ldest = crc32.getValue();
        long lsum = lsrc + ldest;
        String edgeID = "E" + lsum;
        return (edgeID);
    }

    public static void main(String[] args)
    {
        UDPProxy udpProxy = new UDPProxy(args);
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[UDPProxy] " + msg);
        }
    }

    public class EdgeRemover extends Thread
    {
        public EdgeRemover (FGraph fgraph)
        {
            _fgraph = fgraph;
            _filter = new Hashtable();
            _filter.put(AttributeList.EDGE_TYPE, "REACHABILITY");
        }

        public void run()
        {
            while (true) {
                try {
                    Thread.sleep(1000);
                    Enumeration en = _fgraph.getEdges(_filter);
                    while (en.hasMoreElements()) {
                        String edgeID = (String) en.nextElement();
                        Long lastUpdate = (Long) _fgraph.getEdgeAttribute(edgeID, "UPDATE_TIMESTAMP");
                        if (lastUpdate != null) {
                            long elapsed = System.currentTimeMillis() - lastUpdate.longValue();
                            if (elapsed > _edgeTimeout) {
                                debugMsg ("%%%%%% Removing edge (" + edgeID + ")");
                                _fgraph.removeEdge(edgeID);
                            }
                        }
                    }
                }
                catch (Exception e) {
                    debugMsg ("EdgeRemover_Got Excepton: " + e.getMessage());
                }
            }
        }

        private long _edgeTimeout = 20000;
        private Hashtable _filter;
        private FGraph _fgraph;
    }

    public class VertexRemover extends Thread
    {
        public VertexRemover (FGraph fgraph)
        {
            _fgraph = fgraph;
            _filter = new Hashtable();
            _filter.put(AttributeList.NODE_TYPE, "ENVIRONMENT");
        }

        public void run()
        {
            while (true) {
                try {
                    Thread.sleep(1000);
                    Enumeration en = _fgraph.getVertices(_filter);
                    while (en.hasMoreElements()) {
                        String vertexID = (String) en.nextElement();
                        Long lastUpdate = (Long) _fgraph.getVertexAttribute(vertexID, "UPDATE_TIMESTAMP");
                        if (lastUpdate != null) {
                            long elapsed = System.currentTimeMillis() - lastUpdate.longValue();
                            debugMsg ("[" + vertexID + "] LastUpdate: " + elapsed);
                            if (elapsed > _vertexTimeout) {
                                debugMsg ("%%%%%% Removing edge (" + vertexID + ")");
                                _fgraph.removeVertex(vertexID);
                            }
                        }
                    }
                }
                catch (Exception e) {
                    debugMsg ("VertexRemover_Got Excepton: " + e.getMessage());
                }
            }
        }

        private long _vertexTimeout = 5000;
        private Hashtable _filter;
        private FGraph _fgraph;
    }

    long _lastTimeStamp;
    long _totPacketsIn;
    long _totBytesIn;
    long _packetsIn;
    long _bytesIn;

    private double _gpsLongLeft = -84.8101895774;
    private double _gpsLongRight = -84.8032660949;
    private double _gpsLatTop = 32.3719197887;
    private double _gpsLatBottom = 32.3674848591;

    private int _defaultPort = 2263;
    private FGraph _fgraph;
    private boolean _running = true;
    private static final int PACKET_SIZE = 65535;
    private boolean _debug = false;
}

