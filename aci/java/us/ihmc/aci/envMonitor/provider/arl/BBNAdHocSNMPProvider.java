package us.ihmc.aci.envMonitor.provider.arl;

import java.net.URI;
import java.net.InetAddress;
import java.util.StringTokenizer;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.zip.CRC32;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.envMonitor.EnvironmentalChangeListener;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.util.SNMPWalker;
import us.ihmc.aci.AttributeList;
import us.ihmc.ds.fgraph.FGraph;
import uk.co.westhawk.snmp.stack.varbind;

public class BBNAdHocSNMPProvider extends AsyncProvider /*implements EnvironmentalChangeListener*/
{
    public void init ()
            throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        _pollInterval = cloader.getPropertyAsInt("arl.provider.BBNSNMP.pollInterval",_pollInterval);
        _debug = cloader.hasProperty("arl.provider.BBNSNMP.debug");
        try {
            //_updateGPSPosition = cloader.getPropertyAsBoolean("arl.provider.BBNSNMP.relayGPSupdates");
            //if (_updateGPSPosition) {
            //    System.out.println("[EnvAgent] Will relay gps info from POS to BBN-infrastructure");
            //}
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println("PoolInterval: " + _pollInterval);
        debugMsg ("Debug is ON");
        _membershipFilter = new Hashtable();
        _membershipFilter.put(AttributeList.EDGE_TYPE, "REACHABILITY");

        _nodeGatewayIP = cloader.getProperty("adhoc.gateway.ip");
        String gssServerHost = cloader.getProperty("gss.server.default.host");
        int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
        if (gssServerHost == null) {
            throw new Exception ("Uknown gssServer host - Faild to locate property (gss.server.default.host)");
        }
        debugMsg ("Will attempt to connect to the fgraphServer at (" + gssServerHost + ":" + gssPort + ")");
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));


        getInterfaceAddress();
        //_environmentalMonitor.addChangeListener(this);      //REGISTERING AS AN EVENT LISTENERS AS WELL
        _snmpWalker = new SNMPWalker (_nodeGatewayIP, "robot");
        _fgraph.setVertexAttribute(_environmentalMonitor.getNodeName(), AttributeList.NODE_ADHOC_IP, _nodeAdHocIP);
    }

    public void run()
    {
        while (_running) {
            try {
                synchronized (this) {
                    varbind[] nbr = _snmpWalker.getMIBArray  (_nbrCost);
                    parsenbr (nbr);
                }
                Thread.sleep(_pollInterval);
            }
            catch (Exception e) {
                debugMsg ("Failed to poll snmp agent.");
                //e.printStackTrace();
            }
        }
    }

    //-------------------------------------------------------------------
    private void parsenbr (varbind[] nbr)
            throws Exception
    {
/*       1.3.6.1.4.1.14.5.15.3.10.1.4.59.31.1: 1
         1.3.6.1.4.1.14.5.15.3.10.1.4.59.31.2: 10
         1.3.6.1.4.1.14.5.15.3.10.1.4.59.31.3: 3
         1.3.6.1.4.1.14.5.15.3.10.1.4.59.31.4: 56
         1.3.6.1.4.1.14.5.15.3.10.1.4.59.31.5: 0
         1.3.6.1.4.1.14.5.15.3.10.1.4.59.31.6: 0    */
        Hashtable nbrCostTable = new Hashtable();

        for (int i=0; i<nbr.length; i++) {
            varbind vbind = nbr[i];
            String sentry = vbind.toString();
            StringTokenizer st = new StringTokenizer (sentry,":");
            String sndInfo= st.nextToken();
            String costValue = st.nextToken();
            if (sndInfo != null && costValue != null) {
                StringTokenizer st2 = new StringTokenizer (sndInfo,".");
                for (int k=0; k<13; k++) {
                    st2.nextToken();
                }
                String srcNode = st2.nextToken();
                String destNode = st2.nextToken();
                String metric = st2.nextToken();
                String key = srcNode + ":" + destNode;
                NbrCostInfo nbrCost = (NbrCostInfo) nbrCostTable.get (key);
                if (nbrCost == null)  {
                    nbrCost = new NbrCostInfo();
                    nbrCost._srcNodeID = "N" + srcNode;
                    nbrCost._destNodeID = "N" + destNode;
                }
                nbrCost.updateCostInfo (metric, costValue);
                debugMsg ("adding nbrCost [" + key + "] to nbrCostTable");
                nbrCostTable.put (key, nbrCost);
            }
        }

        debugMsg ("AfterParsing snmp info (neighbor cost table)");
//        debugMsg (nbrCostTable.toString());

        //handling every pair-of-nodes now...
        Hashtable attributes = new Hashtable ();    //will be reused several times here.
        Hashtable edgesAddedInThisRun = new Hashtable();
        Enumeration en = nbrCostTable.elements();
        while (en.hasMoreElements()) {
            NbrCostInfo nbrCost = (NbrCostInfo) en.nextElement();
            debugMsg ("number of hops:: [" + nbrCost._minhop + "]");
            if (nbrCost.isOneHop()) {
                debugMsg (nbrCost.toString());
                String edgeID = createReachEdgeID (nbrCost._srcNodeID, nbrCost._destNodeID);
                attributes.clear();
                attributes.put(AttributeList.EDGE_TYPE, "REACHABILITY");
                attributes.put(AttributeList.EDGE_BBN_COST, ""+nbrCost._cost);
                attributes.put(AttributeList.EDGE_BBN_QUALITY, ""+nbrCost._quality);
                attributes.put(AttributeList.EDGE_BBN_UTILIZATION, ""+nbrCost._utilization);
                attributes.put(AttributeList.EDGE_BBN_DB, ""+nbrCost._db);
                try {
                    if (_fgraph.hasEdge(edgeID)) {
                        debugMsg ("setting edge property [" + edgeID + "]");
                        _fgraph.setEdgeAttributeList(edgeID, attributes);
                        edgesAddedInThisRun.put (edgeID,"");
                    }
                    else {
                        debugMsg ("adding edge property [" + edgeID + "]");
                        _fgraph.addEdge(edgeID, nbrCost._srcNodeID, nbrCost._destNodeID, attributes);
                        edgesAddedInThisRun.put (edgeID,"");
                    }
                }
                catch (Exception e) {
                    debugMsg ("Exception updating edge Info: " + e.getMessage());
                }
            }
        }
        cleanupOldReachabilityEdges (edgesAddedInThisRun);
    }

    //--------------------------------------------------------------------
    private void cleanupOldReachabilityEdges (Hashtable edgesAdded)
    {
        try {
            Enumeration en = _fgraph.getEdges(_membershipFilter);
            while (en.hasMoreElements()) {
                String edgeID = (String) en.nextElement();
                if (!edgesAdded.containsKey(edgeID)) {
                    _fgraph.removeEdge(edgeID);
                }
            }
        }
        catch (Exception e) {
            debugMsg ("Got Exception when cleaningUp old edges: " + e.getMessage());
        }
    }

    //--------------------------------------------------------------------
    /**
     * This method will attempt to create a unique id from source and destination
     * node names. The order should not matter. That is, (NodeA, NodeB) and (NodeB,NodeA)
     * should generate the same edgeID.
     * @param srcNode
     * @param destNode
     * @return
     */
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

    //--------------------------------------------------------------------
    public void terminate()
    {
        _running = false;
    }

    public void attributeValueChanged(String attrName, Object value) throws Exception
    {
        debugMsg ("Update of a single attribute is NOT supported");
    }

    /*
    public void attributeValuesChanged(Hashtable attributes) throws Exception
    {
        debugMsg ("Got Attributes Value Changed message");
        if (_updateGPSPosition) {
            debugMsg ("Update GPS Position is ON");
            Enumeration en = attributes.keys();
            while (en.hasMoreElements()) {
                String key = (String) en.nextElement();
                Object att = attributes.get(key);
                System.out.println("\t" + key + " -> " + att.toString());
            }
            System.out.println("-----------------------------------");

            if (attributes.containsKey(AttributeList.NODE_GPS_ALTITUDE) ||
                    attributes.containsKey(AttributeList.NODE_GPS_LATITUDE) ||
                    attributes.containsKey(AttributeList.NODE_GPS_LONGITUDE) ||
                    attributes.containsKey(AttributeList.NODE_HEADING) ||
                    attributes.containsKey(AttributeList.NODE_PITCH) ||
                    attributes.containsKey(AttributeList.NODE_ROLL)) {

                debugMsg ("It is a coordinate attribute - will propagate");
                String gpsInfo = "";
                Double dval = (Double) attributes.get(AttributeList.NODE_GPS_LATITUDE);
                if (dval != null) {
                    gpsInfo = gpsInfo + dval.doubleValue() + " ";
                }
                else {
                    gpsInfo = gpsInfo + "999.0 ";
                }
                dval = (Double) attributes.get(AttributeList.NODE_GPS_LONGITUDE);
                if (dval != null) {
                    gpsInfo = gpsInfo + dval.doubleValue() + " ";
                }
                else {
                    gpsInfo = gpsInfo + "999.0 ";
                }
                dval = (Double) attributes.get(AttributeList.NODE_GPS_ALTITUDE);
                if (dval != null) {
                    gpsInfo = gpsInfo + Math.round(dval.doubleValue()) + " ";
                }
                else {
                    gpsInfo = gpsInfo + "0 ";
                }
                dval = (Double) attributes.get(AttributeList.NODE_HEADING);
                if (dval != null) {
                    gpsInfo = gpsInfo + Math.round(dval.doubleValue()) + " ";
                }
                else {
                    gpsInfo = gpsInfo + "0 ";
                }
                dval = (Double) attributes.get(AttributeList.NODE_PITCH);
                if (dval != null) {
                    gpsInfo = gpsInfo + Math.round(dval.doubleValue()) + " ";
                }
                else {
                    gpsInfo = gpsInfo + "0 ";
                }
                dval = (Double) attributes.get(AttributeList.NODE_ROLL);
                if (dval != null) {
                    gpsInfo = gpsInfo + Math.round(dval.doubleValue()) + " ";
                }
                else {
                    gpsInfo = gpsInfo + "0 ";
                }
                gpsInfo = gpsInfo + ((int)System.currentTimeMillis()) + " ";

                synchronized (this) {
                    try {
                        System.out.println("%%%%%%%%%%%%%%%% Setting (" + gpsInfo + ")");
                        _snmpWalker.setMIBValue(_gpsPosition + ".0", gpsInfo);
                    }
                    catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }
    */
    /////////////////////////////////////////// Private Methods /////////////////////////////
    private void getInterfaceAddress ()
    {
        if (_nodeGatewayIP != null) {
            StringTokenizer st = new StringTokenizer (_nodeGatewayIP, ".");
            if (st.countTokens() == 4) {
                String oct1 = st.nextToken();
                String oct2 = st.nextToken();
                String oct3 = st.nextToken();
                String oct4 = st.nextToken();
                _nodeAdHocIP = "10.0.0." + oct3;
                debugMsg ("Setting nodeGatewayIP to (" + _nodeGatewayIP + ")");
                debugMsg ("Setting nodeAdHocIP to (" + _nodeAdHocIP + ")");
            }
            return;
        }

        _nodeGatewayIP = "192.168.11.1";
        _nodeAdHocIP = "10.0.0.11";
        try {
            InetAddress[] allInets = InetAddress.getAllByName("localhost");
            InetAddress add = InetAddress.getByName (InetAddress.getLocalHost().getHostAddress());
            debugMsg("My Detected Host Addresss is: " + add.getHostAddress());
            if (!add.isLoopbackAddress()) {
                String inetAdd = add.getHostAddress();
                debugMsg ("Setting local Address to (" + inetAdd + ")");
                StringTokenizer st = new StringTokenizer (inetAdd, ".");
                if (st.countTokens() == 4) {
                    String oct1 = st.nextToken();
                    String oct2 = st.nextToken();
                    String oct3 = st.nextToken();
                    String oct4 = st.nextToken();
                    _nodeGatewayIP = oct1 + "." + oct2 + "." + oct3 + ".1";
                    _nodeAdHocIP = "10.0.0." + oct3;
                    debugMsg ("Setting nodeGatewayIP to (" + _nodeGatewayIP + ")");
                    debugMsg ("Setting nodeAdHocIP to (" + _nodeAdHocIP + ")");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //--------------------------------------------------------------------
    protected class NbrCostInfo
    {
        public void updateCostInfo (String metric, String value)
        {
            if (metric != null) {
                metric = metric.trim();
            }
            if (value != null) {
                value = value.trim();
            }

            try {
                int iMetric = Integer.parseInt(metric);
                int iValue = Integer.parseInt(value);
                switch (iMetric) {
                    case 1:
                        _minhop = iValue;
                        break;
                    case 2:
                        _quality = iValue;
                        break;
                    case 3:
                        _cost = iValue;
                        break;
                    case 4:
                        _db = iValue;
                        break;
                    case 5:
                        _utilization = iValue;
                        break;
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }

        public boolean isOneHop ()
        {
            if (_minhop == 1) {
                return true;
            }
            return false;
        }

        public String toString()
        {
            return "NbrCostInfo:: _srcNodeID = " + _srcNodeID
                               + ", _destNodeID = " + _destNodeID
                               + ", _minhop = " + _minhop
                               + ", _quality = " + _quality
                               + ", _cost = " + _cost
                               + ", _db = " + _db
                               + ", _utilization = " + _utilization;
        }

        protected String _srcNodeID;
        protected String _destNodeID;
        protected int _minhop;
        protected int _quality;
        protected int _cost;
        protected int _db;
        protected int _utilization;
   }

    //--------------------------------------------------------------------
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[SNMPProv] " + msg);
        }
    }

    private Hashtable _membershipFilter;
    private boolean _debug = false;
    private FGraph _fgraph;
    //private boolean _updateGPSPosition = true;
    private SNMPWalker _snmpWalker;
    private String _nodeGatewayIP;
    private String _nodeAdHocIP;
    private boolean _running = true;
    private int _pollInterval = 3000;

    private static final String _nbrCost = "1.3.6.1.4.1.14.5.15.3.10.1.4";
    private static final String _gpsPosition = "1.3.6.1.4.1.14.5.15.3.4";
}
