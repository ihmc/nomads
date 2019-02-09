package us.ihmc.aci.proxyudp;

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
import uk.co.westhawk.snmp.stack.AsnObjectId;

public class BBNAdHocSNMPProviderUDP extends AsyncProvider /*implements EnvironmentalChangeListener*/
{
    public void init ()
            throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        _pollInterval = cloader.getPropertyAsInt("arl.provider.BBNSNMP.pollInterval",_pollInterval);
        _debug = cloader.hasProperty("arl.provider.BBNSNMP.debug");
        System.out.println("PoolInterval: " + _pollInterval);
        getInterfaceAddress();
        _snmpWalker = new SNMPWalker (_nodeGatewayIP, "robot");
    }

    public void run()
    {
        while (_running) {
            try {
                synchronized (this) {
                    varbind[] nbr = _snmpWalker.getMIBArray  (_nbrCost);
                    //varbind[] nbr = buildFakeData();
                    parsenbr (nbr);
                }
            }
            catch (Exception e) {
                debugMsg ("Failed to poll snmp agent. " + e.getMessage());
            }

            try {
                Thread.sleep(_pollInterval);
            }
            catch (Exception e) { }
        }
    }

    //-------------------------------------------------------------------
    private void parsenbr (varbind[] nbr)
            throws Exception
    {
        Hashtable nbrCostTable = new Hashtable();
        debugMsg ("Size of the nbr: " + nbr.length);
        for (int i=0; i<nbr.length; i++) {
            String sdebugInfo = "";
            sdebugInfo = sdebugInfo + "parsenbr: [" + i + "]\t";
            varbind vbind = nbr[i];
            String sentry = vbind.toString();
            sentry = sentry.substring(_nbrCost.length()+1);
            sdebugInfo = sdebugInfo + sentry + "\t";
            StringTokenizer st = new StringTokenizer (sentry,":");
            String sndInfo= st.nextToken().trim();
            String costValue = st.nextToken().trim();
            //System.out.println("\tsentry: " + sentry);
            //System.out.println("\tsndInfo: " + sndInfo);
            //System.out.println("\tcostValue: " + costValue);
            if (sndInfo != null && costValue != null) {
                StringTokenizer st2 = new StringTokenizer (sndInfo,".");
                String srcNode = st2.nextToken().toString();
                String destNode = st2.nextToken().toString();
                String metric = st2.nextToken().toString();
                //System.out.println("\tmetric: " + metric);
                if (_nodeId != null) {
                    if (_nodeId.equals(srcNode.trim()) || _nodeId.equals(destNode.trim())) {
                        String key = srcNode + ":" + destNode;
                        NbrCostInfo nbrCost = (NbrCostInfo) nbrCostTable.get (key);
                        if (nbrCost == null)  {
                            nbrCost = new NbrCostInfo();
                            nbrCost._srcNodeID = "N" + srcNode;
                            nbrCost._destNodeID = "N" + destNode;
                        }
                        nbrCost.updateCostInfo (metric, costValue);
                        nbrCostTable.put (key, nbrCost);
                        sdebugInfo = sdebugInfo + "\t" + nbrCost.toString();
                    }
                    else {
                        sdebugInfo = sdebugInfo + "\t<Discarded>";
                    }
                }
            }
            debugMsg (sdebugInfo);
        }

        //debugMsg ("AfterParsing snmp info (neighbor cost table)");
        //debugMsg (nbrCostTable.toString());

        //handling every pair-of-nodes now...
        Hashtable edgesAddedInThisRun = new Hashtable();
        Enumeration en = nbrCostTable.elements();
        String msg = "";
        while (en.hasMoreElements()) {
            NbrCostInfo nbrCost = (NbrCostInfo) en.nextElement();
            if (nbrCost.isOneHop()) {
                String edgeID = createReachEdgeID (nbrCost._srcNodeID, nbrCost._destNodeID);
                try {
                    msg = msg + "ADD_EDGE " + nbrCost._srcNodeID;
                    msg = msg + " " +  nbrCost._destNodeID + " ";
                    msg = msg + nbrCost._minhop + " " + nbrCost._cost + " " + nbrCost._quality + " "
                            + " " + nbrCost._db + " " + nbrCost._utilization;
                    msg = msg + "\n";
                    edgesAddedInThisRun.put (edgeID,"");
                }
                catch (Exception e) {
                    debugMsg ("Exception updating edge Info: " + e.getMessage());
                }
            }
        }
        _envAgentUDP.setEdgeUpdate(msg);
    }

    private varbind[] buildFakeData()
    {
        varbind[] blist = new varbind[30];
        blist[0] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.11.1"),new AsnObjectId("1"));
        blist[1] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.11.2"),new AsnObjectId("3"));
        blist[2] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.11.3"),new AsnObjectId("3"));
        blist[3] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.11.4"),new AsnObjectId("23"));
        blist[4] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.11.5"),new AsnObjectId("0"));
        blist[5] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.11.6"),new AsnObjectId("0"));
        blist[6] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.34.1"),new AsnObjectId("1"));
        blist[7] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.34.2"),new AsnObjectId("3"));
        blist[8] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.34.3"),new AsnObjectId("3"));
        blist[9] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.34.4"),new AsnObjectId("25"));
        blist[10] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.34.5"),new AsnObjectId("0"));
        blist[11] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.34.6"),new AsnObjectId("0"));
        blist[12] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.44.1"),new AsnObjectId("1"));
        blist[13] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.44.2"),new AsnObjectId("200"));
        blist[14] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.44.3"),new AsnObjectId("3"));
        blist[15] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.44.4"),new AsnObjectId("63"));
        blist[16] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.44.5"),new AsnObjectId("0"));
        blist[17] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.2.44.6"),new AsnObjectId("0"));
        blist[18] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.2.1"),new AsnObjectId("1"));
        blist[19] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.2.2"),new AsnObjectId("3"));
        blist[20] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.2.3"),new AsnObjectId("3"));
        blist[21] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.2.4"),new AsnObjectId("15"));
        blist[22] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.2.5"),new AsnObjectId("0"));
        blist[23] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.2.6"),new AsnObjectId("0"));
        blist[24] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.34.1"),new AsnObjectId("1"));
        blist[25] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.34.2"),new AsnObjectId("3"));
        blist[26] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.34.3"),new AsnObjectId("3"));
        blist[27] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.34.4"),new AsnObjectId("16"));
        blist[28] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.34.5"),new AsnObjectId("0"));
        blist[29] = new varbind(new AsnObjectId("1.3.6.1.4.1.14.5.15.3.10.1.4.11.34.6"),new AsnObjectId("0"));
        return blist;
    }

    //--------------------------------------------------------------------
    public void setEnvAgentUDP (EnvAgentUDP envAgentUDP)
    {
        _envAgentUDP = envAgentUDP;
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

    /////////////////////////////////////////// Private Methods /////////////////////////////
    private void getInterfaceAddress ()
    {
        _nodeGatewayIP = "192.168.11.1";
        _nodeAdHocIP = "10.0.0.11";
        try {
            InetAddress[] allInets = InetAddress.getAllByName("localhost");
            InetAddress add = InetAddress.getByName (InetAddress.getLocalHost().getHostAddress());
            System.out.println(add.getHostAddress());
            if (!add.isLoopbackAddress()) {
                String inetAdd = add.getHostAddress();
                debugMsg ("Setting local Address to (" + inetAdd + ")");
                StringTokenizer st = new StringTokenizer (inetAdd, ".");
                if (st.countTokens() == 4) {
                    String oct1 = st.nextToken();
                    String oct2 = st.nextToken();
                    String oct3 = st.nextToken();
                    String oct4 = st.nextToken();
                    _nodeId = oct3;
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
            debugMsg ("nbrCost.updateCostInfo (" + metric + "," + value + ")");
            try {
                int iMetric = Integer.parseInt(metric.trim());
                int iValue = Integer.parseInt(value.trim());
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
            catch (Exception e) {}
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
            String sinfo = _srcNodeID + ">" + _destNodeID + " [";
            sinfo = sinfo + _minhop + "," + _quality + "," + _cost + "," + _utilization + "]";
            return (sinfo);
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

    private boolean _debug = false;
    private String _nodeId = null;             //based on the 3rd octet of the ip
    private static EnvAgentUDP _envAgentUDP;
    private SNMPWalker _snmpWalker;
    private String _nodeGatewayIP;
    private String _nodeAdHocIP;
    private boolean _running = true;
    private int _pollInterval = 3000;

    private static final String _nbrCost = "1.3.6.1.4.1.14.5.15.3.10.1.4";
    private static final String _gpsPosition = "1.3.6.1.4.1.14.5.15.3.4";
}
