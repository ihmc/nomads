package us.ihmc.aci.vis;

import java.io.Serializable;
import java.util.Enumeration;
import java.util.Hashtable;
import us.ihmc.aci.coord.NodeInfo;
import us.ihmc.aci.coord.resvis.ResourceVisualizer;
import us.ihmc.aci.grpMgrOld.GroupManager;
import us.ihmc.aci.grpMgrOld.GroupManagerListener;
import us.ihmc.ds.fgraph.FGraphException;
import us.ihmc.ds.fgraph.FGraphLocal;

import us.ihmc.sync.Mutex;

/**
 * NodeStateMonitor.java
 *
 * Created on September 28, 2006, 10:06 PM
 *
 * @author  nsuri
 */
public class NodeStateMonitor
    implements GroupManagerListener
{
    public NodeStateMonitor (GroupManager gm, String groupName)
    {
        _groupName = groupName;
        _groupManager = gm;
        _resVis = new ResourceVisualizer();
    }

    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {
        System.out.println ("conflict with restricted group " + groupName + " with node " + nodeUUID);
    }

    public void deadPeer (String nodeUUID)
    {
        // Ignore this for now - handle groupMemberLeft instead
    }

    public void groupListChange (String nodeUUID)
    {
        // Ignore this for now
    }

    public void groupMemberLeft (String groupName, String memberUUID)
    {
        if (groupName.equals (_groupName)) {
            _m.lock();
            try {
                System.out.println ("Dead Group Member " + memberUUID);
                _resVis.deadPeer (memberUUID);
                if (_stateInfo.hasVertex (memberUUID)) {
                    if (_stateInfo.getVertexAttribute (memberUUID, "active") != null) {
                        // A previously active node has disappeared
                        // Remove the active attribute
                        _stateInfo.removeVertexAttribute (memberUUID, "active");
                        // Consider notifying the coordinator in case something needs to be done
                    }
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            _m.unlock();
        }
    }

    public void newGroupMember (String groupName, String memberUUID, byte[] data)
    {
        if (groupName.equals (_groupName)) {
            _m.lock();
            System.out.println ("New Group Member " + memberUUID);
            try {
                NodeInfo ni = null;
                if (_stateInfo.hasVertex (memberUUID)) {
                    ni = (NodeInfo) _stateInfo.getVertexAttribute (memberUUID, "nodeInfo");
                    if (ni == null) {
                        // Know about this node but the node does not have the node info - so add one
                        ni = new NodeInfo (memberUUID);
                        _stateInfo.setVertexAttribute (memberUUID, "nodeInfo", ni);
                    }
                }
                else {
                    // This is the first time we have heard about this node - create a new Vertex and a NodeInfo
                    _stateInfo.addVertex (memberUUID);
                    ni = new NodeInfo (memberUUID);
                    _stateInfo.setVertexAttribute (memberUUID, "nodeInfo", ni);
                }
                _stateInfo.setVertexAttribute (memberUUID, "active", "true");
                _resVis.newPeer (_groupManager.getPeerNodeName (memberUUID), memberUUID);
                if (data != null) {
                    ni.parseNodeMonInfo (data);
                    _resVis.updateNodeInfo (memberUUID, ni);
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            _m.unlock();
        }
    }

    public void newPeer (String nodeUUID)
    {
        // Ignore this for now - handle newGroupMember
    }

    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] data)
    {
        if (groupName.equals (_groupName)) {
            _m.lock();
            try {
                NodeInfo ni = null;
                if (_stateInfo.hasVertex (nodeUUID)) {
                    ni = (NodeInfo) _stateInfo.getVertexAttribute (nodeUUID, "nodeInfo");
                    if (ni == null) {
                        // Don't know anything about this node - consider displaying a warning
                    }
                    else if (data != null) {
                        ni.parseNodeMonInfo (data);
                        _resVis.updateNodeInfo (nodeUUID, ni);
                    }
                }
                else {
                    // Don't know anything about this node - consider displaying a warning
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            _m.unlock();
        }
    }

    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        // Ignore this for now
    }

    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        // Ignore this for now
    }

    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
        // Ignore this for now
    }

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String peerSearchUUID)
    {
    }

    public void lock()
    {
        _m.lock();
    }

    public void unlock()
    {
        _m.unlock();
    }
    
    public boolean nodeExists (String nodeUUID)
    {
        if (_stateInfo.hasVertex (nodeUUID)) {
            return true;
        }
        return false;
    }

    public boolean addNode (String nodeUUID)
    {
        try {
            _stateInfo.addVertex (nodeUUID);
        }
        catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public Object getNodeAttribute (String nodeUUID, String attribute)
    {
        try {
            if (_stateInfo.hasVertex (nodeUUID)) {
                return _stateInfo.getVertexAttribute (nodeUUID, attribute);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public String getNodeAttributeAsString (String nodeUUID, String attribute)
    {
        try {
            if (_stateInfo.hasVertex (nodeUUID)) {
                return (String) _stateInfo.getVertexAttribute (nodeUUID, attribute);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public NodeInfo getNodeInfo (String nodeUUID)
    {
        try {
            if (_stateInfo.hasVertex (nodeUUID)) {
                return (NodeInfo) _stateInfo.getVertexAttribute (nodeUUID, "nodeInfo");
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public Enumeration getNodeUUIDs()
    {
        return _stateInfo.getVertices();
    }

    public Enumeration getActiveNodeUUIDs()
    {
        Hashtable filter = new Hashtable();
        filter.put ("active", "true");      // NOTE: The value "true" is immaterial
        return _stateInfo.getVertices (filter);
    }

    public boolean setNodeAttribute (String nodeUUID, String attribute, Serializable value)
    {
        try {
            if (_stateInfo.hasVertex (nodeUUID)) {
                _stateInfo.setVertexAttribute (nodeUUID, attribute, value);
                return true;
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    public Object removeNodeAttribute (String nodeUUID, String attribute)
    {
        try {
            return _stateInfo.removeVertexAttribute (nodeUUID, attribute);
        }
        catch (FGraphException e) {
            return null;
        }
    }

    //public String getDest()
    //{
    //    return _dest;
    //}

    //public void setDest(String dest)
    //{
    //    this._dest = dest;
    //}

    //public String getSource()
    //{
    //    return _source;
    //}

    //public void setSource(String source)
    //{
    //    this._source = source;
    //}

    private static final int DEFAULT_NETWORK_REACHABILITY = 200;

    private String                _groupName;
    private GroupManager          _groupManager;
    //private String      _source, _dest;
    private Mutex                 _m = new Mutex();
    private FGraphLocal           _stateInfo = new FGraphLocal();
    private ResourceVisualizer    _resVis;
}
