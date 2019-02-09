package us.ihmc.aci.nodemon;


import com.google.protobuf.Duration;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Timestamp;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.ddam.Group;
import us.ihmc.aci.nodemon.data.process.ProcessStats;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.aci.nodemon.util.Utils;

import java.util.*;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * BaseWorldState.java
 * <p/>
 * Class <code>BaseWorldState</code> represents the complete status of all the nodes of the network at any given time.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class BaseWorldState implements WorldState
{
    private class WorldStateController implements Runnable, Controller
    {
        WorldStateController ()
        {
            _isRunning = new AtomicBoolean(false);
            _updates = new LinkedBlockingDeque<>();
        }

        public synchronized void addUpdate (Container update)
        {
            _updates.add(update);
        }

        @Override
        public void run ()
        {
            _isRunning.set(true);
            while (_isRunning.get()) {
                try {
                    Container update;

                    TimeUnit.MILLISECONDS.sleep(50);
                    update = _updates.poll(Long.MAX_VALUE, TimeUnit.DAYS);


                    if (update == null) {
                        log.warn("Polled null update for NODE, skipping");
                        continue;
                    }

                    DataType type = update.getDataType();
                    String nodeId = update.getDataNodeId(); //TODO verify this works correctly

                    //objects
                    Node oldNode = getNode(nodeId);
                    if (oldNode == null) {
                        oldNode = createNode(nodeId);
                    }

                    Node node = null;
                    Info info;
                    Traffic traffic;
                    Topology topology;
                    NetworkHealth networkHealth;

                    switch (type) {
                        case NODE:
                            node = update.getNode();
                            break;
                        case INFO:
                            info = update.getInfo();
                            node = ProtoUtils.merge(oldNode, info);
                            break;
                        case GROUP:
                            List<Group> groups = update.getGroupsList();
                            for (Group g : groups) {
                                node = ProtoUtils.merge(oldNode, g);
                                oldNode = node; //update node
                            }
                            break;
                        case LINK:
                            List<Link> links = update.getLinksList();
                            for (Link l : links) {
                                node = ProtoUtils.merge(oldNode, l);
                                oldNode = node; //update node
                            }
                            break;
                        case TRAFFIC:
                            //(from NetSensor)
                            traffic = update.getTraffic();
                            node = ProtoUtils.merge(oldNode, traffic);
                            break;
                        case TOPOLOGY:
                            topology = update.getTopology();
                            node = ProtoUtils.merge(oldNode, topology);
                            break;
                        case NETWORK_HEALTH:
                            networkHealth = update.getNetworkHealth();
                            node = ProtoUtils.merge(oldNode, networkHealth);
                            break;
                        default:
                            throw new IllegalArgumentException("Type not supported for WorldState update: " + type);
                    }

                    if (node != null) put(node.getId(), node);

                }
                catch (Exception e) {
                    log.error("Error while updating the WorldState", e);
                }
            }
        }

        private Node createNode (String nodeId)
        {
            Node node;
            node = getNode(nodeId);
            if (node == null) {
                log.warn(" ^^^ Node Creation ^^^: creating " + nodeId + " - wasn't present in the WorldState");
                node = Node.newBuilder()
                        .setId(nodeId)
                        .setInfo(Info.newBuilder().build())
                        .setGrump(Grump.newBuilder().build())
                        .setTraffic(Traffic.newBuilder().build())
                        .setTimestamp(TimeUtil.getEpoch())
                        .build();
                put(node.getId(), node);
            }
            return node;
        }

        private boolean isTimeValid (Timestamp oldTime, Timestamp time)
        {
            Duration duration = getDuration(oldTime, time);
            if (TimeUtil.toNanos(duration) <= 0) {
                log.debug("Not updating, old Timestamp: " + time);
                return false;
            }

            return true;
        }

        private Duration getDuration (Timestamp oldTime, Timestamp newTime)
        {
            Duration duration;
            log.trace("Comparing Time."
                    + " Old: " + TimeUtil.toString(oldTime)
                    + " New: " + TimeUtil.toString(newTime));
            duration = TimeUtil.distance(oldTime, newTime);
            return duration;
        }

        @Override
        public void start ()
        {
            (new Thread(this, "WorldStateController")).start();
        }

        @Override
        public boolean isRunning ()
        {
            return _isRunning.get();
        }

        @Override
        public void stop ()
        {
            _isRunning.set(false);
        }


        private final BlockingDeque<Container> _updates;
        private final AtomicBoolean _isRunning;
        private final Logger log = Logger.getLogger(WorldStateController.class);
    }

    private static class Holder
    {
        static final BaseWorldState INSTANCE = new BaseWorldState();
    }

    public static WorldState getInstance ()
    {
        return Holder.INSTANCE;
    }

    BaseWorldState ()
    {
        _nodeMap = new ConcurrentHashMap<>();
        _nodeMapByIP = new ConcurrentHashMap<>();
        _processStats = new ProcessStats();
//        _trafficStats = new TrafficStats();
        _worldStateController = new WorldStateController();
    }


    @Override
    public synchronized void init (NodeMon nodeMon, String localNodeId, String localNodeName, String localIPAddress)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _localNodeId = Objects.requireNonNull(localNodeId, "LocalNodeId can't be null");
        _localIPAddress = localIPAddress;
        //create local node for the first time
        Node localNode = Node.newBuilder()
                .setId(Objects.requireNonNull(localNodeId, "Local node id can't be null"))
                .setName(Objects.requireNonNull(localNodeName, "Local node name can't be null"))
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();
        put(localNodeId, localNode);
        _worldStateController.start();
    }

    @Override
    public String getLocalNodeId ()
    {
        return _localNodeId;
    }

    @Override
    public Node getLocalNode ()
    {
        return _nodeMap.get(_localNodeId);
    }

    @Override
    public Node getLocalNodeCopy ()
    {
        return Node.newBuilder(_nodeMap.get(_localNodeId)).build();
    }

    @Override
    public void setLocalNode (Node node)
    {
        Objects.requireNonNull(node, "the local node can't be null");
        //_nodeMap.put(_localNodeId, node);
        Container c = Container.newBuilder()
                .setNode(node)
                .setDataType(DataType.NODE)
                .setDataNodeId(node.getId())
                .build();
        updateData(node.getId(), c);
    }

    @Override
    public void updateData (String nodeId, Container c)
    {
        _worldStateController.addUpdate(c);
    }

    @Override
    public void updateClients (String nodeId, Container c)
    {
        if (_nodeMon.getProxyScheduler() == null) {
            log.error("NodeMon proxy scheduler is null, unable to deliver message to proxy clients");
            return;
        }

        _nodeMon.getProxyScheduler().addOutgoingMessage(c);
    }

    private void put (String nodeId, Node node)
    {

        _nodeMap.put(nodeId, node);
        String primaryIP = Utils.getPrimaryIP(node);

        if (primaryIP == null) {
            //log.trace("Primary IP not found, trying to get it from id..");
            if (Utils.containsIP(nodeId)) {
                primaryIP = nodeId;
            }
        }

        if (primaryIP != null) {
            _nodeMapByIP.put(primaryIP, node);
            for (String ip : _nodeMapByIP.keySet()) {
                log.trace("[ " + ip + ": " + _nodeMapByIP.get(ip).getId() + " ]");
            }
        }
    }

    @Override
    public Node getNode (String nodeId)
    {
        return _nodeMap.get(nodeId);
    }

    @Override
    public Node getNodeByIP (String ipAddress)
    {
        return _nodeMapByIP.get(ipAddress);
    }

    @Override
    public int getOutgoingMcastTraffic (String observerIpAddress, String sourceIpAddress, String destMcastAddress,
                                        Duration duration)
    {
        Objects.requireNonNull(observerIpAddress, "the observerIpAddress can't be null");
        Objects.requireNonNull(sourceIpAddress, "the sourceIPAddress can't be null");
        Objects.requireNonNull(destMcastAddress, "the destMcastAddress can't be null");


        int sumMulticast = 0;
        if (!destMcastAddress.equals("*")) { //sum all multicast
            return getOutgoingTraffic(observerIpAddress, sourceIpAddress, destMcastAddress, duration);
        }
        else {
            for (String destMulticast : Utils.getIPList(DefaultValues.NodeMon.WORLDSTATE_MULTICAST_ADDRESSES)) {
                log.trace("Looking for outgoing multicast traffic: " + observerIpAddress + " -> " + destMulticast);
                sumMulticast += getOutgoingTraffic(observerIpAddress, sourceIpAddress, destMulticast, duration);
            }
        }

        return sumMulticast;
    }

    @Override
    public int getIncomingMcastTraffic (String observerIpAddress, String destMcastAddress, String
            sourceIPAddress, Duration duration)
    {
        Objects.requireNonNull(observerIpAddress, "the observerIpAddress can't be null");
        Objects.requireNonNull(destMcastAddress, "the destMcastAddress can't be null");
        Objects.requireNonNull(sourceIPAddress, "the sourceIPAddress can't be null");

        int sumMulticast = 0;
        try {
            if (!sourceIPAddress.equals("*")) { //sum all multicast
                return getIncomingTraffic(observerIpAddress, destMcastAddress, sourceIPAddress, duration);
            }
            else {
                //if called with getIncomingMcastTraffic(observerIpAddress, "0.0.0.0", "*");
                //go through all the sources and get only the destination multicast
                for (String destMulticast : Utils.getIPList(DefaultValues.NodeMon.WORLDSTATE_MULTICAST_ADDRESSES)) {
                    log.trace("Looking for incoming multicast traffic: " + destMulticast + " <- " + sourceIPAddress);
                    sumMulticast += getIncomingTraffic(observerIpAddress, destMulticast, sourceIPAddress, duration);
                }
            }
        }
        catch (Exception e) {
            log.error("Error while getting Incoming MCast traffic", e);
            return 0;
        }


        return sumMulticast;
    }

    @Override
    public int getObservedMcastTraffic (String observerIpAddress, String sourceIpAddress, String destMcastAddress,
                                        Duration duration)
    {
        Objects.requireNonNull(observerIpAddress, "the observerIpAddress can't be null");
        Objects.requireNonNull(sourceIpAddress, "the sourceIPAddress can't be null");
        Objects.requireNonNull(destMcastAddress, "the destMcastAddress can't be null");


        int sumMulticast = 0;
        if (!destMcastAddress.equals("*")) { //sum all multicast
            return getObservedTraffic(observerIpAddress, sourceIpAddress, destMcastAddress, duration);
        }
        else {
            for (String destMulticast : Utils.getIPList(DefaultValues.NodeMon.WORLDSTATE_MULTICAST_ADDRESSES)) {
                log.trace("Looking for observed multicast traffic: " + sourceIpAddress + " -> " + destMcastAddress);
                sumMulticast += getObservedTraffic(observerIpAddress, sourceIpAddress, destMulticast, duration);
            }
        }

        return sumMulticast;
    }

    @Override
    public int getObservedTraffic (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration
            duration)
    {
        Objects.requireNonNull(observerIpAddress, "the observerIpAddress can't be null");
        Objects.requireNonNull(sourceIpAddress, "the sourceIPAddress can't be null");
        Objects.requireNonNull(destIpAddress, "the destIpAddress can't be null");

        if (sourceIpAddress.equals("*")) {
            throw new IllegalArgumentException("Unable to select * for source ip address, only destination");
        }

        //sum all dest with source 0.0.0.0
        int sum = 0;
        if (destIpAddress.equals("*")) {
            //sum all dest with source 0.0.0.0
            Source src = getSource(observerIpAddress, sourceIpAddress);
            if (src == null) {
                return 0;
            }
            for (Integer ipInt : src.getDestinations().keySet()) {
                String currentDestIp = Utils.convertIPToString(ipInt);
                if (currentDestIp == null) {
                    continue;
                }

                //exclude multicast
                List<String> multicastAddress = Utils.getIPList(DefaultValues.NodeMon.WORLDSTATE_MULTICAST_ADDRESSES);
                if (!multicastAddress.contains(currentDestIp)) {
                    sum += getTraffic(TrafficType.OBSERVED, observerIpAddress, sourceIpAddress, currentDestIp,
                            duration);
                }

            }

            return sum;
        }
        else {
            return getTraffic(TrafficType.OBSERVED, observerIpAddress, sourceIpAddress, destIpAddress, duration);
        }
    }

    @Override
    public int getIncomingTraffic (String observerIpAddress, String destIpAddress, String sourceIpAddress, Duration
            duration)
    {
        Objects.requireNonNull(observerIpAddress, "the observerIpAddress can't be null");
        Objects.requireNonNull(sourceIpAddress, "the sourceIPAddress can't be null");
        Objects.requireNonNull(destIpAddress, "the destIpAddress can't be null");

        if (destIpAddress.equals("*")) {
            throw new IllegalArgumentException("Unable to select * for dest ip address, only source");
        }

        if (sourceIpAddress.equals("*")) {
            Node nodeObs = getNodeObserver(observerIpAddress);
            if (nodeObs == null) {
                return 0;
            }

            Traffic nodeObsTraffic = nodeObs.getTraffic();
            if (nodeObsTraffic == null) {
                log.trace("Unable to get traffic, node obs (" + nodeObs.getId() + ") traffic is empty");
                return 0;
            }

            int sum = 0;
            for (Integer srcIp : nodeObsTraffic.getSources().keySet()) {

                Source source = nodeObsTraffic.getSources().get(srcIp);
                String currentSourceIp = Utils.convertIPToString(srcIp);
                if (currentSourceIp == null) {
                    continue;
                }

                for (Integer destIp : source.getDestinations().keySet()) {
                    String currentDestIp = Utils.convertIPToString(destIp);
                    if (currentDestIp == null) {
                        continue;
                    }

                    if (currentDestIp.equals(destIpAddress)) {
                        sum += getTraffic(TrafficType.RECEIVED, observerIpAddress,
                                currentSourceIp,
                                currentDestIp,
                                duration);
                    }
                }
            }
            //exclude incoming multicast
            //return sum - getIncomingMcastTraffic(observerIpAddress, destIpAddress, sourceIpAddress, duration);
            return sum;
        }
        else {
            return getTraffic(TrafficType.RECEIVED, observerIpAddress, sourceIpAddress, destIpAddress, duration);
        }
    }

    @Override
    public int getOutgoingTraffic (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration
            duration)
    {
        Objects.requireNonNull(observerIpAddress, "the observerIpAddress can't be null");
        Objects.requireNonNull(sourceIpAddress, "the sourceIPAddress can't be null");
        Objects.requireNonNull(destIpAddress, "the destIpAddress can't be null");

        if (sourceIpAddress.equals("*")) {
            throw new IllegalArgumentException("Unable to select * for source ip address, only destination");
        }

        //sum all dest with source 0.0.0.0
        int sum = 0;
        if (destIpAddress.equals("*")) {
            //sum all dest with source 0.0.0.0
            Source src = getSource(observerIpAddress, sourceIpAddress);
            if (src == null) {
                return 0;
            }
            for (Integer ipInt : src.getDestinations().keySet()) {
                String currentDestIp = Utils.convertIPToString(ipInt);
                if (currentDestIp == null) {
                    continue;
                }

                //exclude multicast
                List<String> multicastAddress = Utils.getIPList(DefaultValues.NodeMon.WORLDSTATE_MULTICAST_ADDRESSES);
                if (!multicastAddress.contains(currentDestIp)) {
                    sum += getTraffic(TrafficType.SENT, observerIpAddress, sourceIpAddress, currentDestIp, duration);
                }

            }

            return sum;
        }
        else {
            return getTraffic(TrafficType.SENT, observerIpAddress, sourceIpAddress, destIpAddress, duration);
        }
    }

    private Node getNodeObserver (String observerIpAddress)
    {
        Node nodeObs = getNodeByIP(observerIpAddress);
        //TODO changed, verify if this is expensive
        //Node nodeObs = getNodesMapByIPCopy().get(observerIpAddress);

        if (nodeObs == null) {
            log.trace("Unable to get link, node obs [" + observerIpAddress + "] isn't present");
        }
        return nodeObs;
    }

    private Source getSource (String observerIpAddress, String sourceIpAddress)
    {
        Node nodeObs = getNodeObserver(observerIpAddress);
        if (nodeObs == null) {
            return null;
        }

        //TODO verify how expensive is this
        //TODO profiling of this code, too many calls from from NetSupervisor

        // Need to look for linkBA inside A
        Traffic nodeObsTraffic = nodeObs.getTraffic();
        if (nodeObsTraffic == null) {
            log.trace("Unable to get traffic, node obs (" + nodeObs.getId() + ") traffic is empty");
            return null;
        }

        Source source = nodeObsTraffic.getSources().get(Utils.convertIPToInteger(sourceIpAddress));
        if (source == null) {
            log.trace("Unable to get source, [" + sourceIpAddress + "] is not between Traffic sources of node obs " +
                    nodeObs.getId());
            return null;
        }

        return source;
    }


    private int getTraffic (TrafficType trafficType, String observerIpAddress, String sourceIpAddress, String
            destIpAddress, Duration duration)
    {
        Objects.requireNonNull(sourceIpAddress, "the ip of source can't be null");
        Objects.requireNonNull(destIpAddress, "the ip of dest can't be null");

        log.trace("[OT] Searching for traffic sent by " + sourceIpAddress + " to " + destIpAddress
                + " in the last " + duration.getSeconds() + " secs");
        //printNodesMapByIP(); causes StatkOverFlowError()

        Node nodeObs = getNodeObserver(observerIpAddress);
        if (nodeObs == null) {
            return 0;
        }
        Source sourceA = getSource(observerIpAddress, sourceIpAddress);
        if (sourceA == null) {
            return 0;
        }

        Link linkAB = sourceA.getDestinations().get(Utils.convertIPToInteger(destIpAddress));
        if (linkAB == null) {
            log.trace("Unable to get link, node obs (" + nodeObs.getId() + ") [" + destIpAddress + "] is not a " +
                    "destination in "
                    + "source [" + sourceIpAddress + "]");
            return 0;
        }

        long dataAgeInSeconds = TimeUtil.distance(linkAB.getTimestamp(), TimeUtil.getCurrentTime()).getSeconds();
        if (dataAgeInSeconds > duration.getSeconds()) {
            log.warn("The data's age: "
                    + dataAgeInSeconds
                    + " seconds is older that the provided interval ("
                    + duration.getSeconds() + ")");
            return 0;
        }

        List<Stat> stats = linkAB.getStatsList();
        if (stats == null) {
            log.trace("Unable to get list of stats, not present in node obs (" + nodeObs.getId() + ") IP [" +
                    observerIpAddress + "] link " + sourceIpAddress + " -> " + destIpAddress);
            return 0;
        }

        int sum = 0;

        switch (trafficType) {
            case SENT:
                for (Stat s : stats) {
                    sum += s.getSentFiveSec();
                }

                log.debug("[OUT-T] Outgoing traffic --> sent by " + sourceIpAddress + "\n"
                        + " to " + destIpAddress + "\n"
                        + " read from Node obs (" + nodeObs.getId() + " traffic stats"
                        + " in the last " + duration.getSeconds() + " secs is: " + sum + " bytes");
                break;
            case RECEIVED:
                for (Stat s : stats) {
                    sum += s.getReceivedFiveSec();
                }

                log.debug("[INC-T] Incoming traffic <-- received by " + destIpAddress + "\n"
                        + " from " + sourceIpAddress + "\n"
                        + " read from Node obs (" + nodeObs.getId() + " traffic stats"
                        + " in the last " + duration.getSeconds() + " secs is: " + sum + " bytes");
                break;
            case OBSERVED:
                for (Stat s : stats) {
                    sum += s.getObservedFiveSec();
                }

                log.debug("[OUT-T] Observed traffic --> sent by " + sourceIpAddress + "\n"
                        + " to " + destIpAddress + "\n"
                        + " read from Node obs (" + nodeObs.getId() + " traffic stats"
                        + " in the last " + duration.getSeconds() + " secs is: " + sum + " bytes");
                break;
            case LATENCY:
                MocketSensor ms = linkAB.getMocketSensor();
                if (ms == null) {
                    log.trace("Unable to get link Description, not present in node obs (" + nodeObs.getId() + ") IP [" +
                            observerIpAddress + "] link " + sourceIpAddress + " -> " + destIpAddress);
                    return 0;
                }


                int latency;

                if (ms.getRTT() > 0 && ms.getRTT() <= 2) {
                    latency = 1;
                }
                else {
                    latency = (int) ms.getRTT() / 2;
                }

                log.debug("[LAT] Latency <-> between " + sourceIpAddress + " and " + destIpAddress
                        + " in the last " + duration.getSeconds() + " secs is : " + latency);
                return latency;
        }


        return sum;
    }

    @Override
    public void setNetworkHealth (String networkName, NetworkHealth networkHealth)
    {
        log.debug("Setting NetworkHealth message for subnet: " + networkName);
        try {
            log.debug("[NH] -> " + JsonFormat.printer().print(networkHealth));
        }
        catch (InvalidProtocolBufferException e) {
            log.error("Error while printing NH", e);
        }
        //update local NodeMon
        updateData(getLocalNodeId(), ProtoUtils.toContainer(DataType.NETWORK_HEALTH, getLocalNodeId(), networkHealth));
        //update clients of local NodeMon through proxy
        updateClients(getLocalNodeId(), ProtoUtils.toContainer(DataType.NETWORK_HEALTH, getLocalNodeId(),
                networkHealth));
    }

    @Override
    public void updateLink (String observerIpAddress, Link link)
    {
        Node n = getNodeObserver(observerIpAddress);
        if (n == null) {
            return;
        }

        //update local NodeMon
        updateData(n.getId(), ProtoUtils.toContainer(DataType.LINK, getLocalNodeId(), link));
        //update clients of local NodeMon through proxy
        updateClients(n.getId(), ProtoUtils.toContainer(DataType.LINK, getLocalNodeId(), link));
    }

    @Override
    public NetworkHealth getNetworkHealth (String networkName)
    {
        Objects.requireNonNull(networkName, "the networkName can't be null");

        return getLocalNodeCopy().getNetworkHealth().get(networkName);
    }


    @Override
    public int getLatency (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration duration)
    {

//        log.info("[L} Latency <-- between " + ipAddressA + " and " + ipAddressB
//                + " in the last " + duration.getSeconds() + " secs is : " + latency);

        return getTraffic(TrafficType.LATENCY, observerIpAddress, sourceIpAddress, destIpAddress, duration);
    }

    @Override
    public ProcessStats getProcessStats ()
    {
        return _processStats;
    }

    @Override
    public ProcessStats getProcessStatsCopy ()
    {
        HashMap<String, us.ihmc.aci.nodemon.data.process.Process> defensiveMap = new HashMap<>();
        defensiveMap.putAll(_processStats.get());
        ProcessStats copy = new ProcessStats();
        copy.put(defensiveMap);
        return copy;
    }


    /**
     * Returns the most up-to-date <code>Collection</code> of <code>NODE</code> instances in this
     * <code>WorldState</code>.
     *
     * @return a <code>Collection</code> of <code>NODE</code> instances.
     */
    @Override
    public Collection<Node> getNodes ()
    {
        return _nodeMap.values();
    }

    /**
     * Returns a copy of the most up-to-date <code>Collection</code> of <code>NODE</code> instances in this
     * <code>WorldState</code>. This call is the expensive version of <code>getNodes()</code>.
     *
     * @return a <code>Collection</code> of <code>NODE</code> instances.
     */
    @Override
    public Map<String, Node> getNodesMapCopy ()
    {
        //defensive copy //expensive
        HashMap<String, Node> defensiveMap = new HashMap<>();
        defensiveMap.putAll(_nodeMap);
        return defensiveMap;
    }

    @Override
    public Map<String, Node> getNodesMapByIPCopy ()
    {
        //defensive copy //expensive
        HashMap<String, Node> defensiveMap = new HashMap<>();
        defensiveMap.putAll(_nodeMapByIP);
        return defensiveMap;
    }

    enum TrafficType
    {
        SENT,
        RECEIVED,
        OBSERVED,
        LATENCY
    }

    private final Map<String, Node> _nodeMap;
    private final Map<String, Node> _nodeMapByIP;
    private String _localNodeId;
    private String _localIPAddress;
    private ProcessStats _processStats;
    //private TrafficStats _trafficStats;
    private WorldStateController _worldStateController;
    private NodeMon _nodeMon;
    private static final Logger log = Logger.getLogger(BaseWorldState.class);
}
