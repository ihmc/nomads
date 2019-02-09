package us.ihmc.aci.gss;

import java.net.URI;
import java.util.Hashtable;
import java.util.Vector;
import java.util.zip.CRC32;

import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.aci.AttributeList;
import us.ihmc.util.ConfigLoader;
                                        
/**
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
public class GSSThinClient
{
    private FGraph _fgraph = null;
    private String _clientID = null;
    private Hashtable _ht = new Hashtable();
        
    
    public GSSThinClient () throws Exception
    {
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
        String shost = cloader.getProperty("gss.server.default.host");
        int iport = cloader.getPropertyAsInt("gss.server.default.port");
        //_nodeRecords = new Hashtable();
        //_envRecords = new Hashtable();
        //_vtxList = new Vector();
        _fgraph = FGraph.getClient (new URI("tcp://" + shost + ":" + iport));
    }

    /**
     * FGraphThinClient Constructor.
     */
    public GSSThinClient (URI uri)  throws Exception
    {
        _fgraph = FGraph.getThinClient (uri);
    }

    /**
     * FGraphThinClient Constructor, that automatically registers
     * as a FGInfoVertex into FGraph.  Parameter vertexID becomes a class
     * variable and users can use methods without vertexID parameters
     * for this specific vertex.
     */
    public GSSThinClient (URI uri, String vertexID)  throws Exception
    {
        _clientID = vertexID;
        _fgraph = FGraph.getThinClient (uri);
        this.register(vertexID);
    }

    /**
     * FGraphClient Constructor.
     */
    public GSSThinClient (FGraph fgraph) throws Exception
    {
        _fgraph = fgraph;
    }
    
    /**
     * FGraphClient Constructor, that automatically registers
     * as a FGInfoVertex into FGraph.  Parameter vertexID becomes a class
     * variable and users can use methods without vertexID parameters
     * for this specific vertex.
     */
    public GSSThinClient (FGraph fgraph, String vertexID) throws Exception
    {
        _clientID = vertexID;
        _fgraph = fgraph;
        this.register(vertexID);
    }
    
    public void register (String nodeID)
    {
        try {
            _fgraph.addVertex(nodeID);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void registerWithAttributes (String vertexID, Hashtable attributes)
    {
        try {
            _fgraph.addVertex(vertexID, attributes);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void bindAgentToEnvironment (String agentName, String envName)
            throws Exception
    {
        Hashtable htAtt = new Hashtable ();
        htAtt.put (AttributeList.EDGE_TYPE, "MEMBERSHIP");
        CRC32 crc32 = new CRC32();
        crc32.update((agentName + ":" + envName).getBytes());
        String edgeID = "E" + crc32.getValue();
        _fgraph.addEdge (edgeID, agentName, envName, htAtt);
    }

    public void deregister (String vertexID)
    {
        try {
            _fgraph.removeVertex (vertexID);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * Sets FGraph vertex x (Side to Side of screen), y (altitude) and 
     * z (Into and out of screen) position.
     */
    synchronized public void setPosition (double x, double y, double z)
    {
        if (_clientID == null) {
            System.err.println("GSSClient: VertexID is required.");
            return;
        }
        
        try {
            _ht = _fgraph.getVertexAttributeList (_clientID);
            
            if (_ht.containsKey(AttributeList.NODE_XPOS)) {
                _ht.remove(AttributeList.NODE_XPOS);
            }
            _ht.put (AttributeList.NODE_XPOS, new Double (x)); 
            
            if (_ht.containsKey(AttributeList.NODE_YPOS)) {
                _ht.remove(AttributeList.NODE_YPOS);
            }
            _ht.put (AttributeList.NODE_YPOS, new Double (y));
            
            if (_ht.containsKey(AttributeList.NODE_ZPOS)) {
                _ht.remove(AttributeList.NODE_ZPOS);
            }
            _ht.put (AttributeList.NODE_ZPOS, new Double (z));           
            
            _fgraph.setVertexAttributeList (_clientID, _ht);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * Sets FGraph vertex heading, pitch and roll orientation.
     */
    synchronized public void setOrientation (int h, int p, int r)
    {
        if (_clientID == null) {
            System.err.println("GSSClient: VertexID is required.");
            return;
        }
        
        try {
            _ht = _fgraph.getVertexAttributeList (_clientID);
            
            if (_ht.containsKey(AttributeList.NODE_HEADING)) {
                _ht.remove(AttributeList.NODE_HEADING);
            }
            _ht.put (AttributeList.NODE_HEADING, new Integer (h)); 
            
            if (_ht.containsKey(AttributeList.NODE_PITCH)) {
                _ht.remove(AttributeList.NODE_PITCH);
            }
            _ht.put (AttributeList.NODE_PITCH, new Integer (p));
            
            if (_ht.containsKey(AttributeList.NODE_ROLL)) {
                _ht.remove(AttributeList.NODE_ROLL);
            }
            _ht.put (AttributeList.NODE_ROLL, new Integer (r));           
            
            _fgraph.setVertexAttributeList (_clientID, _ht);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    /**
     * Sets FGraph vertex x (Side to Side of screen), y (altitude) and 
     * z (Into and out of screen) position.
     */
    synchronized public void setPosition (String vertexID, double x, double y, double z)
    {
        try {
            _ht = _fgraph.getVertexAttributeList (vertexID);
            
            if (_ht.containsKey(AttributeList.NODE_XPOS)) {
                _ht.remove(AttributeList.NODE_XPOS);
            }
            _ht.put (AttributeList.NODE_XPOS, new Double (x)); 
            
            if (_ht.containsKey(AttributeList.NODE_YPOS)) {
                _ht.remove(AttributeList.NODE_YPOS);
            }
            _ht.put (AttributeList.NODE_YPOS, new Double (y));
            
            if (_ht.containsKey(AttributeList.NODE_ZPOS)) {
                _ht.remove(AttributeList.NODE_ZPOS);
            }
            _ht.put (AttributeList.NODE_ZPOS, new Double (z));           
            
            _fgraph.setVertexAttributeList (vertexID, _ht);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Sets FGraph vertex heading, pitch and roll orientation.
     */
    synchronized public void setOrientation (String vertexID, int h, int p, int r)
    {
        try {
            _ht = _fgraph.getVertexAttributeList (vertexID);
            
            if (_ht.containsKey(AttributeList.NODE_HEADING)) {
                _ht.remove(AttributeList.NODE_HEADING);
            }
            _ht.put (AttributeList.NODE_HEADING, new Integer (h)); 
            
            if (_ht.containsKey(AttributeList.NODE_PITCH)) {
                _ht.remove(AttributeList.NODE_PITCH);
            }
            _ht.put (AttributeList.NODE_PITCH, new Integer (p));
            
            if (_ht.containsKey(AttributeList.NODE_ROLL)) {
                _ht.remove(AttributeList.NODE_ROLL);
            }
            _ht.put (AttributeList.NODE_ROLL, new Integer (r));           
            
            _fgraph.setVertexAttributeList (vertexID, _ht);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

}