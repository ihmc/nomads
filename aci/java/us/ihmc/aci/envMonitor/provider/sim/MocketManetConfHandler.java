package us.ihmc.aci.envMonitor.provider.sim;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.util.ConfigLoader;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.manet.MANETConstants;
import us.ihmc.manet.router.aodv.AODVConstants;

import java.net.URI;
import java.util.Hashtable;

/**
 * MocketManetConfHandler
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on Jul 23, 2004 at 12:37:36 AM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class MocketManetConfHandler extends AsyncProvider implements FGraphEventListener
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
    }

    public void run()
    {
        try {
            _fgraph.addGraphEventListener(this);
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
        if (vertexID.equals("CONFIG_MANAGER")) {
            updateConfiguration (vertexID);
        }
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {

    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        if (vertexID.equals("CONFIG_MANAGER")) {
            updateConfiguration (vertexID);
        }
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        if (vertexID.equals("CONFIG_MANAGER")) {
            updateConfiguration (vertexID);
        }
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
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
    }

    public void connected()
    {
    }

    //----------------------------------------------------------------

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[NetConMon] " + msg);
        }
    }

    private void updateConfiguration(String vertexID)
    {
        try {
            TestConfiguration tconfig = (TestConfiguration) _fgraph.getVertexAttribute(vertexID, "TEST_CONFIGURATION");
            if (tconfig != null) {
                /*
                Mocket.ACK_TIMEOUT = tconfig.ACK_TIMEOUT;
                Mocket.BUFFERING_TIME = tconfig.BUFFERING_TIME;
                Mocket.CLOSE_WAIT_TIME = tconfig.CLOSE_WAIT_TIME;
                Mocket.CONNECT_RETRIES = tconfig.CONNECT_RETRIES;
                Mocket.CONNECT_TIMEOUT = tconfig.CONNECT_TIMEOUT;
                Mocket.FIN_ACK_TIMEOUT = tconfig.FIN_ACK_TIMEOUT;
                Mocket.KEEPALIVE_TIME = tconfig.KEEPALIVE_TIME;
                Mocket.MAX_PACKET_SIZE = tconfig.MAX_PACKET_SIZE;
                Mocket.MTU = tconfig.MTU;
                Mocket.PACKET_TRANSMIT_INTERVAL = tconfig.PACKET_TRANSMIT_INTERVAL;
                Mocket.RCVR_BUFFER_SIZE = tconfig.RCVR_BUFFER_SIZE;
                Mocket.RECEIVE_TIMEOUT = tconfig.RECEIVE_TIMEOUT;
                Mocket.SLIDING_WINDOW_SIZE = tconfig.SLIDING_WINDOW_SIZE;
                Mocket.SYN_HISTORY_SIZE = tconfig.SYN_HISTORY_SIZE;
                Mocket.SYN_RESEND_TIMEOUT = tconfig.SYN_RESEND_TIMEOUT;
                Mocket.SYN_VALIDITY_WINDOW = tconfig.SYN_VALIDITY_WINDOW;
                */
                MANETConstants.MANET_GATEWAY_BUFFER_QUEUE_SIZE = tconfig.MANET_GATEWAY_BUFFER_QUEUE_SIZE;
                MANETConstants.MANET_SOCKET_BUFFER_HANDLER_QUEUE_SIZE = tconfig.MANET_SOCKET_BUFFER_HANDLER_QUEUE_SIZE;
                AODVConstants.ACTIVE_ROUTE_LIFETIME = tconfig.ACTIVE_ROUTE_LIFETIME;
                AODVConstants.ECHO_CACHE_TIMEOUT = tconfig.ECHO_CACHE_TIMEOUT;
                AODVConstants.RREQ_RREQ_MIN_DELAY = tconfig.RREQ_RREQ_MIN_DELAY;
                AODVConstants.SEQUENCE_NUMBER_THRESHOLD = tconfig.SEQUENCE_NUMBER_THRESHOLD;
                AODVConstants.VALID_ROUTE_LIFETIME = tconfig.VALID_ROUTE_LIFETIME;
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static final String CONFIG_MANAGER_NAME = "CONFIG_MANAGER";
    public static final String TEST_CONFIG_ATTRIBUTE_NAME = "TEST_CONFIGURATION";

    private boolean _debug = false;
    private FGraph _fgraph;
    private boolean _running = true;
}

