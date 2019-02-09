package us.ihmc.aci.coord.ld2;

import java.io.IOException;

import java.util.Arrays;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Vector;

import us.ihmc.aci.agserve.HTTPAgServeAdminClient;
import us.ihmc.aci.coord.NodeInfo;
import us.ihmc.aci.kernel.LocalCoordinator;
import us.ihmc.aci.util.kernel.Locator;
import us.ihmc.aci.kernel.ServiceManager;
import us.ihmc.aci.kernel.KernelProxy;

import us.ihmc.manet.xlayer.XLayerLink;

import us.ihmc.util.ConfigManager;

/**
 * Initial implementation of the Distributed Local Coordinator
 *
 * Currently all operations are done in the local node
 *
 * @author mrebeschini
 */
public class Coordinator implements LocalCoordinator
{
    public Coordinator()
    {
    }

    /**
     * @param serviceManager the ServiceManager
     * @param kernelProxy the KernelProxy object which allows the Coordinator to interact with the ACI Kernel
     */
    public void init (ServiceManager serviceManager, KernelProxy kernelProxy)
    {
        try {
            _serviceManager = serviceManager;
            _kernelProxy = kernelProxy;
            initConfigManager();
            _localNodeUUID = _serviceManager.getGroupManagerProxy().getNodeUUID();
            _localNodeProperties = _kernelProxy.getNodeProperties();
            _localNodeInfo = new NodeInfo (_localNodeUUID);
            _nsm = new NodeStateMonitor (this, _cfgManager.getValue (ACI_GROUP_NAME_KEY));
            _serviceManager.getGroupManagerProxy().addListener (_nsm);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public synchronized String activateServiceRequest (String requestorURI, String serviceName, boolean onLocalNodeOnly)
        throws Exception
    {
        System.out.println ("Coordinator: activate service request for service " + serviceName);
      
        // Query the resource utilization database of each active node.
        String[] _nodes = getNodesOrderedByCPUAvailability();
        for (int i = 0; i < _nodes.length; i++) {
            String nodeProperties = _nsm.getNodeInfo (_nodes[i]) . properties;
            if ((nodeProperties == null) || (nodeProperties.indexOf ("client") < 0)) {
            	 HTTPAgServeAdminClient _adminClient = new HTTPAgServeAdminClient();
            	 float res = _adminClient.getResourceInfoRemotely(_nodes[i], 1, serviceName, "public java.math.BigInteger[] us.ihmc.aci.service.factorizationService.FactorizationService.getFactorizationResult()");
            	 System.out.println (" Node: " + _nodes[i]);
            	 System.out.println (" Service Name: " + serviceName);
            	 System.out.println (" Info: " + res);
            }
        }
        
        if (onLocalNodeOnly) {
            String serviceUUID = _kernelProxy.activateService (serviceName, KernelProxy.SCT_Undefined); //use the default VMContainer
            _activeServices.put (serviceUUID, new ActiveService (serviceUUID, _localNodeUUID, serviceName, true));
            System.out.println ("Coordinator: Activated service " + serviceName + " locally; service UUID = " + serviceUUID);
            return "acil://" + serviceUUID + "|" + _localNodeUUID + "|";
        }
        else {
            String[] nodes = getNodesOrderedByCPUAvailability();
            System.out.print ("Nodes ordered by CPU availability:");
            for (int i = 0; i < nodes.length; i++) {
                System.out.print (" " + nodes[i]);
            }
            System.out.println();
            Vector netTopology = null; //_kernelProxy.getNetworkTopology();
            if (netTopology == null) {
                System.out.println ("Network topology information is not known");
            }
            else {
                System.out.println ("Network topology:");
                for (int i = 0; i < netTopology.size(); i++) {
                    XLayerLink link = (XLayerLink) netTopology.elementAt (i);
                    System.out.println (link);
                }
            }
            String selectedNode = null;
            for (int i = 0; i < nodes.length; i++) {
                String nodeProperties = _nsm.getNodeInfo (nodes[i]) . properties;
                if ((nodeProperties == null) || (nodeProperties.indexOf ("client") < 0)) {
                    // Found a non-client node - use it
                    selectedNode = nodes[i];
                    System.out.println ("Coordinator: selected node " + selectedNode + " with properties " + nodeProperties);
                    break;
                }
                else {
                    System.out.println ("Coordinator: skipping node " + nodes[i] + " with properties " + nodeProperties);
                }
            }
            if (selectedNode != null) {
                HTTPAgServeAdminClient adminClient = new HTTPAgServeAdminClient();
                String serviceURI = adminClient.activateServiceRemotely (selectedNode, serviceName);
                Locator serviceLoc = new Locator (serviceURI);
                _activeServices.put (serviceLoc.getInstanceUUID(), new ActiveService (serviceLoc.getInstanceUUID(), serviceLoc.getNodeUUID(), serviceName, false));
                System.out.println ("Coordinator: Activated service " + serviceName + " on node " + serviceLoc.getNodeUUID() + "; service UUID = " + serviceLoc.getInstanceUUID());
                return serviceURI;
            }
            else {
                // No other nodes running: activating service in the local node
                System.out.println ("Coordinator: no other node found - activating service on local node");
                String serviceUUID = _kernelProxy.activateService (serviceName);
                _activeServices.put (serviceUUID, new ActiveService (serviceUUID, _localNodeUUID, serviceName, true));
                return "acil://" + serviceUUID + "|" + _localNodeUUID + "|";
            }
        }
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public synchronized boolean deactivateServiceInstanceRequest (String requestorURI, String serviceInstanceUUID)
        throws Exception
    {
        ActiveService as = (ActiveService) _activeServices.get (serviceInstanceUUID);
        if (as != null) {
            if (as.onLocalNode) {
                _kernelProxy.deactivateService (serviceInstanceUUID);
                _activeServices.remove (serviceInstanceUUID);
            }
            else {
                return (new HTTPAgServeAdminClient()).deactivateServiceRemotely (as.nodeUUID, serviceInstanceUUID);
            }
        return true;
        }
        else {
            System.out.println ("Coordinator: Could not find service instance " + serviceInstanceUUID + " in active services table");
            return false;
        }
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public synchronized String lookupServiceInstanceRequest (String requestorURI, String instanceUUID, String oldNodeUUID)
        throws Exception
    {
        System.out.println ("Coordinator: looking for service " + instanceUUID + " [oldNodeUUID: " + oldNodeUUID + "]");
        ActiveService as = (ActiveService) _activeServices.get (instanceUUID);
        if ((as != null) && (((oldNodeUUID != null) && (!oldNodeUUID.equals(as.nodeUUID))))) {
        	return as.nodeUUID;
        }
        else {
	        String nodeUUID = _kernelProxy.locateServiceInstance (instanceUUID);
	        _activeServices.put (instanceUUID, new ActiveService(instanceUUID, nodeUUID, null, false));
	        System.out.println ("Coordinator: located service on node: " + nodeUUID);
	        return nodeUUID;
        }
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public boolean allowRestoreServiceInstance (String requestorURI, String sourceURI, String instanceUUID) throws Exception
    {
        System.out.println ("Coordinator: request to restore a service instance " + instanceUUID + " from " + requestorURI + "; service migrating from " + sourceURI);
        return true;
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public void serviceRestored (String serviceName, String instanceUUID)
    {
        System.out.println ("Coordinator: service " + instanceUUID + " + restored");
        _activeServices.put (instanceUUID, new ActiveService (instanceUUID, _localNodeUUID, serviceName, true));
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public void kernelSuspending()
        throws Exception
    {
        System.out.println ("Coordinator: kernel being suspended");
        migrateAllServicesToOtherNode();
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public void kernelResumed()
        throws Exception
    {
        System.out.println ("Coordinator: kernel resumed");
    }

    /**
     * @see us.ihmc.aci.kernel.LocalCoordinator interface
     */
    public void nodeInfoUpdated (byte[] nodeInfoData)
        throws Exception
    {
        _localNodeInfo.parseNodeMonInfo (nodeInfoData);
/*
        System.out.println ("Coordinator.nodeInfoUpdated(): cpuUtilizationACI = " + _localNodeInfo.cpuUtilizationACI + 
                             ", cpuUtilizationOther = " + _localNodeInfo.cpuUtilizationOther);
*/
        if (_localNodeInfo.cpuUtilizationOther > NODE_CPU_UTIL_THRESHOLD && _activeServices.size() > 0) {
            System.out.println ("Other CPU utilization > " + NODE_CPU_UTIL_THRESHOLD + " and one or more active services");
            if (_localNodeInfo.properties == null) {
                // This is an opportunistic node - and other processes are using a significant portion of the CPU time
                // So - migrate services out of this node
                System.out.println ("Migrating services to an other node");
                migrateAllServicesToOtherNode();
            }
        }
    }

    void newGroupMemberFound (String memberUUID)
    {
        exploitNode (memberUUID);
    }
    
    void groupMemberRevived (String memberUUID)
    {
        exploitNode (memberUUID);
    }
    
    void peerGroupDataChanged (String memberUUID)
    {
        exploitNode (memberUUID);
    }

    void groupMemberDied (String memberUUID)
    {
        // Do nothing for now
    }

    private void initConfigManager()
        throws Exception
    {
        // Initialize the ConfigManager
        try {
            _cfgManager = new ConfigManager (_kernelProxy.getConfigFilePath());
        }
        catch (IOException e) {
            System.out.println ("Could not initialize ConfigManager using path <" + _kernelProxy.getConfigFilePath() + ">; Exception = " + e);
            throw e;
        }
        if (!_cfgManager.hasValue (ACI_GROUP_NAME_KEY)) {
            throw new Exception ("Config file <" + _kernelProxy.getConfigFilePath() + "> does not contain setting for <" + ACI_GROUP_NAME_KEY + ">");
        }
    }

    private String[] getNodesOrderedByCPUAvailability()
    {
        Vector v = new Vector();
        Iterator it = _nsm.getActiveNodes();
        while (it.hasNext()) {
            v.add (it.next());
        }
        String[] nodes = new String [v.size()];
        v.toArray (nodes);
        Arrays.sort (nodes, new ComparatorForNodesByCPU (_nsm));

        return nodes;
    }
    
    private void updateNetworkTopology()
    {
        _networkTopology = null; //_kernelProxy.getNetworkTopology();
    }

    private synchronized void exploitNode (String nodeUUID)
    {
    	NodeInfo nodeInfo = _nsm.getNodeInfo (nodeUUID);
        //System.out.println ("Coordinator: examining node " + nodeUUID + " with properties " + nodeInfo.properties);
        if ((nodeInfo.properties != null) && (nodeInfo.properties.indexOf("client") >= 0)) {
            // The new node is a client node - ignore
            System.out.println ("Coordinator: not exploiting client node " + nodeUUID);
            return;
        }
        else {
            // NOTE: Want to predicate this behavior on the number of services already running on the
            //       new node (get that via the group manager peer data) and/or the CPU availability

            int localServiceCountThreshold = 1;    // Migrate if more than this many instances are running locally
            if ((_localNodeProperties != null) && (_localNodeProperties.indexOf ("client") >= 0)) {
                // We are a client node - migrate any instances
                localServiceCountThreshold = 0;
            }
            
            // exploit a node only if the node's overall CPU utilization is less than EXPLOITABLE_NODE_CPU_UTIL_THRESHOLD
            // or if the node is a server node and the local node is an opportunistic node
            
            if (((nodeInfo.cpuUtilizationACI + nodeInfo.cpuUtilizationOther) <= EXPLOITABLE_NODE_CPU_UTIL_THRESHOLD) || 
                ((_localNodeProperties == null) && ((nodeInfo.properties != null) && (nodeInfo.properties.indexOf("server") >= 0)))) {
                Enumeration e = _activeServices.keys();
	            while (e.hasMoreElements()) {
	                String instanceUUID = (String) e.nextElement();
	                ActiveService as = (ActiveService) _activeServices.get (instanceUUID);
	                if (as.onLocalNode) {
	                    // A locally running service has been found
	                    if (localServiceCountThreshold > 0) {
	                        localServiceCountThreshold--;
	                    }
	                    else {
	                        // Found more services locally than desirable - try to migrate
	                        try {
	                            System.out.println ("Coordinator: found a locally running service " + instanceUUID + " to migrate to node " + nodeUUID);
	                            _kernelProxy.migrateService (as.serviceUUID, nodeUUID);
	                            _activeServices.remove (instanceUUID);
	                            System.out.println ("Coordinator: successfully migrated service to node " + nodeUUID);
	                            return;
	                        }
	                        catch (Exception ex) {
	                            System.out.println ("Coordinator: failed to migrate service instance " + instanceUUID + " to node " + nodeUUID + "; exception = " + ex);
	                        }
	                    }
	                }
	            }
            }
        }
    }

    private synchronized void migrateAllServicesToOtherNode()
        throws Exception
    {
        String[] otherCandidateNodes;
        long startTime = System.currentTimeMillis();
        while (true) {
            otherCandidateNodes = getNodesOrderedByCPUAvailability();
            if ((otherCandidateNodes != null) && (otherCandidateNodes.length > 0)) {
                break;
            }
            else if ((System.currentTimeMillis() - startTime) > MAX_WAIT_TIME_WHILE_SUSPENDING) {
                // Cannot find any nodes to migrate services to
                System.out.println ("Coordinator: no node is available to migrate services to :-(");
                throw new Exception ("migration failed - no other nodes available");
            }
            else {
                // Wait a little while and try again
                Thread.sleep (50);
            }
        }
        System.out.print ("Coordinator: Nodes ordered by CPU availability:");
        for (int i = 0; i < otherCandidateNodes.length; i++) {
            System.out.print (" " + otherCandidateNodes[i]);
        }
        System.out.println();
        for (int i = 0; i < otherCandidateNodes.length; i++) {
            String nodeProperties = _nsm.getNodeInfo (otherCandidateNodes[i]) . properties;
            if ((nodeProperties != null) && (nodeProperties.indexOf ("client") >= 0)) {
                System.out.println ("skipping node " + otherCandidateNodes[i] + " because it is a client node");
                continue;
            }
            System.out.println ("Coordinator: trying to migrate all services to " + otherCandidateNodes[i]);
            try {
                _kernelProxy.migrateAllServices (otherCandidateNodes[i]);
                System.out.println ("Coordinator: successfully migrated all services to " + otherCandidateNodes[i]);
                _activeServices.clear();
                return;
            }
            catch (Exception e) {
                System.out.println ("Coordinator: migrating services to " + otherCandidateNodes[i] + " failed - exception: " + e);
            }
        }
        System.out.println ("Coordinator: no node is available to migrate services to :-(");
        throw new Exception ("migration failed - no (other) nodes available");
    }

    public class ComparatorForNodesByCPU implements Comparator
    {
        ComparatorForNodesByCPU (NodeStateMonitor nsm)
        {
            _nsm = nsm;
        }

        public int compare (Object o1, Object o2)
        {
            NodeInfo firstNodeInfo = (NodeInfo) _nsm.getNodeInfo ((String)o1);
            NodeInfo secondNodeInfo = (NodeInfo) _nsm.getNodeInfo ((String)o2);
            return ((firstNodeInfo.cpuUtilizationACI + firstNodeInfo.cpuUtilizationOther) - 
                    (secondNodeInfo.cpuUtilizationACI + secondNodeInfo.cpuUtilizationOther));
        }

        private NodeStateMonitor _nsm;
    }

    public class ActiveService
    {
        public ActiveService (String serviceUUID, String nodeUUID, String serviceName, boolean onLocalNode)
        {
            this.onLocalNode = onLocalNode;
            this.serviceUUID = serviceUUID;
            this.nodeUUID = nodeUUID;
            this.serviceName = serviceName;
        }
        
        /**
         * @return "acil://[<_instanceUUID>]|[<_nodeUUID>]|[<ipaddr[:_port]>]"
         */
        public String getServiceURI() 
        {
        	return "acil://" + serviceUUID + "|" + nodeUUID + "|";
        }
        
        public String toString()
        {
        	return getServiceURI();
        }

        public boolean onLocalNode;
        public String serviceUUID;
        public String nodeUUID;
        public String serviceName;
    }

    public static final String ACI_GROUP_NAME_KEY = "aci.groupmanager.acigroup.name";
    public static final int MAX_WAIT_TIME_WHILE_SUSPENDING = 2000;
    public static final int EXPLOITABLE_NODE_CPU_UTIL_THRESHOLD = 10; //Others+ACI
    public static final int NODE_CPU_UTIL_THRESHOLD = 50; //Others
    
    ////////////////////////////////////////////////////////////////////////////
    private ServiceManager _serviceManager;
    private KernelProxy _kernelProxy;
    private ConfigManager _cfgManager;
    private String _localNodeUUID;
    private String _localNodeProperties;
    private NodeInfo _localNodeInfo;
    private NodeStateMonitor _nsm;
    private Vector _networkTopology;
    private Hashtable _activeServices = new Hashtable();   // Maps serviceUUIDs (String) to ActiveService objects /*!!*/ // Consider splitting this into two hashtables - one for local and one for remote services
}
