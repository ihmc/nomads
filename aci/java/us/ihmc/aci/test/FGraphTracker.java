package us.ihmc.aci.test;

import us.ihmc.util.ConfigLoader;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.aci.proxyudp.UDPProxy;

import java.net.URI;
import java.util.Hashtable;

/**
 * FGraphTracker
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 8, 2004 at 2:36:45 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphTracker implements FGraphEventListener
{
    public FGraphTracker ()
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        try {
            String gssHost = cloader.getProperty("gss.server.default.host");
            int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
            _fgraph = FGraph.getClient(new URI("tcp://" + gssHost + ":" + gssPort));
            _fgraph.addGraphEventListener(this);
            _fgraph.setEcho(false);
            showGraph();
            while (true) {
                Thread.sleep(1000);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void vertexAdded(String vertexID)
    {
        showGraph ("VERTEX_ADDED " + vertexID);
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        showGraph ("VERTEX_REMOVED " + vertexID);
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        showGraph ("VERTEX_ATT_SET " + vertexID + " " + attKey);
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        showGraph ("VERTEX_ATT_LIST_SET " + vertexID);
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        showGraph ("VERTEX_ATT_REMOVED " + vertexID);
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        showGraph ("EDGE_REMOVED " + edgeID);
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        showGraph ("EDGE_REMOVED " + edgeID);
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        showGraph ("EDGE_ATT_SET " + edgeID + " " + attKey);
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        showGraph ("EDGE_ATT_LIST_SET " + edgeID + " #Atts:" + attributes.size());
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        showGraph ("EDGE_REMOVED " + edgeID);
    }

    public void connectionLost()
    {
        showMessage ("Lost Connection with Fgraph");
    }

    public void connected()
    {
        showMessage ("Connected to Fgraph");
    }

    public static void main(String[] args)
    {
        FGraphTracker fgTracker = new FGraphTracker();
    }

    private void showMessage (String message)
    {
        System.out.println("@" + System.currentTimeMillis() + "\t" + message);
    }

    private void showGraph ()
    {
        showGraph ("GRAPH_DUMP");
    }

    private void showGraph (String message)
    {
        System.out.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
        System.out.println("@" + System.currentTimeMillis() + "\t" + message);
        System.out.println(_fgraph.toString());
        System.out.println("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
    }

    private FGraph _fgraph;
}
