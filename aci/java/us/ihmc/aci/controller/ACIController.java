/**
 * ACIController
 * 
 * @author      Marco Arguedas
 * 
 * @version     $Revision$
 *              $Date$
 */
 
package us.ihmc.aci.controller;
 
import us.ihmc.ds.graph.*;
import java.util.List;

/**
 * 
 */ 
public class ACIController
{
    public ACIController (Graph graph)
    {
        _graph = graph;
    }
    
    /**
     * 
     */
    public void addNode (String nodeID)
    {
        if (_graph.containsKey(nodeID)) {
            log ("addNode:: Node with id [ " + nodeID + "] already present in the graph.");
            return;
        }
    }
    
    /**
     * 
     */
    public void addReachabilityEdge (String sourceNodeID, String destNodeID)
    {
        Node sourceNode = _graph.get (sourceNodeID);
        Node destNode = _graph.get (destNodeID);
        
        if (sourceNode == null) {
            sourceNode = _graph.put (sourceNodeID);
        }
        if (destNode == null) {
            destNode = _graph.put (destNodeID);
        }
        
        Node edge = _graph.put();
        edge.put (EDGE_NAME_KEY, computeEdgeName(sourceNodeID, destNodeID));
        edge.put (EDGE_TYPE_KEY, "EDGE_REACHABILITY");
        
        sourceNode.put (edge, destNode);
    }
    
    /**
     * 
     */
    public void addActiveService (String nodeID, String serviceName, String instanceUUID)
    {
        Node sourceNode = _graph.get (nodeID);
        if (sourceNode == null) {
            sourceNode = _graph.put (nodeID);
        }
        
        Node activeServices = sourceNode.getNode (NODE_ACTIVE_SERVICES_KEY);
        if (activeServices == null) {
            activeServices = sourceNode.put (NODE_ACTIVE_SERVICES_KEY);
        }
        
        List servicesList = activeServices.list();
        Node service = _graph.put();
        service.put (SERVICE_NAME_KEY, serviceName);
        service.put (SERVICE_INSTANCE_UUID_KEY, instanceUUID);
        
        servicesList.add (service);
    }
    
    /**
     * 
     */
    public void removeNode (String nodeID)
    {
        if (!_graph.containsKey(nodeID)) {
            log ("removeNode:: Node with id [ " + nodeID + "] not present in the graph.");
        }
        
        _graph.remove (nodeID);
    }

    /**
     * 
     */    
    public void updateNodePosition (String nodeID, double xPos, double yPos)
    {
        Node node = _graph.get (nodeID);
        if (node == null) {
            node = _graph.put (nodeID);
        }

        node.put (NODE_ATTR_X_POS, xPos);
        node.put (NODE_ATTR_Y_POS, yPos);
    }
    
    /**
     * 
     */
    public void updateNodeCPUUsage (String nodeID, double usage)
    {
        Node node = _graph.get (nodeID);
        if (node == null) {
            node = _graph.put (nodeID);
        }

        node.put (NODE_ATTR_CPU_USAGE, usage);
    }
    
    /**
     * 
     */
    public String computeEdgeName (String sourceNodeID, String destNodeID)
    {
        return sourceNodeID + "#" + destNodeID;
    }
    
    /**
     * 
     */
    private void log (String msg)
    {
        if (DEBUG) {
            System.out.println ("[ACIController] " + msg);
        }
    }
    
 
 
    // /////////////////////////////////////////////////////////////////////////
    public static String EDGE_TYPE_KEY              = "EDGE_TYPE";
    public static String EDGE_NAME_KEY              = "EDGE_NAME";

    public static String SERVICE_NAME_KEY           = "SERVICE_NAME";
    public static String SERVICE_INSTANCE_UUID_KEY  = "SERVICE_INSTANCE_UUID";

    public static String NODE_ATTR_X_POS            = "X_POS";
    public static String NODE_ATTR_Y_POS            = "Y_POS";
    public static String NODE_ATTR_CPU_USAGE        = "CPU_USAGE";

    public static String NODE_ACTIVE_SERVICES_KEY   = "ACIK_ACTIVE_SERVICES";
   // /////////////////////////////////////////////////////////////////////////
    private static final boolean DEBUG = true;

    private Graph _graph = null;

    // /////////////////////////////////////////////////////////////////////////
    // MAIN METHOD. for testing purposes ///////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     * 
     */
    public static void main (String[] args)
    {
    }
}