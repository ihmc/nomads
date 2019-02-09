package us.ihmc.aci.envMonitor.provider;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.AttributeList;
import us.ihmc.util.ConfigLoader;
import us.ihmc.ds.fgraph.FGraph;

import java.net.DatagramPacket;
import java.net.URI;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * NeighborListMonitor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Feb 17, 2005 at 11:55:15 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class NeighborListMonitor extends AsyncProvider
{
    public void init() throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cloader.getProperty("gss.server.default.host");
        int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
        if (gssServerHost == null) {
            throw new Exception ("Uknown gssServer host - Faild to locate property (gss.server.default.host)");
        }
        _running = true;
        _edgeList = new Hashtable();
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));
        _name = _environmentalMonitor.getNodeName();
    
        BeaconServer bServer = new BeaconServer(_edgeList);
        bServer.start();

        byte[] nameArray = _name.getBytes();
        _beaconSocket = new DatagramSocket (NeighborListMonitor.SERVERPORT +1);
        _beaconPacket = new DatagramPacket (nameArray, nameArray.length);
        _beaconPacket.setLength(nameArray.length);
        _beaconPacket.setAddress(InetAddress.getByName("192.168.0.255"));
        _beaconPacket.setPort(NeighborListMonitor.SERVERPORT);
    }

    public void run()
    {
        while (_running) {
            try {
                Thread.sleep(_updateInterval);
                sendBeacon();
                cleanupExpiredEdges();
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void sendBeacon()
    {
        try {
            _beaconSocket.send(_beaconPacket);
            //System.out.println("Sent (" + _beaconPacket.getData().length + ") bytes - " + new String(_beaconPacket.getData()));
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void cleanupExpiredEdges ()
    {
        Enumeration en = _edgeList.keys();
        while (en.hasMoreElements()) {
            String edgeID = (String) en.nextElement();
            long lastUpdate = Long.parseLong((String) _edgeList.get(edgeID));
            if ((System.currentTimeMillis() - lastUpdate) > _expireInterval) {
                try {
                    _fgraph.removeEdge(edgeID);
                    _edgeList.remove(edgeID);
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }


    /**
     * NeighborListMonitor.BeaconServer interior class.
     */
    public class BeaconServer extends Thread
    {
        public BeaconServer(Hashtable edgeList) throws Exception
        {
            _edgeList = edgeList;
            _dpacket = new DatagramPacket(new byte[1024], 1024);
            _dsockRcv = new DatagramSocket(NeighborListMonitor.SERVERPORT);
            _htAtts = new Hashtable();
            _htAtts.put(AttributeList.EDGE_TYPE,"REACHABILITY");
        }

        public void run()
        {
            while (_serverRunning) {
                try {
                    _dsockRcv.receive(_dpacket);
                    _exceptionCount = 0;
                    String msg = new String(_dpacket.getData());
                    msg = msg.substring(0, _dpacket.getLength()).trim();
                    if (msg.compareTo(_name) != 0) {
                        String edgeID = computeEdgeName (msg,_name);
                        if (!_fgraph.hasEdge(edgeID)) {
                            _fgraph.addUndirectedEdge(edgeID, msg, _name, _htAtts);
                        }
                        _edgeList.put(edgeID, System.currentTimeMillis()+"");
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
            String edge = node1+"#"+node2;
            if (node1.compareTo(node2)>0) {
                edge = node2+"#"+node1;
            }
            return edge;
        }


        private Hashtable _htAtts;
        private boolean _serverRunning = true;
        private int _exceptionCount = 0;
        private DatagramPacket _dpacket;
        private DatagramSocket _dsockRcv;
    }


    private FGraph _fgraph;
    private String _name;
    private boolean _running;
    private DatagramSocket _beaconSocket;
    private DatagramPacket _beaconPacket;
    private long _updateInterval = 500;
    private long _expireInterval = 20 * _updateInterval;
    private Hashtable _edgeList;
    public static int SERVERPORT = 6257;
}
