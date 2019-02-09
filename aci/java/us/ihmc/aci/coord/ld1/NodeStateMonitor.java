package us.ihmc.aci.coord.ld1;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.HashSet;
import java.util.Iterator;

import us.ihmc.aci.coord.NodeInfo;

import us.ihmc.aci.grpMgrOld.GroupManagerListener;

/**
 * StateMonitor.java
 *
 * Created on September 28, 2006, 10:06 PM
 *
 * @author  nsuri
 */
public class NodeStateMonitor implements GroupManagerListener
{
    public NodeStateMonitor (Coordinator coord, String groupName)
    {
        _coord = coord;
        _groupName = groupName;
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
            synchronized(_mutex) {
                System.out.println ("Dead Group Member " + memberUUID);
                if (_activeNodes.remove (memberUUID)) {
                    // A node that we knew about has disappeared
                    // Notify the coordinator in case something needs to be done
                    try {
                        // Put this in a try catch so that even if there is an exception,
                        // the rest of the logic in this method will execute, thereby
                        // ensuring the consistency of data structures
                        _coord.groupMemberDied (memberUUID);
                    }
                    catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                if (_nodes.containsKey (memberUUID)) {
                    // We do know about this node - so put it into the list of dead nodes
                    _deadNodes.add (memberUUID);
                }
                else {
                    System.out.println ("Internal logic error! - node " + memberUUID + " has left but is not in the nodes hashtable");
                }
            }
        }
    }

    public void newGroupMember (String groupName, String memberUUID, byte[] data)
    {
        if (groupName.equals (_groupName)) {
            synchronized (_mutex) {
                NodeInfo ni = (NodeInfo)_nodes.get (memberUUID);
                if (ni == null) {
                    // This is the first time we have heard about this node - create a new NodeInfo
                    System.out.println ("New Group Member " + memberUUID);
                    ni = new NodeInfo (memberUUID);
                    if (data != null) {
                        ni.parseNodeMonInfo (data);
                        System.out.println ("NodeStateMonitor: new node " + memberUUID + " has properties <" + ni.properties + ">");
                        //ni.display();
                    }
                    _nodes.put (memberUUID, ni);
                    _activeNodes.add (memberUUID);
                    try {
                        // Put this in a try catch so that even if there is an exception,
                        // the rest of the logic in this method will execute, thereby
                        // ensuring the consistency of data structures
                        _coord.newGroupMemberFound (memberUUID);
                    }
                    catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                else {
                    if (_deadNodes.remove (memberUUID)) {
                        System.out.println ("Revived Group Member " + memberUUID);
                        if (data != null) {
                            ni.parseNodeMonInfo (data);
                            System.out.println ("NodeStateMonitor: revived node " + memberUUID + " has properties <" + ni.properties + ">");
                            //ni.display();
                        }
                        _activeNodes.add (memberUUID);
                        try {
                            // Put this in a try catch so that even if there is an exception,
                            // the rest of the logic in this method will execute, thereby
                            // ensuring the consistency of data structures
                            _coord.groupMemberRevived (memberUUID);
                        }
                        catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    else {
                        // Node is already in the _nodes table and not in deadNodes - yet newGroupMember has been called!
                        System.out.println ("Internal logic error! - node " + memberUUID + " should have been in the deadNodes hashset");
                    }
                }
            }
        }
    }

    public void newPeer (String nodeUUID)
    {
        // Ignore this for now - handle newGroupMember
    }

    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] data)
    {
        if (groupName.equals (_groupName)) {
            synchronized(_mutex) {
                NodeInfo ni = (NodeInfo) _nodes.get (nodeUUID);
                if (ni == null) {
                    // Don't know anything about this node - consider displaying a warning
                }
                else if (data != null) {
                    ni.parseNodeMonInfo (data);
                    //System.out.println ("NodeStateMonitor: received update from node " + nodeUUID + "; properties = " + ni.properties);
                    _coord.peerGroupDataChanged (nodeUUID);
                    //ni.display();
                }
            }
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

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String searchUUI)
    {
        // Ignore this for now
    }

    public void lockNodesHashtable()
    {
        // Implement a lock here
    }
    
    public void unlockNodesHashtable()
    {
        // Unlock here
    }
    
    public NodeInfo getNodeInfo (String nodeUUID)
    {
        return (NodeInfo) _nodes.get(nodeUUID);
    }

    public Enumeration getNodes()
    {
        return _nodes.elements();
    }
    
    public Iterator getActiveNodes()
    {
        return _activeNodes.iterator();
    }

    private Coordinator _coord;
    private String      _groupName;
    private Object      _mutex = new Object();
    private Hashtable   _nodes = new Hashtable();       // Maps node UUID (String) to NodeInfo object
    private HashSet     _activeNodes = new HashSet();   // Contains UUIDs of nodes that are still active (reachable)
    private HashSet     _deadNodes = new HashSet();     // Contains UUIDs of nodes that are no longer active
}
