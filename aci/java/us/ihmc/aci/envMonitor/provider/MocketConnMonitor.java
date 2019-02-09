package us.ihmc.aci.envMonitor.provider;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.AttributeList;
import us.ihmc.mockets.util.MocketStatusMonitor;
import us.ihmc.mockets.util.MocketConnectionListener;
import us.ihmc.util.ConfigLoader;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphClient;

import java.util.Hashtable;
import java.util.Enumeration;
import java.net.URI;
import java.net.InetAddress;

/**
 * @author mcarvalho@ihmc.us
 * Date: Nov 22, 2004
 * Time: 11:15:31 PM
 * To change this template use File | Settings | File Templates.
 *
 * This provider will add/modify only EDGE objects with EDGE_TYPE="MOCKET_CONNECTION" in fgraph.
 * MOCKET_CONNECTION edges will be set with the (MOCKET_CONNECTION_STATE) attribute.
 *
 * The MOCKET_CONNECTION cam have to String values "HEALTHY" or "BROKEN"
 *
 */
public class MocketConnMonitor extends AsyncProvider implements MocketConnectionListener
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
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));
        _htFilter = new Hashtable();
        _htFilter.put(AttributeList.NODE_TYPE,"ENVIRONMENT");

        _mocketStatusMonitor = new MocketStatusMonitor();
        _mocketStatusMonitor.setMocketConnectionListener(this);
        Thread mockMon = new Thread(_mocketStatusMonitor);
        mockMon.start();
    }

    public void run()
    {
        while (_running) {
            try {
                Thread.sleep(3000);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void connectionFailed(String clientIPAddr, String serverIPAddr, int serverPort) {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    ///////////////////// MocketConnectionListener Interface Methods ////////////////////////////
    public void connectionEstablished(String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        this.addMocketConnection(ipAddrOne, ipAddrTwo);
    }

    public void connectionLost(String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        this.removeMocketConnection(ipAddrOne, ipAddrTwo);
    }

    public void connectionRestored(String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        this.updateMocketConnection(ipAddrOne, ipAddrTwo, true);
    }

    public void connectionClosed(String ipAddrOne, int portOne, String ipAddrTwo, int portTwo)
    {
        this.updateMocketConnection(ipAddrOne, ipAddrTwo, false);
    }

    //////////////////// Private Methods ////////////////////////////////////////////////////////
    private void addMocketConnection (String ipAddressOne, String ipAddressTwo)
    {
        debugMsg ("Add Mocket Connection (" + ipAddressOne + "," + ipAddressTwo + ")");
        String nodeOne = getEnvironmentID (ipAddressOne);
        String nodeTwo = getEnvironmentID (ipAddressTwo);
        String edgeId = composeEdgeIDBetweenNodes (nodeOne, nodeTwo);

        try {
            if (_fgraph.hasEdge(edgeId)) {
                _fgraph.setEdgeAttribute(edgeId, AttributeList.MOCKET_CONNECTION_STATE, "HEALTHY");
            }
            else {
                Hashtable att = new Hashtable();
                att.put(AttributeList.MOCKET_CONNECTION_STATE,"HEALTY");
                _fgraph.addUndirectedEdge(edgeId, nodeOne, nodeTwo, att);
            }
        }
        catch (Exception e) {
            debugMsg ("Ignoring exception : " + e.getMessage());
            //e.printStackTrace();
        }
    }

    private void removeMocketConnection (String ipAddressOne, String ipAddressTwo)
    {
        debugMsg ("Remove Mocket Connection (" + ipAddressOne + "," + ipAddressTwo + ")");
        String nodeOne = getEnvironmentID (ipAddressOne);
        String nodeTwo = getEnvironmentID (ipAddressTwo);
        String edgeId = composeEdgeIDBetweenNodes (nodeOne, nodeTwo);

        try {
            if (_fgraph.hasEdge(edgeId)) {
                _fgraph.removeEdge(edgeId);
            }
        }
        catch (Exception e) {
            debugMsg ("Ignoring exception : " + e.getMessage());
            //e.printStackTrace();
        }
    }

    private void updateMocketConnection (String ipAddressOne, String ipAddressTwo, boolean healthy)
    {
        String msg = "Update Mocket Connection (" + ipAddressOne + "," + ipAddressTwo + ") ";
        if (healthy) {
            msg = msg + " TRUE";
        }
        else {
            msg = msg + " FALSE";
        }
        String nodeOne = getEnvironmentID (ipAddressOne);
        String nodeTwo = getEnvironmentID (ipAddressTwo);
        String edgeId = composeEdgeIDBetweenNodes (nodeOne, nodeTwo);

        try {
            if (_fgraph.hasEdge(edgeId)) {
                if (healthy) {
                    _fgraph.setEdgeAttribute(edgeId, AttributeList.MOCKET_CONNECTION_STATE, "HEALTHY");
                }
                else {
                    _fgraph.setEdgeAttribute(edgeId, AttributeList.MOCKET_CONNECTION_STATE, "BROKEN");
                }
            }
            else {
                Hashtable att = new Hashtable();
                if (healthy) {
                    att.put(AttributeList.MOCKET_CONNECTION_STATE,"HEALTY");
                }
                else {
                    att.put(AttributeList.MOCKET_CONNECTION_STATE,"BROKEN");
                }
                _fgraph.addUndirectedEdge(edgeId, nodeOne, nodeTwo, att);
            }
        }
        catch (Exception e) {
            debugMsg ("Ignoring exception : " + e.getMessage());
            //e.printStackTrace();
        }
    }

    private String composeEdgeIDBetweenNodes (String nodeOne, String nodeTwo)
    {
        String edgeID = null;
        if (nodeOne != null && nodeTwo != null) {
            if (nodeOne.compareTo(nodeTwo) <= 0) {
                edgeID = nodeOne + ":" + nodeTwo;
            }
            else {
                edgeID = nodeTwo + ":" + nodeOne;
            }
        }
        return edgeID;
    }

    private String getEnvironmentID (String ipAddress)
    {
        try {
            Enumeration en = _fgraph.getVertices(_htFilter);
            //debugMsg("Locating URI for IPAddress: (" + ipAddress + ")");
            while (en.hasMoreElements()) {
                String vertexID = (String) (en.nextElement());
                String uriCsvList = (String) _fgraph.getVertexAttribute(vertexID,
                                                                        AttributeList.ENVIRONMENT_FFCOMM_URILIST);
                //debugMsg ("Look for ip (" + ipAddress + ") in URICsvList = " + uriCsvList);
                if (ipAddress != null) {
                    if (uriCsvList.indexOf(ipAddress) >=0) {
                        //debugMsg("Returning EnvironmentID : (" + vertexID + ")");
                        return (vertexID);
                    }
                }
            }
        }
        catch (Exception e) {
            debugMsg ("Ignoring exception : " + e.getMessage());
            //e.printStackTrace();
        }
        return null;
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[MocketConnMonitor] " + msg);
        }
    }

    private Hashtable _htFilter;
    private boolean _debug = false;
    private boolean _running = true;
    private MocketStatusMonitor _mocketStatusMonitor;
    private FGraph _fgraph;
}


