package us.ihmc.aci.envMonitor.provider.sim;

import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphException;
import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.AttributeList;
import us.ihmc.util.ConfigLoader;

import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;
import java.net.URI;


/**
 * ConnRecoveryMonitor
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Jul 23, 2004 at 12:37:36 AM
 *          $Date: 2005/07/19 21:20:53
 *          $ Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 *
 * ConnRecoveryMonitor monitor is a provider that Attempts to maintain active streams between
 * nodes by acting as a relay for communications. The provider will monitor all acvite stream
 * requests and reachability in the network to decide when/where to move in order to recover
 * comms.
 */

public class ConnRecoveryMonitor extends AsyncProvider implements FGraphEventListener
{

    public void run()
    {
        _running = true;
        try {
            _fgraph.addGraphEventListener(this);
            checkConnectivity();
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

    /////////////////// AsyncProvider Interface Methods ///////////////////////////
    public void init()
            throws Exception
    {
        _envName = _environmentalMonitor.getNodeName();
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cloader.getProperty("gss.server.default.host");
        int gssPort = cloader.getPropertyAsInt("gss.server.default.port");
        if (gssServerHost == null) {
            throw new Exception ("Uknown gssServer host - Faild to locate property (gss.server.default.host)");
        }

        //checking if the node has the property (MOBILE) set
        _fgraph = FGraph.getClient(new URI("tcp://" + gssServerHost + ":" + gssPort));
        if (_fgraph.getVertexAttribute(_envName, AttributeList.NODE_MOBLIE) != null) {
            _isMobile = true;
        }
        _edgeRequestFilter = new Hashtable();
        _edgeRequestFilter.put(AttributeList.EDGE_TYPE, "STREAM_REQUEST");
        _edgeStreamFilter = new Hashtable();
        _edgeStreamFilter.put(AttributeList.EDGE_TYPE, "DATA_STREAM");
        _edgeMembershipFilter = new Hashtable();
        _edgeMembershipFilter.put(AttributeList.EDGE_TYPE,"MEMBERSHIP");
        _edgeReachabilityFilter = new Hashtable();
        _edgeReachabilityFilter.put(AttributeList.EDGE_TYPE,"REACHABILITY");
    }

    ///////////////////// FGraphEventListener Methods /////////////////////////////
    public void vertexAdded(String vertexID)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        checkConnectivity();
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        checkConnectivity();
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connectionLost()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connected()
    {
        checkConnectivity();
    }

    ///////////////////// Private Methods /////////////////////////////
    private synchronized void checkConnectivity()
    {
        if (_isMobile) {
            try {
                Enumeration en = _fgraph.getEdges(_edgeRequestFilter);
                while (en.hasMoreElements()) {
                    String streamReqID = (String) en.nextElement();
                    if (checkStreamRequest (streamReqID)) {
                        break;
                    }
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    //------------------------------------------------------------------
    private boolean checkStreamRequest (String streamReqID)
    {
        try {
            boolean found_activeStream = false;
            Enumeration en = _fgraph.getEdges(_edgeStreamFilter);
            while (en.hasMoreElements()) {
                String streamID = (String) en.nextElement();
                String strReq = (String) _fgraph.getEdgeAttribute(streamID, AttributeList.EDGE_STREAM_REQUEST_ID);
                if (strReq.equals(streamReqID)) {
                    found_activeStream = true;
                    break;
                }
            }

            if (found_activeStream) {
                _fgraph.removeVertexAttribute(_envName, AttributeList.NODE_TARGET_XPOS);
                _fgraph.removeVertexAttribute(_envName, AttributeList.NODE_TARGET_YPOS);
            }
            else {
                String srcNode = (String) _fgraph.getEdgeSource(streamReqID);
                String targetNode = (String) _fgraph.getEdgeTarget(streamReqID);

                // finding source/target environments from srcNode and targetNode (agents).
                String srcEnv = null;
                Enumeration en2 = _fgraph.getOutgoingEdges (srcNode,_edgeMembershipFilter);
                if (en2.hasMoreElements()) {
                    String edgeid = (String) en2.nextElement();
                    if (edgeid != null) {
                        srcEnv = _fgraph.getEdgeTarget(edgeid);
                    }
                }
                String targetEnv = null;
                Enumeration en3 = _fgraph.getOutgoingEdges (targetNode,_edgeMembershipFilter);
                if (en3.hasMoreElements()) {
                    String edgeid = (String) en3.nextElement();
                    if (edgeid != null) {
                        targetEnv = _fgraph.getEdgeTarget(edgeid);
                    }
                }

                if (commRelayEligible ()) {
                    if (!findPathFrom (srcEnv, targetEnv, null)) {
                        moveToCentralPointBetween(srcEnv, targetEnv);
                        found_activeStream = true;
                        return true;
                    }
                }
                found_activeStream = false;
            }

        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }


    //------------------------------------------------------------------
    private boolean findPathFrom (String srcEnv, String destEnv, Vector visited)
    {
        if (visited == null) {
            visited = new Vector();
        }

        visited.addElement(srcEnv);
        try {
            Enumeration en = _fgraph.getOutgoingEdges (srcEnv,_edgeReachabilityFilter);
            while (en.hasMoreElements()) {
                String edgeId = (String) en.nextElement();
                String edgeSrc = _fgraph.getEdgeSource(edgeId);
                String edgeDest = _fgraph.getEdgeTarget(edgeId);
                if (edgeSrc.equals(srcEnv)) {
                    if (edgeDest.equals(destEnv)) {
                        return true;
                    }
                    else if (!visited.contains(edgeDest)) {
                        return findPathFrom (edgeDest, destEnv, visited);
                    }
                }
                else {
                    if (edgeSrc.equals(destEnv)) {
                        return true;
                    }
                    else if (!visited.contains(edgeSrc)) {
                        return findPathFrom (edgeSrc, destEnv, visited);
                    }
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    //------------------------------------------------------------------
    private void moveToCentralPointBetween (String src, String dest)
    {
        try {
            Double xSrc = (Double) _fgraph.getVertexAttribute(src, AttributeList.NODE_XPOS);
            Double ySrc = (Double) _fgraph.getVertexAttribute(src, AttributeList.NODE_YPOS);
            Double xDest = (Double) _fgraph.getVertexAttribute(dest, AttributeList.NODE_XPOS);
            Double yDest = (Double) _fgraph.getVertexAttribute(dest, AttributeList.NODE_YPOS);
            if (xSrc != null && ySrc != null && xDest!= null && yDest!=null) {
                double dx = (xDest.doubleValue() + xSrc.doubleValue())/2.0;
                double dy = (yDest.doubleValue() + ySrc.doubleValue())/2.0;
                Hashtable atts = new Hashtable();
                atts.put(AttributeList.NODE_TARGET_XPOS,new Double(dx));
                atts.put(AttributeList.NODE_TARGET_YPOS,new Double(dy));
                _fgraph.setVertexAttributeList(_envName, atts);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //------------------------------------------------------------------
    private boolean commRelayEligible()
            throws FGraphException
    {
        //checking if there are acrive stream edges going thorugh this environment
        Enumeration enStreams = _fgraph.getOutgoingEdges (_envName,_edgeStreamFilter);
        if (enStreams.hasMoreElements()) {
            return false;           //not-elegible if currently relaying a stream...
        }

        //checking if any of the agents connected to this env have a pending (or active) str-request.
        Vector connectedAgents = listConnectedAgents ();
        for (int i=0; i<connectedAgents.size(); i++) {
            String localAgent = (String) connectedAgents.elementAt(i);
            Enumeration enOutReq = _fgraph.getOutgoingEdges (localAgent,_edgeRequestFilter);
            if (enOutReq.hasMoreElements()) {
                return false;       //a local agent has an outgoing stream request (pending or active)
            }
            Enumeration enInReq = _fgraph.getIncomingEdges (localAgent,_edgeRequestFilter);
            if (enInReq.hasMoreElements()) {
                return false;       //a local agent has an incoming stream request (pending or active)
            }
        }
        return true;
    }

    //------------------------------------------------------------------
    private Vector listConnectedAgents()
            throws FGraphException
    {
        Vector clientList = new Vector();
        Enumeration enMembEdges = _fgraph.getIncomingEdges(_envName, _edgeMembershipFilter);
        while (enMembEdges.hasMoreElements()) {
            clientList.addElement(_fgraph.getEdgeSource((String) enMembEdges.nextElement()));
        }
        return clientList;
    }

    private void debugMsg2 (String message)
    {
        System.out.println ("(ConnRecoveryMonitor)" + message);
    }

    private boolean _running;
    private FGraph _fgraph;
    private String _envName;
    private boolean _isMobile = false;
    private Hashtable _edgeRequestFilter;
    private Hashtable _edgeStreamFilter;
    private Hashtable _edgeMembershipFilter;
    private Hashtable _edgeReachabilityFilter;
}
