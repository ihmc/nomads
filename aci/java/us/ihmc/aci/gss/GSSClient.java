package us.ihmc.aci.gss;

import java.net.URI;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;
import java.util.zip.CRC32;

import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphEventListener;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.AttributeList;

/**
 * GSSClient.java
 * GSSClient.java
 * Global System State (GSS) Client
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @author Christopher Eagle (ceagle@ihmc.us)
 * @version $Revision$
 *          Created on May 17, 2004 at 4:06:38 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class GSSClient implements FGraphEventListener
{
    public GSSClient () throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String shost = cloader.getProperty("gss.server.default.host");
        int iport = cloader.getPropertyAsInt("gss.server.default.port");
        _nodeRecords = new Hashtable();
        _envRecords = new Hashtable();
        _vtxList = new Vector();
        _fgraph = FGraph.getClient (new URI("tcp://" + shost + ":" + iport));
        _fgraph.setEcho(true);
        _fgraph.addGraphEventListener(this);
    }

    /**
     * FGraphClient Constructor.
     */
    public GSSClient (URI uri)  throws Exception
    {
        _fgraph = FGraph.getClient (uri);
    }

    /**
     * FGraphClient Constructor.
     */
    public GSSClient (FGraph fgraph) throws Exception
    {
        _fgraph = fgraph;
    }
    
    public void addEventListener (FGraphEventListener eventListener)
    {
        _fgraph.addGraphEventListener (eventListener);
    }

    public void removeEventListener (FGraphEventListener eventListener)
    {
        _fgraph.removeGraphEventListener (eventListener);
    }

    public synchronized void registerAgent (String nodeID)
            throws Exception
    {
        Hashtable htAtt = new Hashtable ();
        htAtt.put(AttributeList.NODE_TYPE, "AGENT");
        _fgraph.addVertex(nodeID, htAtt);
        while (!_vtxList.contains(nodeID)) {
            Thread.sleep(50);
        }
        _nodeRecords.put (nodeID, new NodeRegRecord(nodeID, null, htAtt));
    }

    public synchronized void registerAgentWithAttributes (String nodeID, Hashtable attributes)
            throws Exception
    {
        debugMsg ("Registering agent (" + nodeID + ") with attributes");
        if (attributes == null) {
            attributes = new Hashtable();
        }
        attributes.put(AttributeList.NODE_TYPE, "AGENT");
        _fgraph.addVertex(nodeID, attributes);
        while (!_vtxList.contains(nodeID)) {
            Thread.sleep(50);
        }
        _nodeRecords.put (nodeID, new NodeRegRecord(nodeID, null, attributes));
    }

    public synchronized void register (String nodeID)
            throws Exception
    {
        debugMsg ("Registering node (Unknown Type) (" + nodeID + ")");
        _fgraph.addVertex(nodeID);
        while (!_vtxList.contains(nodeID)) {
            Thread.sleep(50);
        }
        _nodeRecords.put (nodeID, new NodeRegRecord(nodeID, null, null));
    }

    public synchronized void registerEnvironment (String nodeID)
            throws Exception
    {
        Hashtable htAtt = new Hashtable ();
        htAtt.put(AttributeList.NODE_TYPE, "ENVIRONMENT");
        registerEnvironmentWithAttributes (nodeID, null);
        _envRecords.put (nodeID, new EnvRegRecord(nodeID, htAtt));
    }

    public synchronized void registerEnvironmentWithAttributes (String nodeID, Hashtable attributes)
            throws Exception
    {
        debugMsg ("Registering environment (" + nodeID + ") with attributes");
        if (attributes == null) {
            attributes = new Hashtable();
        }
        attributes.put(AttributeList.NODE_TYPE, "ENVIRONMENT");
       _fgraph.addVertex(nodeID, attributes);
        while (!_vtxList.contains(nodeID)) {
            System.out.print("r");
            Thread.sleep(50);
        }
        _envRecords.put (nodeID, new EnvRegRecord(nodeID, attributes));
   }

    public synchronized void setNodeAttributes (String nodeName, Hashtable attributes)
    {
        debugMsg ("Setting Node Attributes to (" + nodeName + ")");
        EnvRegRecord envRecord = (EnvRegRecord) _envRecords.get(nodeName);
        if (envRecord != null) {
            if (envRecord._envAtts == null) {
                envRecord._envAtts = new Hashtable();
            }
            Enumeration en = attributes.keys();
            while (en.hasMoreElements()) {
                String skey = (String) en.nextElement();
                debugMsg ("Setting attribute (" +  skey + ") to environment " + nodeName);
                envRecord._envAtts.put(skey, attributes.get(skey));
            }

            try {
                _fgraph.setVertexAttributeList(nodeName, attributes);
                _envRecords.put(nodeName, envRecord);
            }
            catch (Exception e) {
                debugMsg ("Failed to set Attributes to node (" + nodeName + "): " + e.getMessage());
            }
        }
    }

    public synchronized void bindAgentToEnvironment (String agentName, String envName)
            throws Exception
    {
        debugMsg ("Binding Agent (" + agentName + ") to environment (" + envName + ")");
        String envType = (String) _fgraph.getVertexAttribute(envName, AttributeList.NODE_TYPE);
        if (envType.compareTo("ENVIRONMENT") != 0) {
            throw new Exception ("Node " + envName + " is NOT an ENVIRONEMNT");
        }
        String nodeType = (String) _fgraph.getVertexAttribute(agentName, AttributeList.NODE_TYPE);
        if (nodeType.compareTo("AGENT") != 0) {
            throw new Exception ("Node " + agentName + " is NOT an AGENT");
        }

        Hashtable htAtt = new Hashtable ();
        htAtt.put (AttributeList.EDGE_TYPE, "MEMBERSHIP");
        CRC32 crc32 = new CRC32();
        crc32.update((agentName + ":" + envName).getBytes());
        String edgeID = "E" + crc32.getValue();
        _fgraph.addEdge (edgeID, agentName, envName, htAtt);

        NodeRegRecord nodeRec = (NodeRegRecord) _nodeRecords.get(agentName);
        if (nodeRec != null) {
            nodeRec._envName = envName;
        }
    }

    public Enumeration getAgentsConnectedToEnvironment (String envName)
            throws Exception
    {
        Vector agentIDs = new Vector();
        Hashtable filter = new Hashtable ();
        filter.put (AttributeList.EDGE_TYPE, "MEMBERSHIP");
        Enumeration en = _fgraph.getIncomingEdges(envName, filter);
        while (en.hasMoreElements()) {
            String membEdgeID = (String) en.nextElement();
            String agentID = _fgraph.getEdgeSource(membEdgeID);
            String agentType = (String) _fgraph.getVertexAttribute(agentID, AttributeList.NODE_TYPE);
            if (agentType.compareTo("AGENT")==0) {
                agentIDs.addElement(agentID);
            }
        }
        return (agentIDs.elements());
    }

    public String[] getConnectedEnvironments(String envID)
            throws Exception
    {
        Vector envList = new Vector();
        Hashtable edgeFilter = new Hashtable ();
        edgeFilter.put (AttributeList.EDGE_TYPE, "RECHEABILITY");
        Hashtable vtxFilter = new Hashtable ();
        vtxFilter.put (AttributeList.NODE_TYPE, "ENVIRONMENT");
        Enumeration outLinks = _fgraph.getOutgoingEdges(envID);
        while (outLinks.hasMoreElements()) {
            String edgeID = (String) outLinks.nextElement();
            String targetID = _fgraph.getEdgeSource(edgeID);
            if (envList.contains(targetID)) {
                envList.addElement(targetID);
            }
        }
        Enumeration inLinks = _fgraph.getIncomingEdges(envID);
        while (inLinks.hasMoreElements()) {
            String edgeID = (String) inLinks.nextElement();
            String sourceID = _fgraph.getEdgeSource(edgeID);
            if (envList.contains(sourceID)) {
                envList.addElement(sourceID);
            }
        }

        String[] nlist = new String[envList.size()];
        for (int i=0; i<envList.size(); i++) {
            nlist[i] = (String) envList.elementAt(i);
        }
        return (nlist);
    }

    public void deregister (String nodeID)
            throws Exception
    {
        _fgraph.removeVertex (nodeID);
    }

    public boolean isNodeRegistered (String nodeID)
            throws Exception
    {
        return (_fgraph.hasVertex(nodeID));
    }

    public boolean isAgentRegisteredWithEnvironment (String agntName, String envName)
            throws Exception
    {
        if (!_fgraph.hasVertex(agntName)) {
            return false;
        }
        String envID = getEnvironmentID (agntName);
        if (envID == null || envID.compareTo(envName)!=0) {
            return false;
        }
        Hashtable filter = new Hashtable ();
        filter.put (AttributeList.EDGE_TYPE, "MEMBERSHIP");
        Enumeration en = _fgraph.getIncomingEdges(envName, filter);
        while (en.hasMoreElements()) {
            String source = _fgraph.getEdgeSource((String)en.nextElement());
            if (source.compareToIgnoreCase(agntName)==0) {
                return true;
            }
        }
        return false;
    }

    public FGraph getFGraph ()
    {
        return (_fgraph);
    }

    public String getEnvironmentID (String agentName)
            throws Exception
    {
        String env = null;
        Hashtable htFilter = new Hashtable();
        htFilter.put (AttributeList.EDGE_TYPE, "MEMBERSHIP");
        Enumeration en = _fgraph.getOutgoingEdges (agentName, htFilter);
        while (en.hasMoreElements()) {
            String edgeID =(String) en.nextElement();
            env = _fgraph.getEdgeTarget (edgeID);
        }
        return (env);
    }

    public Object getEnvironmentAttribute (String environmentID, String attributeName)
            throws Exception
    {
        String envType = (String) _fgraph.getVertexAttribute(environmentID, AttributeList.NODE_TYPE);
        if (envType.compareTo("ENVIRONMENT")!=0) {
            throw new Exception ("Node " + envType + " is not registered as an ENVIRONMENT");
        }
        return (_fgraph.getVertexAttribute(environmentID, attributeName));
    }

    public Enumeration getEnvironmentList ()
            throws Exception
    {
        Hashtable filter = new Hashtable();
        filter.put(AttributeList.NODE_TYPE,"ENVIRONMENT");
        return (_fgraph.getVertices(filter));
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[GSSClient] " + msg);
        }
    }

    public void vertexAdded(String vertexID)
    {
        _vtxList.addElement(vertexID);
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        _vtxList.remove(vertexID);
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
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
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
        debugMsg ("Got (connection lost) warning");
        _lostConnection = true;
    }

    public void connected()
    {
        debugMsg ("Got (connected) warning");
        if (_lostConnection) {
            registerAllElements();
        }
    }

    private void registerAllElements()
    {
        //Re-Registering all the environments.....
        Enumeration en = _envRecords.elements();
        while (en.hasMoreElements()) {
            EnvRegRecord envRec = (EnvRegRecord) en.nextElement();
            try {
                debugMsg ("*** Re-Registering ENVIRONMENT (" + envRec._envName + ")");
                registerEnvironmentWithAttributes (envRec._envName, envRec._envAtts);
            }
            catch (Exception e) {
                System.out.println("Failed to register environment (" + envRec._envName + ")");
            }
        }

        en = _nodeRecords.elements();
        while (en.hasMoreElements()) {
            NodeRegRecord nodeRec = (NodeRegRecord) en.nextElement();
            if (nodeRec._att != null) {
                try {
                    debugMsg ("*** Re-Registering NODE (" + nodeRec._nodeName + ")");
                    registerAgentWithAttributes (nodeRec._nodeName, nodeRec._att);
                    if (nodeRec._envName != null) {
                        debugMsg ("*** Re-biding NODE (" + nodeRec._nodeName + ") to ENVIRONMENT (" + nodeRec._envName + ")");
                        bindAgentToEnvironment (nodeRec._nodeName,  nodeRec._envName);
                    }
                }
                catch (Exception e) {
                    System.out.println("Failed to register agent (" + nodeRec._nodeName + ")");
                }
            }
            else {
                try {
                    debugMsg ("*** Re-Registering NODE (" + nodeRec._nodeName + ") (NOT BOUND)");
                    register (nodeRec._nodeName);
                }
                catch (Exception e) {
                    System.out.println("Failed to register agent (" + nodeRec._nodeName + ")");
                }
            }
        }
    }

    ////////////////////// Inner Class //////////////////////////
    class EnvRegRecord
    {
        EnvRegRecord (String envName, Hashtable envAtts)
        {
            _envName = envName;
            _envAtts = envAtts;
        }
        protected String _envName;
        protected Hashtable _envAtts;
    }

    class NodeRegRecord
    {
        NodeRegRecord (String nodeName, String envName, Hashtable att)
        {
            _nodeName = nodeName;
            _envName = envName;
            _att = att;
        }

        protected String _nodeName;
        protected String _envName;
        protected Hashtable _att;
    }

    private Hashtable _nodeRecords;
    private Hashtable _envRecords;

    private Vector _vtxList;
    private boolean _lostConnection = false;
    private boolean _debug = true;
    private FGraph _fgraph = null;
    private Hashtable _ht = new Hashtable();
}