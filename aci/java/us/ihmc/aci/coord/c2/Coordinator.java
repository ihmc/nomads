package us.ihmc.aci.coord.c2;

import java.io.File;

import java.net.InetAddress;

import java.util.Hashtable;
import java.util.Iterator;
import java.util.Vector;

import us.ihmc.aci.grpMgrOld.GroupManager;
import us.ihmc.aci.grpMgrOld.PeerInfo;

//import us.ihmc.ffaci.feedParams.FeedParams;

import us.ihmc.net.NetUtils;

import us.ihmc.util.ConfigLoader;
import us.ihmc.util.Dime;
import us.ihmc.util.DimeRecord;

/**
 * Coordinator.java
 *
 * Created on April 13, 2007, 7:03 PM
 *
 * @author  nsuri
 */
public class Coordinator
{
    public Coordinator()
    {
        try {
            // Initiailize the Config Loader and put in default properties
            initConfigLoader();

            // Initializing the GroupManager and the NodeStateMonitor 
            _httpHandler = new HTTPProtocolHandler (this, _cfgLoader.getPropertyAsInt ("aci.coordinator.port"));
            _gm = new GroupManager (_cfgLoader.getPropertyAsInt ("aci.groupmanager.port"));
            _gm.setNodeName (_cfgLoader.getProperty ("aci.coordinator.name"));
            _nsm = new NodeStateMonitor (_cfgLoader.getProperty ("aci.groupmanager.default.groupname"));
            _gm.setListener (_nsm);
            _gm.createPublicPeerGroup (_cfgLoader.getProperty ("aci.groupmanager.default.groupname"), null);
            //_gm.createPrivatePeerGroup (_cfgLoader.getProperty ("aci.groupmanager.default.groupname"), "nomads", null);
            _gm.start();
            _httpHandler.start();
        }
        catch (Exception e) {
            System.out.println ("Exception in Coordinator constructor: " + e);
        }
    }

    public class ActivationInfo
    {
        public String nodeUUID;
        public String instanceUUID;
    }

    public class ActiveFeedInfo
    {
        public String producerNodeUUID;
        public String producerName;
        public String consumerNodeUUID;
        public String consumerName;
//        public FeedParams feedParams;
    }

    /**
     * Selects a node, activates the service on the node, and returns the UUID of the
     * node and the UUID of the service instance.
     * 
     * @param serviceName         the classname of the service
     * @param clientNodeUUID      the UUID of the node requesting the service activation
     *
     * @return     An instance of ActivationInfo with the UUID of the node where the
     *             service has been activated and the UUID of the service instance or
     *             null in case of error
     */
    public ActivationInfo activateService (String serviceName, String clientNodeUUID)
        throws Exception
    {
        System.out.println ("Coordinator.activateService: Received activation for service: " + serviceName + 
                            " from clientNode: " + clientNodeUUID);
        
        String lowestCPUNodeUUID = getLowestCPUNode();

        System.out.println ("Coordinator.activateService - elected node for service " + serviceName +
                            " is: " + lowestCPUNodeUUID);

        String instanceUUID = HTTPRequester.tellNodeToActivateService (getIPAddrForNode (lowestCPUNodeUUID),
                                                                       _cfgLoader.getPropertyAsInt ("aci.kernel.port"),
                                                                       serviceName);

        ActivationInfo activationInfo = null;
        if (instanceUUID != null) {
            activationInfo = new ActivationInfo();
            activationInfo.nodeUUID = lowestCPUNodeUUID;
            activationInfo.instanceUUID = instanceUUID;
        }

        return activationInfo;
    }

//    public String requestToStartFeed (String producerNodeUUID, String producerName, String consumerNodeUUID, String consumerName, FeedParams feedParams)
//    {
//        return null;
//    }

    public void requestToStopFeed (String feedId)
    {
    }

    private void initConfigLoader()
        throws Exception
    {
        // Initialize the ConfigLoader
        String nomadsHome = (String) System.getProperties().get ("nomads.home");
        String cfgFilePath = File.separator + "aci" + File.separator + "conf" + File.separator + "aci.properties";
        _cfgLoader = ConfigLoader.initDefaultConfigLoader (nomadsHome, cfgFilePath);

        if (_cfgLoader.hasProperty ("aci.coordinator.name")) {
            _cfgLoader.setProperty ("aci.coordinator.name", COORDINATOR_NAME);
        }

        if (_cfgLoader.hasProperty ("aci.coordinator.port")) {
            _cfgLoader.setProperty ("aci.coordinator.port", DEFAULT_COORDINATOR_PORT_NUM + "");
        }

        if (_cfgLoader.hasProperty ("aci.groupmanager.default.groupname")) {
            _cfgLoader.setProperty ("aci.groupmanager.default.groupname", DEFAULT_GROUP_MANAGER_GROUP_NAME);
        }

        if (_cfgLoader.hasProperty ("aci.kernel.port")) {
            _cfgLoader.setProperty ("aci.kernel.port", DEFAULT_KERNEL_PORT_NUM + "");
        }

        if (_cfgLoader.hasProperty ("aci.groupmanager.port")) {
            _cfgLoader.setProperty ("aci.groupmanager.port", DEFAULT_GROUP_MANAGER_PORT_NUM + "");
        }
    }

    private InetAddress getIPAddrForNode (String nodeUUID)
    {
        PeerInfo pgmi = _gm.getPeerInfo (nodeUUID);
        Vector netIFs = pgmi.nicsInfo; //Vector<NICInfo>
        InetAddress nodeAddr = NetUtils.determineDestIPAddr (pgmi.nicsInfo, _gm.getActiveNICsInfo());
        return nodeAddr;
    }

    private String getLowestCPUNode()
    {
        String nodeUUID = null;
        int minCpuVal = 100;
        Iterator it = _nsm.getActiveNodes();
        while (it.hasNext()) {
            String uuid = (String) it.next();
            NodeInfo nInfo = (NodeInfo) _nsm.getNodeInfo (uuid);
//            if (nInfo.cpuUtilization < minCpuVal) {
//                minCpuVal = nInfo.cpuUtilization;
//                nodeUUID = uuid;
//            }
            if (nInfo.cpuUtilizationAci < minCpuVal) {
                minCpuVal = nInfo.cpuUtilizationAci;
                nodeUUID = uuid;
            }
        }

        return nodeUUID;
    }

//    private String tellNodeToStartFeed (String nodeUUID, String producerName, String consumerNodeUUID, String consumerName, FeedParams feedParams)
//        throws Exception
//    {
//        try {
//            String feedId = HTTPRequester.tellNodeToStartFeed (getIPAddrForNode (nodeUUID),
//                                                               _cfgLoader.getPropertyAsInt ("aci.kernel.port"),
//                                                               producerName, consumerNodeUUID, consumerName, feedParams);
//            ActiveFeedInfo afi = new ActiveFeedInfo();
//            afi.producerNodeUUID = nodeUUID;
//            afi.producerName = producerName;
//            afi.consumerNodeUUID = consumerNodeUUID;
//            afi.consumerName = consumerName;
//            afi.feedParams = feedParams;
//            _activeFeeds.put (feedId, afi);
//            return feedId;
//        }
//        catch (Exception e) {
//            e.printStackTrace();
//            throw e;
//        }
//    }

    private void tellNodeToStopFeed (String feedId)
        throws Exception
    {
        ActiveFeedInfo afi = (ActiveFeedInfo) _activeFeeds.get (feedId);
        if (afi != null) {
            try {
                HTTPRequester.tellNodeToStopFeed (getIPAddrForNode (afi.producerNodeUUID),
                                                  _cfgLoader.getPropertyAsInt ("aci.kernel.port"),
                                                  feedId);
            }
            catch (Exception e) {
                e.printStackTrace();
                throw e;
            }
        }
    }

    public static void main (String[] args)
    {
        if (System.getProperties().get ("nomads.home") == null) {
            System.out.println ("Error! You need to define nomads.home");
            System.exit (-1);
        }
        
        Coordinator c = new Coordinator();
    }

    public static final int DEFAULT_KERNEL_PORT_NUM = 2005; 
    public static final int DEFAULT_GROUP_MANAGER_PORT_NUM = 2007;
    public static final int DEFAULT_COORDINATOR_PORT_NUM = 2009;
    public static final String DEFAULT_GROUP_MANAGER_GROUP_NAME = "ihmc.aci";
    public static final String COORDINATOR_NAME = "ACICentralCoordinator";

    private ConfigLoader _cfgLoader;    
    private HTTPProtocolHandler _httpHandler;
    private GroupManager _gm;
    private NodeStateMonitor _nsm;

    private Hashtable _activeFeeds = new Hashtable();      // Maps FeedId (String) to ActiveFeedInfo objects
}
