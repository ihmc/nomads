package us.ihmc.aci.envMonitor.provider;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.AttributeList;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.util.ConfigLoader;
import us.ihmc.manet.router.routerMonitor.RouterMonitorClient;
import us.ihmc.manet.util.InetUtil;

import java.net.URI;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Enumeration;

/**
 * MANETConnMonitor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ 
 * Created on Nov 27, 2004 at 2:54:46 PM 
 * $Date: 2004/12/02 04:48:26 
 * $ Copyright (c) 2004, Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class MANETConnMonitor extends AsyncProvider
{
    public void init() throws Exception
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
        _htAtts = new Hashtable();
        _htAtts.put(AttributeList.EDGE_TYPE,"REACHABILITY");

        _routerMonitorClient = new RouterMonitorClient(null);
    }

    public void run()
    {
        while (_running) {
            try {
                String rtable = _routerMonitorClient.getNextMessage();
                parseRTable (rtable);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void terminate()
    {
        _running = false;
    }

    private void  parseRTable (String rtable) throws Exception
    {
        String localIP = InetUtil.getPreferredLocalAddress().getHostAddress();
        StringTokenizer st = new StringTokenizer (rtable,"#");
        while (st.hasMoreElements()) {
            String route = st.nextToken();
            try {
                debugMsg ("Parsing Route: " + route);
                String[] routeInfo = parseRoute (route);
                if (routeInfo != null) {
                    if (routeInfo[1].compareToIgnoreCase("Static") != 0) {  //ignore static routes
                        debugMsg ("\tNOT STATIC");
                        int hopCount = Integer.parseInt(routeInfo[10]);
                        if (hopCount == 1) {
                            String targetIP = routeInfo[0];
                            if (routeInfo[3].compareToIgnoreCase("Inactv") != 0) {
                                debugMsg ("\tSET_ACTIVE");
                                //adding reachability edge for this ONE_HOP route
                                String edgeID = composeEdgeIDBetweenNodes (localIP, targetIP);
                                debugMsg ("\tHAS EDGE (" + edgeID + ")?");
                                if (!_fgraph.hasEdge(edgeID)) {
                                    debugMsg ("\t\tNO");
                                    String envA = getEnvironmentID (localIP);
                                    String envB = getEnvironmentID (targetIP);
                                    debugMsg ("ADDING EDGE (" + edgeID + ") between " + envA + ", " + envB);
                                    if (envA != null && envB != null) {
                                        Hashtable hAtt = new Hashtable();
                                        hAtt.put(AttributeList.EDGE_TYPE, "REACHABILITY");
                                        _fgraph.addUndirectedEdge(edgeID, envA, envB, hAtt);
                                    }
                                }
                                else {
                                    debugMsg ("\t\tYES");
                                }
                            }
                            else {
                                debugMsg ("\tSET_INACTIVE");
                                //remove reachability edge for this ONE_HOP route
                                String edgeID = composeEdgeIDBetweenNodes (localIP, targetIP);
                                if (_fgraph.hasEdge(edgeID)) {
                                    _fgraph.removeEdge(edgeID);
                                }
                            }
                        }
                        else {
                            debugMsg ("\tNOT_ONE_HOP");
                        }
                    }
                    else {
                        debugMsg ("\tSTATIC");
                    }
                }
            }
            catch (Exception e) {
                System.out.println("Skip Route (" + route + "), Exception: " + e.getMessage());
            }
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
            while (en.hasMoreElements()) {
                String vertexID = (String) (en.nextElement());
                String uriCsvList = (String) _fgraph.getVertexAttribute(vertexID,
                                                                        AttributeList.ENVIRONMENT_FFCOMM_URILIST);
                if (ipAddress != null) {
                    if (uriCsvList.indexOf(ipAddress) >=0) {
                        return (vertexID);
                    }
                }
            }
        }
        catch (Exception e) {
            debugMsg ("Environment Not Found: " + e.getMessage());
            //e.printStackTrace();
        }
        return null;
    }

    private String[] parseRoute (String route)
            throws Exception
    {
        String[] routeInfo = new String[15];
        StringTokenizer st = new StringTokenizer (route);
        if (st.countTokens() == 15) {
            int count = 0;
            while (st.hasMoreElements()) {
                routeInfo[count++] = st.nextToken();
            }
        }
        else {
            debugMsg ("WARNING: RouteMessage (" + route + ") \n\thas more than the expected 15 tokens");
            return null;
        }
        return routeInfo;
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[MANETConnMonitor] " + msg);
        }
    }

    private boolean _debug = false;
    private boolean _running = true;
    private RouterMonitorClient _routerMonitorClient;
    private FGraph _fgraph;
    private Hashtable _htFilter;
    private Hashtable _htAtts;
}
