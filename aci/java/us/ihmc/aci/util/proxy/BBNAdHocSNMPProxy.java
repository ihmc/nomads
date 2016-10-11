package us.ihmc.aci.util.proxy;

import us.ihmc.ds.fgraph.FGraphClient;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.ds.CircularQueue;
import us.ihmc.aci.AttributeList;
import us.ihmc.util.ConfigLoader;

import java.net.URI;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.*;



/**
 * Created by IntelliJ IDEA.
 * User: mcarvalho
 * Date: Dec 20, 2004
 * Time: 1:23:12 PM
 * To change this template use File | Settings | File Templates.
 */
public class BBNAdHocSNMPProxy extends Thread implements FGraphEventListener
{
    BBNAdHocSNMPProxy (URI remoteURI, int localServerPort)
            throws Exception
    {
        try {
            _handlers = new Hashtable();
            connectToServer (remoteURI);
            startServer (localServerPort);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    BBNAdHocSNMPProxy (int localServerPort)
            throws Exception
    {
        try {
            _handlers = new Hashtable();
            ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
            String shost = cloader.getProperty("gss.server.default.host");
            int iport = cloader.getPropertyAsInt("gss.server.default.port");
            connectToServer (new URI("tcp://" + shost + ":" + iport));
            startServer (localServerPort);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void main (String[] args)
    {
        if (args.length < 1) {
            System.out.println ("Usage: BBNAdHocSNMPProxy [<gssServer_uri>] <localPort>\n");
            System.out.println ("URI format: http://<host>:<port>");
            System.out.println ("The URI is optional. If not provided, the config file will be used.");
            return;
        }

        try {
            if (args.length == 2) {
                BBNAdHocSNMPProxy proxy = new BBNAdHocSNMPProxy (new URI(args[0]), Integer.parseInt(args[1]));
                proxy.start();
            }
            if (args.length == 1) {
                BBNAdHocSNMPProxy proxy = new BBNAdHocSNMPProxy (Integer.parseInt(args[0]));
                proxy.start();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Creates an instance of FGraphClient, connected to FGraph server.
     * @param remoteURI
     * @throws Exception
     */
    private void connectToServer (URI remoteURI)
            throws Exception
    {
        debugMsg ("connecting to Remote FGraphServer (" + remoteURI.toASCIIString() + ")");
        _fgraph = (FGraphClient) FGraph.getClient(remoteURI);
        debugMsg ("Registering for fgraph Events");

        _fgraph.addGraphEventListener(this);
    }

    /**
     * Starts local TCP server, waiting for connections from local clients
     * @param localServerPort
     * @throws IOException
     */
    public void startServer (int localServerPort)
            throws IOException
    {
        _serverSocket = new ServerSocket (localServerPort);
        debugMsg ("Listening on port " + localServerPort);
    }

    public void run()
    {
        while (_running) {
            try {
                debugMsg ("waiting for connetions");
                Socket sock = _serverSocket.accept();
                debugMsg ("Received connection from Client (" + sock.getInetAddress().getHostAddress() + ")");
                FGCProxyConnHandler fgcConnHandler = new FGCProxyConnHandler (sock, this);
                debugMsg ("Registering ConnHandler: " + fgcConnHandler.getConnHandlerName());
                _handlers.put(fgcConnHandler.getConnHandlerName(), fgcConnHandler);
                fgcConnHandler.start();
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void terminate ()
    {
        debugMsg ("Received request for termination");
        Enumeration en = _handlers.keys();
        while (en.hasMoreElements()) {
            String sname = (String) en.nextElement();
            FGCProxyConnHandler connHandler = (FGCProxyConnHandler) _handlers.get(sname);
            if (connHandler != null) {
                debugMsg ("\tRequesting termination of ConnHandler: " + sname);
                connHandler.terminate();
            }
        }
        _running = false;
        try {
            if (_serverSocket != null) {
                debugMsg ("\tClosing Server socket");
                _serverSocket.close();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void terminateConnHandler (String connHandlerName)
    {
        debugMsg ("Terminating ConnHandler: " + connHandlerName);
        if (_handlers.containsKey(connHandlerName)) {
            FGCProxyConnHandler connHandler = (FGCProxyConnHandler) _handlers.get(connHandlerName);
            _handlers.remove(connHandlerName);
            connHandler.terminate();
        }
    }

    public void vertexAdded(String vertexID)
    {
        debugMsg ("(Ignoring) Vertex_Added (" + vertexID + ")");
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        debugMsg ("(Ignoring) vertexRemoved (" + vertexID + ")");
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        debugMsg ("(Ignoring) vertexAttribSet (" + vertexID + ")");
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        debugMsg ("(Ignoring) vertexAttribListSet (" + vertexID + ")");
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        debugMsg ("(Ignoring) vertexAttribRemoved (" + vertexID + ")");
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        debugMsg ("EdgeAdded (" + edgeID + ")");
        String message = buildMessageForEdge (edgeID, sourceID, destID);
        if (message != null) {
            debugMsg ("\tNotify: " + message);
            notifyConnHandlers (message);
        }
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        debugMsg ("(Ignoring) edgeRemoved (" + edgeID + ")");
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        debugMsg ("EdgeAttributeSet (" + edgeID + ")");
        String message = buildMessageForEdge (edgeID, null, null);
        if (message != null) {
            debugMsg ("\tNotify: " + message);
            notifyConnHandlers (message);
        }
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        debugMsg ("edgeAttribListSet (" + edgeID + ")");
        String message = buildMessageForEdge (edgeID, null, null);
        if (message != null) {
            debugMsg ("\tNotify: " + message);
            notifyConnHandlers (message);
        }
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        debugMsg ("(Ignoring) edgeAttribRemoved (" + edgeID + ")");
    }

    public void connectionLost()
    {
        debugMsg ("(Ignoring) connectionLost");
    }

    public void connected()
    {
        debugMsg ("Connected");
        try {
            debugMsg ("\tEnumerating edges for notification");
            Enumeration edgeList = _fgraph.getEdges();
            while (edgeList.hasMoreElements()) {
                String edgeName = (String) edgeList.nextElement();
                String message = buildMessageForEdge (edgeName, null, null);
                if (message != null) {
                    debugMsg ("\t(Notify) " + edgeName + ":" + message);
                    notifyConnHandlers (message);
                }
                else {
                    debugMsg ("\t(Ignore) " + edgeName + ": NULL");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //////////// Proxy ConnHandler - Handles Comms with a client (TCP)//////////////
    public class FGCProxyConnHandler extends Thread
    {
        public FGCProxyConnHandler (Socket sock, BBNAdHocSNMPProxy proxy)
                throws IOException
        {
            _socket = sock;
            _queue = new CircularQueue (100);
            _proxy = proxy;
            _name = _socket.getInetAddress().getHostAddress() + ":" + _socket.getPort();
            _bw = new BufferedWriter(new OutputStreamWriter (_socket.getOutputStream()));
        }

        public String getConnHandlerName ()
        {
            return (_name);
        }

        public void terminate ()
        {
            _running = false;
            try {
                _socket.close();
            }
            catch (Exception e) {
                debugMsg ("Terminate::Exception: " + e.getMessage());
            }
        }

        public synchronized void addMessage (String message)
        {
            if (_queue.isFull()) {
                debugMsg ("Warning - Queue is FULL - Dropping: " + message);
            }
            _queue.enqueue(message);
            notifyAll();
        }

        public void run()
        {
            while (_running) {
                synchronized (this) {
                    try {
                        String msg = null;
                        while (msg == null) {
                            wait();
                            msg = (String) _queue.dequeue();
                        }
                        _bw.write(msg);
                        _bw.newLine();
                        _bw.flush();
                    }
                    catch (Exception e) {
                        //e.printStackTrace();
                        debugMsg ("Exception: " + e.getMessage());
                        _proxy.terminateConnHandler (getConnHandlerName());
                    }
                }
            }
        }

        private String _name;
        BBNAdHocSNMPProxy _proxy;
        private boolean _running = true;
        private CircularQueue _queue;
        private BufferedWriter _bw;
        private Socket _socket;
    }

    ////////////////////// BuildMessage for FGInfoEdge ///////////////////////////////////////////
    private String buildMessageForEdge (String edgeID, String sourceID, String targetID)
    {
        String message = null;

        try {
            String edgeType = (String) _fgraph.getEdgeAttribute(edgeID, AttributeList.EDGE_TYPE);
            if (edgeType.equals("REACHABILITY")) {
                String cost = (String) _fgraph.getEdgeAttribute(edgeID, AttributeList.EDGE_BBN_COST);
                String quality = (String) _fgraph.getEdgeAttribute(edgeID, AttributeList.EDGE_BBN_QUALITY);
                String utilization = (String) _fgraph.getEdgeAttribute(edgeID, AttributeList.EDGE_BBN_UTILIZATION);
                String db = (String) _fgraph.getEdgeAttribute(edgeID, AttributeList.EDGE_BBN_DB);
                if (cost == null) {
                    cost = "X";
                }
                if (quality == null) {
                    quality = "X";
                }
                if (utilization == null) {
                    utilization = "X";
                }
                if (db == null) {
                    db = "X";
                }

                if (sourceID == null) {
                    sourceID = _fgraph.getEdgeSource(edgeID);
                }

                if (targetID == null) {
                    targetID = _fgraph.getEdgeTarget(edgeID);
                }

                if (sourceID != null && targetID != null) {
                    //if the nodes have a name attribute, let's use that instead of the ID....
                    String sourceName = (String) _fgraph.getVertexAttribute(sourceID, AttributeList.NODE_NAME);
                    if (sourceName != null) {
                        sourceID = sourceName;
                    }
                    String targetName = (String) _fgraph.getVertexAttribute(targetID, AttributeList.NODE_NAME);
                    if (targetName != null) {
                        targetID = targetName;
                    }

                    message = sourceID + "," + targetID + ",";
                    message = message + cost + "," + quality + "," + utilization + "," + db;
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return message;
    }

    ////////////////////// Interfacing with Proxy Clients via TCP  ////////////////////////
    public void notifyConnHandlers (String message)
    {
        Enumeration en = _handlers.elements();
        while (en.hasMoreElements()) {
            FGCProxyConnHandler fgcHandler = (FGCProxyConnHandler) en.nextElement();
            debugMsg ("\t\t>" + fgcHandler.getConnHandlerName());
            fgcHandler.addMessage(message);
        }
    }


    ////////////////////////// Private Mathods ////////////////////////////
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[FGClientProxy] " + msg);
        }
    }

    ///////////////////////// Member Variables ////////////////////////////
    private Hashtable _handlers;
    private ServerSocket _serverSocket;
    private boolean _debug = true;
    private boolean _running = true;
    private FGraphClient _fgraph;





}
