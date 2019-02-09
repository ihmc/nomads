package us.ihmc.aci.netSupervisor.topology;

import java.util.*;
import java.net.*;

import us.ihmc.aci.ddam.*;
import us.ihmc.aci.netSupervisor.information.InformationBroker;
import us.ihmc.aci.netSupervisor.traffic.WorldStateSummary;
import us.ihmc.aci.netSupervisor.utilities.Utilities;
import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import java.util.HashMap;
import java.util.Map;
import org.apache.log4j.Logger;
//import us.ihmc.aci.nodemon.util.Utils;

/**
 * Created by Roberto on 8/8/2016.
 */
public class TopologyManager {

    /**
     * Default constructor of Topology Manager
     */
    public TopologyManager ()
    {
        _subnetworks = new HashMap<>();
    }

    /**
     * Update the local topology view with the new information
     * @param  ib update container
     * @param  monitoredNodes
     * @return True if the process was successful false otherwise
     */
    public boolean updateTopologyInfo (InformationBroker ib, Collection<String> monitoredNodes)
    {
        log.debug("TopologyManager: Updating Topology Info");
        if(ib != null) {
            if(updateSubnetStructure(ib.getTopology())) {
                log.debug("TopologyManager - updateTopologyInfo: Subnet information updated");
            }
            else {
                log.warn("TopologyManager - updateTopologyInfo: Subnet information update failed");
            }

            if(updateConnectedCouples(ib.getConnectedCouplesInformation())) {
                log.debug("TopologyManager - updateConnectedCouples: Connected Couples information updated");
            }
            else {
                log.warn("TopologyManager - updateConnectedCouples: Connected Couples information update failed");
            }

            if(updateSubnetCardinality(ib.getsubnetCardinality())) {
                log.debug("TopologyManager - updateSubnetCardinality: Subnet Cardinality information updated");
            }
            else {
                log.warn("TopologyManager - updateSubnetCardinality: Subnet Cardinality information update failed");
            }

            if(updateTrafficBetweenSubnets(ib.getTrafficBetweenSubnets())) {
                log.debug("TopologyManager - updateTrafficBetweenSubnets: Traffic Between subnet information updated");
            }
            else {
                log.warn("TopologyManager - updateTrafficBetweenSubnets: Traffic Between subnet information update failed");
            }

            if(updateTrafficBetweenNP(ib.getTrafficBetweenNPs())) {
                log.debug("TopologyManager - updateTrafficBetweenNP: Traffic Between NetProxy information updated");
            }
            else {
                log.warn("TopologyManager - updateTrafficBetweenNP: Traffic Between NetProxy information update failed");
            }

            if(updateUnmappedTraffic(ib.getNonMappedTraffic())) {
                log.debug("TopologyManager - updateUnmappedTraffic: Traffic Between NetProxy information updated");
            }
            else {
                log.warn("TopologyManager - updateUnmappedTraffic: Traffic Between NetProxy information update failed");
            }

            return true;
        }
        else {
            log.warn("TopologyManager: InformationBroker object is null");
            return false;
        }
    }


    /**
     * Update the traffic between netproxy view with the new information
     * @param  unmappedTraffic MAP of traffic between netproxy indexed by observerIP-subnetIDA-subnetIDB
     * @return True if the process was successful false otherwise.
     */
    private boolean updateUnmappedTraffic(Map<String, Double> unmappedTraffic)
    {
        Map<String, Double> umt = null;
        if(unmappedTraffic != null) {
            for(String unmappedKey : unmappedTraffic.keySet()) {
                log.debug("TopologyManager - updateUnmappedTraffic: Replacing value for " + unmappedKey);
                umt.replace(unmappedKey, unmappedTraffic.get(unmappedKey));
            }
            return true;
        }
        else {
            log.warn("TopologyManager - updateUnmappedTraffic: unmappedTraffic is null?");
            return false;
        }
    }


    /**
     * Update the traffic between netproxy view with the new information
     * @param  trafficBetweenNetProxy MAP of traffic between netproxy indexed by netproxyIPA-netproxyIPB
     * @return True if the process was successful false otherwise.
     */
    private boolean updateTrafficBetweenNP(Map<String, Double> trafficBetweenNetProxy)
    {
        Map<String, Double> tbnp = null;
        if(trafficBetweenNetProxy != null) {
            for(String netproxyIPcouple : trafficBetweenNetProxy.keySet()) {
                log.debug("TopologyManager - updateTrafficBetweenNP: Replacing value for " + netproxyIPcouple);
                tbnp.replace(netproxyIPcouple,trafficBetweenNetProxy.get(netproxyIPcouple));
            }
            return true;
        }
        else {
            log.warn("TopologyManager - updateTrafficBetweenNP: trafficBetweenNetProxy is null?");
            return false;
        }
    }


    /**
     * Update the local cc view with the new information
     * @param  trafficBetweenSubnets MAP of traffic between subnets indexed by subnetIDA-subnetIDB
     * @return True if the process was successful false otherwise.
     */
    private boolean updateTrafficBetweenSubnets(Map<String, Double> trafficBetweenSubnets)
    {
        Map<String, Double> tbs = null;
        if(trafficBetweenSubnets != null) {
            for(String subnetIDCouple : trafficBetweenSubnets.keySet()) {
                log.debug("TopologyManager - updateTrafficBetweenSubnets: Replacing value for " + subnetIDCouple);
                tbs.replace(subnetIDCouple,trafficBetweenSubnets.get(subnetIDCouple));
            }
            return true;
        }
        else {
            log.warn("TopologyManager - updateTrafficBetweenSubnets: trafficBetweenSubnets is null?");
            return false;
        }
    }

    /**
     * Update the local cc view with the new information
     * @param  cardinalityMap MAP of subnet cardinalities indexed by subnetID
     * @return True if the process was successful false otherwise.
     */
    private boolean updateSubnetCardinality(Map<String, Integer> cardinalityMap)
    {
        //make this private
        Map<String, Integer> cc = null;
        if(cardinalityMap != null) {
            for(String networkID : cardinalityMap.keySet())
            {
                log.debug("TopologyManager - updateSubnetCardinality: Replacing value for " + networkID);
                cc.replace(networkID,cardinalityMap.get(networkID));
            }
            return true;
        }
        else {
            log.warn("TopologyManager - updateSubnetCardinality: connectedCouplesMap is null?");
            return false;
        }
    }


    /**
     * Update the local cc view with the new information
     * @param  connectedCouplesMap MAP of cc indexed by subnetID
     * @return True if the process was successful false otherwise.
     */
    private boolean updateConnectedCouples(Map<String, Integer> connectedCouplesMap)
    {
        Map<String, Integer> _ccMap = null; //INITIALIZE THIS PRIVATE VARIABLE!

        if(connectedCouplesMap != null)
        {
            for(String subnetID : connectedCouplesMap.keySet()) {
                _ccMap.replace(subnetID,connectedCouplesMap.get(subnetID));
            }
            return true;
        }
        else {
            log.warn("TopologyManager - updateConnectedCouples: connectedCouplesMap object is null");
            return false;
        }
    }

    /**
     * Update the local topology view with the new information
     * @param  topology updated topology
     * @return True if the process was successful
     */
    private boolean updateSubnetStructure(Map<String, Subnetwork> topology)
    {
        if(topology != null) {
            for(String subnetID : topology.keySet()) {
                Subnetwork subnet = topology.get(subnetID);
                //THIS MAY NEED TO BECOME MORE COMPLEX
                _subnetworks.replace(subnetID,subnet);
            }
            return true;
        }
        else {
            log.warn("TopologyManager - updateSubnetStructure: topology object is null");
            return false;
        }
    }




    public boolean analizeTopology(InformationBroker ib, Collection<String> monitoredNodes)
    {
        return true;
    }

    /**
     * This method analyzes the World State obtained from the local Node Monitor and retrieve Topology Information.
     * @param  localWorldState world state of the local Node Monitor.
     * @return True if no problem was detected, false otherwise.
     */
    /*
    public boolean analyzeWorldState (WorldState localWorldState, Collection<String> monitoredNodes)
    {
        log.debug("");
        log.debug("TopologyManager: Analyzing world state");
        Map<String, Node> nodesMap = localWorldState.getNodesMapCopy();

        _remoteNetProxy = new ArrayList<>();

        //for each node
        for(String nodeID : nodesMap.keySet()) {
            log.debug("Get Node: " + nodeID);
            log.debug("");
            Node node = nodesMap.get(nodeID);

            //get the topology
            Map<String, Topology> topology = node.getTopology();

            //for each subnet
            for(String topologyID : topology.keySet()) {
                Topology topologyElement = topology.get(topologyID);
                String networkName = topologyElement.getNetworkName();
                log.debug("Network node from topology element: " + networkName);

                //check if there is already a subnet entry in the subnet summary, if not add it
                Subnetwork subnetElement = getOrAddSubnetEntry(networkName,topologyElement.getSubnetMask());

                //internals
                //check if the internal nodes are already accounted for, if not add them
                computeInternals (subnetElement, topologyElement);
            }
        }
        associateMasterAndProxyToEachMonitoredIP(localWorldState, monitoredNodes);
        setRemoteNetproxyList(localWorldState);
        return true;
    }
    */

    /**
     * Getter method for the Remote NetProxy collection
     * @return True if no problem was detected, false otherwise.
     */
    public Collection<String> getRemoteNetProxyCollection()
    {
        return _remoteNetProxy;
    }

    /**
     * This is a getter method for the private variable _Subnetworks.
     * @return A map containing the subnetwork or null if the map is not present.
     */
    public Map<String, Subnetwork> getSubnetworks() {
        return _subnetworks;
    }

    //doesn't account for groups!
    /*
    public boolean setRemoteNetproxyList (WorldState worldState)
    {
        for(String subnetworkKey : getSubnetworks().keySet()) {
            Subnetwork subnetwork = getSubnetworks().get(subnetworkKey);
            String subnetworkMasterIp = subnetwork.getNodeMonitorMaster();

            String localMasterIP = Utils.getPrimaryIP(worldState.getLocalNode());
            if (mastersAreDifferent(subnetworkMasterIp, localMasterIP)) {
                String netproxyIp = findNetProxyNode(worldState, subnetworkMasterIp);
                if(addNetProxyToList(netproxyIp)) {
                    log.trace("New netproxyIP: " + netproxyIp);
                }
                else {
                    log.trace("NetProxy not added to the list.");
                }
            }
        }
        return true;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * Checks if the masters are different (check if they are null too)
     * @param subnetworkMasterIp first Ip (from a different subnetwork)
     * @param localMasterIp second Ip (the local one)
     * @return true if they are different
     */
    private boolean mastersAreDifferent(String subnetworkMasterIp, String localMasterIp)
    {
        if(localMasterIp != null) {
            if(subnetworkMasterIp != null) {
                if(!subnetworkMasterIp.equals(localMasterIp)){
                    return true;
                }
                else {
                    log.trace("Local and subnet master are the same.");
                }
            }
            else {
                log.trace("Remote master IP is null.");
            }
        }
        else {
            log.trace("Local master IP is null.");
        }

        return false;
    }

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * Checks if a netProxy is already present in the list and adds it
     * @param netProxyIp
     * @return true if it's added
     */
    private boolean addNetProxyToList(String netProxyIp)
    {
        if (!_remoteNetProxy.contains(netProxyIp)) {
            if (netProxyIp != null) {
                _remoteNetProxy.add(netProxyIp);
                return true;
            }
            else {
                log.debug("Remote NetProxy IP is null");
            }
        }
        else {
            log.debug("New netproxyIP: " + netProxyIp + " was already present");
        }
        return false;
    }

    /**
     * This method get the already present subnetowrk object or create and return a new one if that is not present.
     * @param  networkName Name of the subnet.
     * @param  netMask netmask of the subnet
     * @return The already presente subnet or a newly created one.
     */
    private Subnetwork getOrAddSubnetEntry(String networkName, String netMask)
    {
        if (_subnetworks.containsKey(networkName)) {
            //we have already the subnet Entry
            return _subnetworks.get(networkName);
        }
        else {
            //We need to create a new subnet Entry
            log.debug("Creating a new subnet entry: " + networkName + " " + netMask);
            Subnetwork newSubnet = new Subnetwork(networkName, netMask);
            _subnetworks.put(networkName,newSubnet);
            return _subnetworks.get(networkName);
        }
    }


    /**
     * This metod analyzes a topologyElement and a subNet element adding to the internal list internal nodes retrieved
     * from the new topology element.
     * @param  subnetElement subnet object we wish to add nodes to.
     * @param  topologyElement topology object under exam to find new internal nodes.
     * @return True if no error was detected, false otherwise.
     */
    /*
    private boolean computeInternals (Subnetwork subnetElement, Topology topologyElement)
    {
        Map<Integer, Host> nodeInternals = topologyElement.getInternals();
        for(Integer stringKey : nodeInternals.keySet()) {
            Host host = nodeInternals.get(stringKey);
            try {
                if(host.getIp() != 0) {
                    String ip = InetAddress.getByAddress(unpack(host.getIp())).getHostAddress();
                    List<String> internalList = subnetElement.getInternalNodes();
                    if(internalList.contains(ip)) {
                        //node already present don't add it
                        continue;
                    }
                    else {
                        log.debug("Added a new IP: " + ip);
                        subnetElement.addInternal(ip);
                    }
                }
            } catch (UnknownHostException e) {
                log.error(e.toString());
                e.printStackTrace();
            }
        }
        return true;
    }
    */

    /**
     *
     * @return the first master
     */
    public String getNodeMonitorMaster1(){
        return _nodeMonitorMasterNode1;
    }

    /**
     *
     * @return the second master
     */
    public String getNodeMonitorMaster2(){
        return _nodeMonitorMasterNode2;
    }

    /**
     *
     * @return the first netproxy
     */
    public String getNetProxy1(){
        return _netProxyNode1;
    }

    /**
     *
     * @return the second netproxy
     */
    public String getNetProxy2(){
        return _netProxyNode2;
    }

    /**
     * set the nodemonitor masters ip for two nodes
     * @param localWorldState
     * @param ipNode1
     * @param ipNode2
     * @return true if setted
     */
    /*
    private boolean setTwoNodeMaster(WorldState localWorldState, String ipNode1, String ipNode2){
        String ipMaster1 = findNodeMonitorMasterNode(localWorldState, ipNode1);
        String ipMaster2 = findNodeMonitorMasterNode(localWorldState, ipNode2);
        if(ipMaster1 != null){
            if(ipMaster2 != null){
                _nodeMonitorMasterNode1 = ipMaster1;
                _nodeMonitorMasterNode2 = ipMaster2;
                return true;
            }else{
                log.trace("Master node 2 not found.");
            }
        }else{
            log.trace("Master node 1 not found.");
        }
        return false;
    }
    */
    /**
     * Sets the master and the netProxy for every subnetwork
     * @param localWorldState local state of the network
     * @return the ip of the master
     */

    /*
    private String findNodeMonitorMasterNode(WorldState localWorldState, String ipNode){
        Subnetwork subnetElement = null;

        //check the subnetwork of the ipNode
        String subnetworkId = getNodeSubnetworkName(ipNode);

        if(subnetworkId != null) {
            Node localNode = localWorldState.getLocalNode();
            Grump grump = localNode.getGrump();
            Map<String, Group> groups = null;
            //I keep all the found masters
            if (grump != null) {
                //get all the groups
                groups = grump.getGroups();
                //find the master with the same group of the node
                if (groups != null) {
                    Group masterGroup = groups.get("masters");
                    return findSubnetworkMasterFromMasterGroup(masterGroup, subnetworkId, localWorldState);
                }
                else {
                    log.trace("The grump hasn't any group.");
                }
            }
        }else{
            log.warn("Subnetwork of the node " + ipNode + " not found.");
        }
        return null;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * Find the master for a specific subnetwork
     * @param masterGroup the grump master group
     * @param subnetworkId
     * @param localWorldState
     * @return the master ip
     */

    /*
    private String findSubnetworkMasterFromMasterGroup(Group masterGroup, String subnetworkId, WorldState localWorldState)
    {
        if (masterGroup != null) {
            Subnetwork subnet = _subnetworks.get(subnetworkId);
            Map<String, String> members = masterGroup.getMembers();
            for (String idMasters : members.keySet()) {
                log.debug("Master ID: " + idMasters);
                Node nmaster = localWorldState.getNode(idMasters);
                String ipMaster = Utils.getPrimaryIP(nmaster);
                log.debug("ipMaster of the master to find: " + ipMaster);
                if (ipMaster != null){
                    if (subnet.isInternalNode(ipMaster)) {
                        return ipMaster;
                    }
                    else {
                        log.trace("The master isn't in the subnetwork " + subnetworkId);
                    }
                } else{
                    log.warn("The master doesn't have an IP.");
                }
            }
        }else{
            log.warn("Masters group empty.");
        }
        return null;
    }
    */

    /**
     * Returns the id of the subnetwork that contains the node
     * @param ipNode the ip of the node to search
     * @return the id of the subnetwork (null if it doesn't exist)
     */
    private String getNodeSubnetworkName(String ipNode){

        //check the subnetwork of the ipNode

        for(String subId : _subnetworks.keySet()){
            Subnetwork subnetwork = _subnetworks.get(subId);
            if(subnetwork.isInternalNode(ipNode)){
                return subId;
            }
        }
        return null;
    }

    /**
     * Returns the id of the subnetwork that contains the node
     * @param ipNode the ip of the node to search
     * @return the subnetwork object(null if it doesn't exist)
     */
    public Subnetwork getNodeSubnetwork(String ipNode){

        //check the subnetwork of the ipNode

        for(String subId : _subnetworks.keySet()){
            Subnetwork subnetwork = _subnetworks.get(subId);
            if(subnetwork.isInternalNode(ipNode)){
                return subnetwork;
            }
        }
        return null;
    }


    /**
     * Finds the two netProxies of the two nodes
     * @param localWorlState
     * @return
     */
    /*
    private boolean setNetProxyTwoNodes(WorldState localWorlState){
        if(_nodeMonitorMasterNode1 != null && _nodeMonitorMasterNode2 != null){
            _netProxyNode1 = findNetProxyNode(localWorlState, _nodeMonitorMasterNode1);
            _netProxyNode2 = findNetProxyNode(localWorlState, _nodeMonitorMasterNode2);
            if(_netProxyNode1 != null && _netProxyNode2 != null)
                return true;
        }
        return false;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * Finds the netProxies and the masters of the nodes
     * @param localWorlState
     */
    /*
    private void associateMasterAndProxyToEachMonitoredIP(WorldState localWorlState, Collection<String> monitoredNodes){
        String netProxyIp, masterIp;

        for(String nodeIp : monitoredNodes) {
            //getting the subnetwork of the node
            String subnetID = getNodeSubnetworkName(nodeIp);
            if(subnetID != null) {
                Subnetwork subnetwork = _subnetworks.get(subnetID);
                masterIp = getMasterOfSubnetwork(subnetwork, nodeIp, localWorlState);
                if(masterIp != null) {
                    subnetwork.setNetProxy(masterIp);
                    log.trace("Master " + masterIp + " setted for the subnet " + subnetwork.getName());
                }
                netProxyIp = getNetProxyOfSubnetwork(subnetwork, nodeIp, localWorlState);
                if(netProxyIp != null) {
                    subnetwork.setNetProxy(netProxyIp);
                    log.trace("NetProxy " + netProxyIp + " setted for the subnet " + subnetwork.getName());
                }
            }
        }
    }
    */

    /**
     * get the master of a subnetwork if it's not already present in the subnetwork object
     * @param subnetwork
     * @param nodeIp
     * @param localWorlState
     * @return
     */
    /*
    private String getMasterOfSubnetwork(Subnetwork subnetwork, String nodeIp, WorldState localWorlState) {
        if (subnetwork != null) {
            if (subnetwork.getNodeMonitorMaster() == null) {
                String masterIp = findNodeMonitorMasterNode(localWorlState, nodeIp);
                if (masterIp != null) {
                    return masterIp;
                }
                else {
                    log.trace("The master isn't present.");
                }
            }
            else {
                log.trace("The master of " + subnetwork.getName() + " is already present.");
            }
        }
        else {
            log.trace("The subnetwork isn't present.");
        }
        return null;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * get the netProxy of a subnetwork if it's not already present in the subnetwork object
     * @param subnetwork
     * @param nodeIp
     * @param localWorlState
     * @return
     */
    /*
    private String getNetProxyOfSubnetwork(Subnetwork subnetwork, String nodeIp, WorldState localWorlState)
    {
        if(subnetwork != null) {
            if (subnetwork.getNodeMonitorMaster() != null && subnetwork.getNetProxy() == null) {
                String netProxyIp = findNetProxyNode(localWorlState, subnetwork.getNodeMonitorMaster());
                if (netProxyIp != null) {
                    return  netProxyIp;
                }
                else {
                    log.trace("The netProxy isn't present.");
                }
            }
            else {
                log.trace("The netProxy of " + subnetwork.getName() + " is already present.");
            }
        }
        else {
            log.trace("The subnetwork isn't present.");
        }

        return null;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * finds the NetProxy of the node
     * @param localWorldState
     * @param ipNodeMaster
     * @return
     */
    /*
    public String findNetProxyNode(WorldState localWorldState, String ipNodeMaster){

        //search the subnetwork of the node master with ipNodeMaster
        if (ipNodeMaster != null && localWorldState != null) {
            String subId = getNodeSubnetworkName(ipNodeMaster);
            Node master = localWorldState.getNodeByIP(ipNodeMaster);

            Topology topology = findSubnetworkTopologyFromMaster(master, subId);
            if(topology != null) {
                return findNetProxyNodeFromTopology(topology);
            }
            else {
                log.trace("No topology part for the subnetwork " + subId);
            }
        }
        else {
            log.trace("Problems using the localWorldState.");
        }
        return null;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * Find the topology object for a master
     * @param master the master node with the topology object
     * @param subnetworkName
     * @return the topology object
     */
    /*
    private Topology findSubnetworkTopologyFromMaster(Node master, String subnetworkName)
    {
        if(master != null && subnetworkName != null){
            //find all the topologies
            Map<String,Topology> topologyArray = master.getTopology();
            if(topologyArray != null) {
                Topology topology = topologyArray.get(subnetworkName);
                return topology;
            }
            else {
                log.trace("No topology object for the master " + master.getId());
            }
        }
        else {
            log.trace("Master or subnetwork name not present.");
        }
        return null;
    }
    */

    /**
     * Author Emanuele Tagliaferro on 10/31/2016.
     *
     * Find the netProxy in a topology object
     * @param topology
     * @return the Ip of netProxy
     */
    /*
    private String findNetProxyNodeFromTopology(Topology topology)
    {

        if(topology != null) {
            Map<Integer, Host> externals = topology.getExternals();
            if (externals != null) {
                //for each external find the netProxy
                for (Integer extId : externals.keySet()) {
                    Host h = externals.get(extId);
                    if (h.getIsDefault()) {
                        return Utils.convertIPToString(h.getIp());
                    }
                }
            }
            else {
                log.trace("No externals found.");
            }
        }
        else {
            log.trace("Topology object not present.");
        }

        return null;
    }
    */

    /**
     *
     */
    byte[] unpack(int bytes) {
        return new byte[] {
                (byte)((bytes >>> 24) & 0xff),
                (byte)((bytes >>> 16) & 0xff),
                (byte)((bytes >>>  8) & 0xff),
                (byte)((bytes       ) & 0xff)
        };
    }

    private final Map<String, Subnetwork> _subnetworks;

    private static final Logger log = Logger.getLogger(TopologyManager.class);
    private Collection<String> _remoteNetProxy;
    private String _localNetProxy;
    private Collection<String> _remoteNodeMonitorMasters;

    private String _nodeMonitorMasterNode1;
    private String _nodeMonitorMasterNode2;
    private String[] _relationCouples;
    private String _netProxyNode1;
    private String _netProxyNode2;
}
