package mil.army.cpi.ba4.discoveryLib;

import java.math.BigInteger;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Set;
import java.util.UUID;

import DiscoveryService.DiscoveryService;
import DiscoveryService.NetworkInfo;
import DiscoveryService.SelfIdentity;
import DiscoveryService.ServiceInfo;
import mil.army.cpi.ba4.utils.Config;
import mil.army.cpi.ba4.utils.Utils;
import us.ihmc.aci.grpMgr.GroupManager;
import us.ihmc.aci.grpMgr.GroupManagerListener;
import us.ihmc.aci.grpMgr.PeerInfo;

class DiscoveredSystem extends AbstractDiscoveredSystem{
    
}

class Service extends AbstractService {
    
}

class Network extends AbstractNetwork {
    
}

class Identity extends AbstractIdentity {
    
}


public class Discovery implements GroupManagerListener {

    /**
     * Class Variables
     */
    private GroupManager _grpMgr;
    boolean grumpRunning = false;
//        private Utils util;


    //Internal Management
    private String mUUID = UUID.randomUUID().toString();
    private String mNodeName;
    private int mPort;
    private int mURN;

    private AbstractIdentity mIdentity;
    private Hashtable<String, AbstractService> mServices = new Hashtable<>();
    private Hashtable<String, AbstractNetwork> mNetworks = new Hashtable<>();
    private ArrayList<String> mGroups = new ArrayList<>();

    private Hashtable<String, ArrayList<String>> groupMembers = new Hashtable<>();
    private Hashtable<String, DiscoveredSystem> mPeers = new Hashtable<>();
    private int mHopCount = 1;

    private String mMulticastGroup;
    private int mTimeoutFactor;
    private int mUnhealthyFactor;
    private boolean mRetransmit;
    private String mFilePath;
    private int mPingInterval;
    private int mInfoInterval;
    private Utils util;

    private IDiscoveryListener mListener;




    public void loadConfigFromFile(String filePath) {
        util = new Utils();
        Config config = util.loadConfig(filePath);
        loadConfig(config);

//            if (!filePath.equals(util.DEFAULT_CONFIG_FILEPATH)) {
//                util.writeConfig(util.DEFAULT_CONFIG_FILEPATH, config);
//            }
    }

    /**
     * Load the default values that the Group Manager should initialize with
     *
     * @param config
     */
    private void loadConfig(Config config) {
        DiscoveryService discoveryService = _grpMgr.getDiscoveryService();
        resetDiscoveryService(discoveryService);

        //Identity
        AbstractIdentity identity = config.getIdentity();
        discoveryService.addSelfIdentity(identity.systemType, identity.platformType, identity.secClass, identity.ADCONURN, identity.OPCONURN, identity.ORGANICURN, (byte) identity.agency, identity.nationality, identity.symbolCode);
        mIdentity = new Identity();
        mIdentity.systemType = identity.systemType;
        mIdentity.platformType = identity.platformType;
        mIdentity.secClass = identity.secClass;
        mIdentity.ADCONURN = identity.ADCONURN;
        mIdentity.OPCONURN = identity.OPCONURN;
        mIdentity.ORGANICURN = identity.ORGANICURN;
        mIdentity.agency = identity.agency;
        mIdentity.nationality = identity.nationality;
        mIdentity.symbolCode = identity.symbolCode;

        //Peer Data
        mNodeName = config.getNodeName();
        _grpMgr.setNodeName(mNodeName);
        mURN = config.getURN();
        discoveryService.setURN(mURN);

        //Services
        for (AbstractService service : config.getServices()) {
            String uuid = discoveryService.addService(service.serviceName, service.serviceType, service.port);
            service.UUID = uuid;
            mServices.put(uuid, service);
        }

        //Networks
        for (AbstractNetwork network : config.getNetworks()) {
            String uuid = discoveryService.addNetwork(false, false, network.priority, (byte) network.networkType, ipToLong(network.ip), 0);
            network.UUID = uuid;
            mNetworks.put(uuid, network);
        }


        List<String> newGroups = manageGroups(config.getGroups());
        //Groups
        for (String group : newGroups) {
            mGroups.add(group);
            _grpMgr.createPublicPeerGroup(group, null);
        }

        _grpMgr.updatePeerNodeData();
    }

    /**
     * Clear out all services and networks from the Discovery Service
     * and the GruMP packets
     *
     * @param discoveryService          The GruMP Discovery Service containing the
     *                                  service and network packets
     */
    private void resetDiscoveryService(DiscoveryService discoveryService) {
        if (!mServices.isEmpty()) {
            for (String serviceID : mServices.keySet()) {
                discoveryService.removeService(serviceID);
            }
            mServices.clear();
        }

        if (!mNetworks.isEmpty()) {
            for (String networkID : mNetworks.keySet()) {
                discoveryService.removeNetwork(networkID);
            }
            mNetworks.clear();
        }
    }

    private List<String> manageGroups(List<String> groups) {
        List<String> interList = new ArrayList<>(mGroups);
        List<String> addingList = new ArrayList<>(groups);
        interList.retainAll(groups);
        mGroups.removeAll(interList);
        addingList.removeAll(interList);

        if (!mGroups.isEmpty()) {
            for (String group : mGroups) {
                _grpMgr.removeGroup(group);
            }
        }

        mGroups.clear();
        mGroups.addAll(interList);
        return addingList;
    }

    /**
     * Initialize the Group Manager
     *
     * @return 0 if successful
     */
    public int initGrump() {

        int rv = -2;

        //Only start GruMP if it is not already running
        if (!grumpRunning) {

            //Initialize the Group Manager
            System.out.println("Initializing GruMP");
            _grpMgr = new GroupManager();
            _grpMgr.setLogger(getFilePath() + "/GrumpLogger.txt");
            _grpMgr.setGroupManagerListener(this);

            rv = -1;


            try {
                _grpMgr.setMulticastGroup(getMulticastGroup());
                _grpMgr.setNodeTimeout(getTimeoutFactor());
                _grpMgr.setNodeUnhealthy(getUnhealthyFactor());
                System.out.println("multicastGroup " + _grpMgr.getMulticastGroup());
                _grpMgr.getDiscoveryService().setRetransmit(getRetransmit());
                rv = _grpMgr.init(mUUID, getmPort());
            } catch (Exception e) {
                e.printStackTrace();
            }

            System.out.println("Initialized Group Manager");

        } else {
            System.out.println("No Active Network");
        }


        return rv;
    }

    /**
     * Start running the Group Manager thread to send and receive packets
     */
    public void startGrump() {
        if (!grumpRunning) {
            System.out.println("Starting GruMP");
            _grpMgr.setPingInterval(getPingInterval());
            _grpMgr.setInfoBCastInterval(getInfoInterval());

            _grpMgr.start();
            grumpRunning = true;
        }
    }

    public void stopGrump() {
        //Shut down GruMP
        if (_grpMgr != null) {
            _grpMgr.terminate();
            grumpRunning = false;
        }
    }

    public void setListener(IDiscoveryListener listener){
        mListener = listener;
    }


    /**
     * @brief Get the UUID for the local Discovery Service
     *
     * @details Each Discovery Service has a unique identifier
     * associated with it
     *
     * @return UUID             a String containing the UUID of the service
     */
    public String getUUID() {
        return _grpMgr.getNodeUUID();
    }

    /**
     * @brief Join a group with a specified group name
     *
     * @details Systems are only discovered if they are in
     * the same groups so joining a group allows you to see all
     * of the other systems that have also joined that group
     *
     * @param groupName             The name of the group to join
     */
    public void joinGroup(String groupName) {
        _grpMgr.createPublicPeerGroup(groupName, null);
        mGroups.add(groupName);
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }
    }

    /**
     * @brief Leave a group that you are currently in
     *
     * @details  Once you no longer wish to discover systems in a particular
     * group you can leave that group.  This will also cause other systems
     * in that group to stop discovering you as well
     *
     * @param groupName             The name of the group to leave
     */
    public void leaveGroup(String groupName){
        _grpMgr.removeGroup(groupName);
        mGroups.remove(groupName);
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }
    }

    /**
     * @brief Add the identity information for the Discovery Service
     *
     * @details Each Discovery Service has various pieces of self identifying information
     *          that can be added
     *
     * @param systemType            The integer value of the system type
     *                              specified by SystemType enum
     *
     * @param platformType          The integer value of the platform type
     *                              specified by PlatformType enum
     *
     * @param secClass              The integer value of the security classification
     *                              specified by SecClass enum
     *
     * @param ADCONURN              URN of the Unit administratively controlling this Unit.
     *
     * @param OPCONURN              URN of the Unit operationally controlling this unit.
     *
     * @param ORGANICURN            URN of the Unit that is the base organic unit controlling
     *                              this unit. The organic unit is the same as the
     *                              opconurn at STARTEX.
     *
     * @param agency                char value for Unit's service branch, e.g. Army, Marines,
     *                              Red Cross, etc.
     *
     * @param nationality           Unit's nationality. e.g. US
     *
     * @param symbolCode            Military Standard 2525B symbol ID code of Unit
     */
    public void addIdentity(int systemType, int platformType, int secClass, int ADCONURN, int OPCONURN, int ORGANICURN, char agency, byte nationality, String symbolCode) {

        mIdentity = new Identity();
        mIdentity.systemType = systemType;
        mIdentity.platformType = platformType;
        mIdentity.secClass = secClass;
        mIdentity.ADCONURN = ADCONURN;
        mIdentity.OPCONURN = OPCONURN;
        mIdentity.agency = agency;
        mIdentity.nationality = nationality;
        mIdentity.symbolCode = symbolCode;
        
        _grpMgr.getDiscoveryService().addSelfIdentity(systemType, platformType,
                secClass, ADCONURN, OPCONURN, ORGANICURN, (byte) agency, nationality, symbolCode);
        _grpMgr.updatePeerNodeData();
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }
    }

    /**
     * @brief Get a list of the UUIDs of all the systems that have been discovered
     *
     * @details An id is returned of each system that was discovered that shares the
     *          same groups as the local Discovery Service
     *
     *
     * @return DiscoveredSystems[]          A list of all the Discovered Systems
     */
    public String[] getAllDiscoveredSystems() {
        ArrayList<String> members = new ArrayList<>();


        Set<String> keys = groupMembers.keySet();
        for (String key : keys) {
            for (String member : groupMembers.get(key)) {
                if(!members.contains(member)){
                    members.add(member);
                    System.out.println("adding peer");
                }

            }
        }
        String[] peers = new String[members.size()];
        peers = members.toArray(peers);
        return peers;
    }

    /**
     * @brief Allows new service for this system to be added
     *
     * @details Called by a CE System Component when a new service becomes available.
     *          The Discovery Service will store the service information, and publish
     *          out the service availability to other discoverable systems
     *
     *
     * @param serviceName               The name of the service
     *
     * @param serviceType               The type of the service to add
     *                                  as available for this system
     *
     * @param port                      The network port that the service is available on
     *
     * @retval None
     */
    public String addService(String serviceName, int serviceType, int port) {
        Service info = new Service();
        info.serviceName = serviceName;
        info.serviceType = serviceType;
        info.port = port;

        String uuid = _grpMgr.getDiscoveryService().addService(serviceName, serviceType, port);

        info.UUID = uuid;

        mServices.put(uuid, info);
        _grpMgr.updatePeerNodeData();
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }

        return uuid;
    }


    /**
     * @brief Allows a service for this system to be removed
     *
     * @details Called by a CE System Component when a service becomes unavailable.
     *          The Discovery Service will no longer be included when publishing out the service
     *          availability to other discoverable systems
     *
     *
     * @param serviceID           The id of the service to be removed
     */
    public void removeService(String serviceID){
        mServices.remove(serviceID);

        _grpMgr.getDiscoveryService().removeService(serviceID);
        _grpMgr.updatePeerNodeData();
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }

    }


    /**
     * @brief Allows new network for this system to be added
     *
     * @details Called by a CE System Component when a new network becomes available.
     *          The Discovery Service will store the network information, and publish out
     *          the network information to other discoverable systems.
     *
     *
     * @param gatewayFlag       (For Prototype only)  This will not be exposed in final version.
     *                          Tells the Discovery Service whether the network being added is a
     *                          gateway network or one local to the system
     *
     * @param ipVersion         If the IP Version is 4 or 6
     *
     * @param priority          Specifies the priority of the network being added
     *
     * @param networkType       The type of radio/comms link associated with the network
     *
     * @param ip                The IP address of the network passed in as a String
     *
     * @param gatewayURN        (For Prototype only)  This will not be used in final version.
     *                          The URN of the network if it is a gateway network
     *
     * @retval None
     *
     */
    public String addNetwork(boolean gatewayFlag, boolean ipVersion, byte priority, byte networkType, String ip, int gatewayURN) {
        AbstractNetwork info = new Network();
        info.gatewayURN = gatewayURN;
        info.priority = priority;
        info.ip = ip;
        info.networkType = networkType;

        long ipLong = ipToLong(ip);
        String networkUUID = _grpMgr.getDiscoveryService().addNetwork(gatewayFlag, ipVersion, priority, networkType, ipLong, gatewayURN);
        _grpMgr.updatePeerNodeData();
        info.UUID = networkUUID;
        mNetworks.put(networkUUID, info);

        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }
        
        return networkUUID;
    }

    /**
     * @brief   Allows network for this system to be modified
     *
     * @details Called by a CE System Component when a network changes
     *          occur.  The Discovery Service will store the updated
     *          network information, and publish out the updated
     *          network information to other discoverable systems.
     *
     * @param   networkID       The ID associated with the network as
     *                          provided by the Discovery Service
     *                          component from the addNetwork() call.
     *
     * @param gatewayFlag       (For Prototype only)  This will not be exposed in final version.
     *                          Tells the Discovery Service whether the network being added is a
     *                          gateway network or one local to the system
     *
     * @param ipVersion         If the IP Version is 4 or 6
     *
     * @param priority          Specifies the priority of the network being added
     *
     * @param networkType       The type of radio/comms link associated with the network
     *
     * @param ip                The IP address of the network passed in as a String
     *
     * @param gatewayURN        (For Prototype only)  This will not be used in final version.
     *                          The URN of the network if it is a gateway network
     *
     * @retval  The new UUID of the network
     *
     */
    public String modifyNetwork(String networkID, boolean gatewayFlag, boolean ipVersion, byte priority, byte networkType, String ip, int gatewayURN) {
        _grpMgr.getDiscoveryService().removeNetwork(networkID);
        mNetworks.remove(networkID);
        String networkUUID =  addNetwork(gatewayFlag, ipVersion, priority, networkType, ip, gatewayURN);

        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }
        return networkUUID;
    }

    /**
     * @brief Removes network information from the system
     *
     * @details Called by a CE System Component when a network is terminated/removed.
     *          The Discovery Service will delete the network information, and publish out
     *          the network information to other discoverable systems with the removed
     *          network no longer present
     *
     *
     * @param networkID           The id of the network
     *
     * @retval None
     *
     */
    public void removeNetwork(String networkID) {
        _grpMgr.getDiscoveryService().removeNetwork(networkID);
        _grpMgr.updatePeerNodeData();

        mNetworks.remove(networkID);
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }

    }

    /**
     * @brief get the information of a system that has been discovered
     *
     * @details Once a Discovery Service has joined a group it will begin to
     *          discover other systems within that same group.  These systems will have
     *          information associated with it that can be accessed
     *
     * @param memberUUID            The id of the discovered system
     *
     * @return discoveredSystem     The system that was discovered
     */
    public DiscoveredSystem getDiscoveredSystem(String memberUUID) {

        DiscoveredSystem discoveredSystem = new DiscoveredSystem();

        //Return our own info if our UUID is called
        if (memberUUID.equals(mUUID)) {
            discoveredSystem.nodeUUID = mUUID;
            discoveredSystem.nodeName = mNodeName;
            discoveredSystem.port = getmPort();
            discoveredSystem.groups = new String[mGroups.size()];
            discoveredSystem.groups = mGroups.toArray(discoveredSystem.groups);
            discoveredSystem.URN = mURN;
            discoveredSystem.services = getMyServices();
            discoveredSystem.networks = getMyNetworks();
            discoveredSystem.identity = mIdentity;

            discoveredSystem.checkForServicesAndNetworks();
        }
        else {
            PeerInfo info = _grpMgr.getPeerInfo(memberUUID);

            discoveredSystem.nodeUUID = info.getNodeUUID();
            discoveredSystem.nodeName = info.getNodeName();
            discoveredSystem.port = info.getPort();
            discoveredSystem.groups = info.getGroups();


            discoveredSystem.URN = _grpMgr.getDiscoveryService().getPeerURN(memberUUID);
            discoveredSystem.services = getServiceInfo(memberUUID);
            discoveredSystem.networks = getNetworkInfo(memberUUID);
            discoveredSystem.identity = getIdentity(memberUUID);

            //See if any services or networks are present so we can send them through aidl properly
            discoveredSystem.checkForServicesAndNetworks();
        }

        return discoveredSystem;
    }

    /**
     * @brief Get all the group members for a specific group
     *
     * @details A Discovery Service can join more that one group. So the view can be limited
     *          to just view Discovered Systems that are a part of one group
     *
     * @param groupName         The name of the group to get the members of
     * @return
     */
    public String[] getGroupMembers(String groupName) {

        if (groupMembers.containsKey(groupName)) {
            String[] members = new String[groupMembers.get(groupName).size()];
            members = groupMembers.get(groupName).toArray(members);
            return members;
        } else {
            return new String[0];
        }
    }

    /**
     * @brief Get all of the groups that have been joined
     *
     *
     * @return groups[]             A list of all the groups that were joined
     */
    public String[] getGroups() {
        String[] groups = new String[mGroups.size()];
        groups = mGroups.toArray(groups);
        return groups;
    }

    /**
     * @brief Query for a service
     *
     * @details Start a search for a service that can be either a local service registered
     *          or a service that another discovered system has registered
     *
     * @param groups                    The list of groups to search within
     * @param serviceType               The type of service to search for
     * @param timeToLive                The length of time the search should be alive for
     *                                  if -1 this will be an indefinite peer search.
     *                                  if set to 0 the search will be processed only once.
     *
     * @return searchUUID               The UUID of the search request
     */
    public String requestService(String[] groups, int serviceType, int timeToLive) {

        byte[] searchService = {(byte) serviceType};

        String searchUUID = _grpMgr.startGroupsSearch(groups, searchService, timeToLive);


        return searchUUID;
    }

    public void cancelServiceRequest(String searchUUID) {
        _grpMgr.stopPersistentPeerSearch(searchUUID);
    }


    //Get the current hop count of the Discovery Service
    public int getHopCount() {
        return mHopCount;
    }






    /************************************Non aidl methods****************************************/

    /**
     * @brief Get all the services on the local Discovery Service
     *
     *
     * @return services[]             A list of all the services that were added
     */
    public AbstractService[] getMyServices() {
        ArrayList<AbstractService> servicesList = new ArrayList<>();

        for (String serviceName : mServices.keySet()) {
            servicesList.add(mServices.get(serviceName));
        }

        AbstractService[] services = new AbstractService[servicesList.size()];
        services = servicesList.toArray(services);
        return services;
    }

    /**
     * @brief Get all the netowrks on the local Discovery Service
     *
     *
     * @return networks[]           A list of all the networks that were added
     */
    public AbstractNetwork[] getMyNetworks() {
        ArrayList<AbstractNetwork> networkList = new ArrayList<>();

        for (String networkName : mNetworks.keySet()) {
            networkList.add(mNetworks.get(networkName));
        }

        AbstractNetwork[] networks = new AbstractNetwork[networkList.size()];
        networks = networkList.toArray(networks);
        return networks;
    }

    public AbstractService[] getServiceInfo(String memberUUID) {
        ServiceInfo[] info = _grpMgr.getDiscoveryService().getPeerServices(memberUUID);
        if (info != null) {
            AbstractService[] services = new AbstractService[info.length];
            for (int i = 0; i < info.length; i++) {
                services[i] = new Service();
                services[i].serviceName = info[i].getServiceName();
                services[i].serviceType = info[i].getServiceType();
                services[i].port = info[i].getPort();
            }
            return services;
        } else {
            return null;
        }

    }

    public AbstractNetwork[] getNetworkInfo(String memberUUID) {
        NetworkInfo[] info = _grpMgr.getDiscoveryService().getPeerNetworks(memberUUID);
        if (info != null) {
            AbstractNetwork[] network = new AbstractNetwork[info.length];
            for (int i = 0; i < info.length; i++) {
                network[i] = new Network();
                network[i].networkType = info[i].getNetworkType();
                network[i].ip = AbstractNetwork.intToIp((int) info[i].getIP());
                network[i].priority = info[i].getPriority();
                network[i].gatewayURN = info[i].getGatewayURN();
            }
            return network;
        } else {
            return null;
        }
    }

    //Get the identity of the Discovery System
    public AbstractIdentity getIdentity(String memberUUID) {
        SelfIdentity self = _grpMgr.getDiscoveryService().getPeerSelfIdentity(memberUUID);
        AbstractIdentity identity = new Identity();
        if (self != null) {
            identity.systemType = (byte)self.getSystemType();
            identity.platformType = self.getPlatformType();
            identity.secClass = (byte) self.getSecClass();
            identity.ADCONURN = self.getADCONURN();
            identity.OPCONURN = self.getOPCONURN();
            identity.ORGANICURN = self.getORGANICURN();
            identity.agency = (char) self.getAgency();
            identity.nationality = self.getNationality() & 0XFF;
            identity.symbolCode = self.getSymbolCode();
        }
        return identity;
    }

    //Convert a String of an ipv4 ip address to the corresponding long value
    public long ipToLong(String ipString) {

        long ipLong = 0;

        try {
            InetAddress address = InetAddress.getByName(ipString);
            ipLong = new BigInteger(address.getAddress()).intValue();
        } catch (Exception e) {
            e.printStackTrace();
        }

        return ipLong;

    }


    /************************************* Grump Listener Calls *********************************************/


    /**
     * New Peer is called when a New Peer event is trigger. It updates the remote
     * group list.
     *
     * @param nodeUUID the uuid of the new remote group
     */
    @Override
    public void newPeer(String nodeUUID) {
    }

    /**
     * Dead Peer is called when a peer event is dead. It updates the remote
     * group list.
     *
     * @param nodeUUID the uuid of the new remote group
     */
    @Override
    public void deadPeer(String nodeUUID) {

        mPeers.remove(nodeUUID);
        
        if(mListener != null){
            mListener.memberDataChanged(mUUID);
        }

    }


    /**
     * New Group Member is generated when a node joins a remote group. It
     * updates the local and remote group list.
     *
     * @param groupName  the group Name of the remote group
     * @param memberUUID the uuid of the local or remote group
     * @param bytes      the group data
     */
    @Override
    public void newGroupMember(String groupName, String memberUUID, byte[] bytes) {

        //Add the group if we are not already a member
        if (!groupMembers.containsKey(groupName)) {
            groupMembers.put(groupName, new ArrayList<String>());
        }
        groupMembers.get(groupName).add(memberUUID);

        if(mListener != null){
            mListener.newGroupMember(groupName, memberUUID);
        }
    }

    /**
     * Group Member Left is generated when a group leave a remote group. It
     * updates the local and remote group list.
     *
     * @param groupName  the group Name of the remote group
     * @param memberUUID the uuid of the local or remote group
     */
    @Override
    public void groupMemberLeft(String groupName, String memberUUID) {

        groupMembers.get(groupName).remove(memberUUID);
        
        if(mListener != null){
            mListener.groupMemberLeft(groupName, memberUUID);
        }
    }


    /**
     * Peer Group Data Changed is called when a node makes changes to
     * the groups data.
     *
     * @param groupName the group Name of the remote group
     * @param nodeUUID  the uuid of the remote node
     * @param param     the updated group data
     */
    @Override
    public void peerGroupDataChanged(String groupName, String nodeUUID, byte[] param) {

    }

    /**
     * Peer Node Data Changed is called when a node makes changes to
     * its own data.
     *
     * @param nodeUUID the uuid of the remote node
     */
    @Override
    public void peerNodeDataChanged(String nodeUUID) {
        System.out.println("Peer Node Data Changed " + nodeUUID);
        boolean foundMatch = false;
        for (String key : groupMembers.keySet()) {
            if (groupMembers.get(key).contains(nodeUUID)) {
                foundMatch = true;
                break;
            }
        }
        if (foundMatch) {
            try {
                mPeers.put(nodeUUID, getDiscoveredSystem(nodeUUID));
                
                if(mListener != null){
                    mListener.memberDataChanged(nodeUUID);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

    }

    /**
     * The hop count has been updated
     *
     * @param i         The new hop count value
     */
    @Override
    public void hopCountChanged(int i) {
        mHopCount = i;
        
        if(mListener != null){
            mListener.hopCountChanged(mHopCount);
        }
    }


    /**
     * A packet hasn't been received from this node in a while and will soon
     * become a dead peer
     *
     * @param nodeUUID              The id of the peer that has become unhealthy
     */
    public void peerUnhealthy(String nodeUUID) {
        System.out.println(nodeUUID + " Unhealthy");
        
        if(mListener != null){
            mListener.peerUnhealthy(nodeUUID);
        }

    }

    /**
     * Group List Change is called when change event occur in the GroupManager. It
     * updates the local and remote group list.
     *
     * @param nodeUUID the uuid of the local or remote group
     */
    @Override
    public void groupListChange(String nodeUUID) {
    }


    /**
     * Conflict With Private PeerGroup
     *
     * @param nodeUUID the uuid of the remote peer group
     */
    @Override
    public void conflictWithPrivatePeerGroup(String groupName, String nodeUUID) {

    }

    // Invoked by the GroupManager when a search request is recieved from another node
    // Application may choose to respond by invoking the searchReply() method in the GroupManager
    @Override
    public void peerSearchRequestReceived(String groupName, String nodeUUID, String searchUUID, byte[] param) {

        String searchService = Service.ServiceType.values()[param[0]].name();
        System.out.println("request received " + searchService);

        for (String serviceKey : mServices.keySet()) {
            AbstractService service = mServices.get(serviceKey);
            if (Service.ServiceType.values()[service.serviceType].name().equals(searchService)) {

                byte[] serviceName = service.serviceName.getBytes();
                byte[] serviceInfo = new byte[4];
                serviceInfo[0] = (byte)((service.serviceType >> 8) & 0xFF);
                serviceInfo[1] = (byte) (service.serviceType & 0xFF);

                serviceInfo[2] = (byte) ((service.port >> 8) & 0xFF);
                serviceInfo[3] = (byte) (service.port & 0xFF);

                byte[] response = new byte[serviceName.length + serviceInfo.length];
                System.arraycopy(serviceInfo, 0, response, 0, serviceInfo.length);
                System.arraycopy(serviceName, 0, response, serviceInfo.length, serviceName.length);
                _grpMgr.respondToPeerSearch(searchUUID, response);
            }
        }

    }

    // Invoked by the GroupManager when a response to a search request is received
    // This may be invoked multiple times, once per response received
    @Override
    public void peerSearchResultReceived(String groupName, String nodeUUID, String searchUUID, byte[] param) {

        AbstractService service = new Service();

        service.serviceType = param[0] << 8 & 0xFF00;
        service.serviceType |= param[1] & 0xFF;

        service.port = param[2] << 8 & 0xFF00;
        service.port |= param[3] & 0xFF;

        if (param.length > 4) {
            System.out.println("size " + (param.length - 4));
            byte[] serviceName = new byte[param.length - 4];
            System.arraycopy(param, 4, serviceName, 0, param.length - 4);

            service.serviceName = new String(serviceName);
        }
        else {
            service.serviceName = "";
        }


        System.out.println("results received! " + service.serviceName + " " + service.port + " " + Service.ServiceType.values()[service.serviceType].name()
                + " " + searchUUID);
        
        //TODO change port to service
        if(mListener != null){
            mListener.peerSearchResponse(nodeUUID, searchUUID, mPort);
        }


    }

    // Message received from another peer
    // groupName is null if the message is a direct message to the node (unicast)
    @Override
    public void peerMessageReceived(String groupName, String nodeUUID, byte[] data) {

        //Create a popup that displays the message
        final String toastText = nodeUUID + ": " + new String(data);

    }

    @Override
    public void persistentPeerSearchTerminated(String groupName, String nodeUUID, String peerSearchUUID) {

    }

    /**
     * @return the mMulticastGroup
     */
    public String getMulticastGroup() {
        return mMulticastGroup;
    }

    /**
     * @param multicastGroup the multicastGroup to set
     */
    public void setMulticastGroup(String multicastGroup) {
        this.mMulticastGroup = multicastGroup;
    }

    /**
     * @return the timeoutFactor
     */
    public int getTimeoutFactor() {
        return mTimeoutFactor;
    }

    /**
     * @param timeoutFactor the timeoutFactor to set
     */
    public void setTimeoutFactor(int timeoutFactor) {
        this.mTimeoutFactor = timeoutFactor;
    }

    /**
     * @return the unhealthyFactor
     */
    public int getUnhealthyFactor() {
        return mUnhealthyFactor;
    }

    /**
     * @param unhealthyFactor the unhealthyFactor to set
     */
    public void setunhealthyFactor(int unhealthyFactor) {
        this.mUnhealthyFactor = unhealthyFactor;
    }

    /**
     * @return the retransmit
     */
    public boolean getRetransmit() {
        return mRetransmit;
    }

    /**
     * @param retransmit the retransmit to set
     */
    public void setRetransmit(boolean retransmit) {
        this.mRetransmit = retransmit;
    }

    /**
     * @return the filePath
     */
    public String getFilePath() {
        return mFilePath;
    }

    /**
     * @param filePath the filePath to set
     */
    public void setFilePath(String filePath) {
        this.mFilePath = filePath;
    }

    /**
     * @return the pingInterval
     */
    public int getPingInterval() {
        return mPingInterval;
    }

    /**
     * @param pingInterval the pingInterval to set
     */
    public void setPingInterval(int pingInterval) {
        this.mPingInterval = pingInterval;
    }

    /**
     * @return the infoInterval
     */
    public int getInfoInterval() {
        return mInfoInterval;
    }

    /**
     * @param infoInterval the infoInterval to set
     */
    public void setInfoInterval(int infoInterval) {
        this.mInfoInterval = infoInterval;
    }

    /**
     * @return the mPort
     */
    public int getmPort() {
        return mPort;
    }

    /**
     * @param mPort the mPort to set
     */
    public void setmPort(int mPort) {
        this.mPort = mPort;
    }
    
    static {
        System.loadLibrary("grpmgrjavawrapper");
    }

}
