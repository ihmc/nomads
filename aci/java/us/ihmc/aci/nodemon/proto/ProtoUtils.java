package us.ihmc.aci.nodemon.proto;

import com.google.protobuf.GeneratedMessage;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Timestamp;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.util.Utils;

import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 * ProtoUtils.java
 * <p/>
 * Class <code>ProtoUtils</code> contains utility methods to operate over Google's Protocol Buffers objects used in
 * <code>NodeMon</code>.
 */
public class ProtoUtils
{

    /**
     * Converts this <code>ReadableNode</code> object to the compact, machine optimized <code>Node</code>.
     *
     * @param readableNode a <code>ReadableNode</code> object.
     * @return a compact, machine optimized <code>Node</code>
     */
    public static Node toNode (ReadableNode readableNode)
    {
        Objects.requireNonNull(readableNode, "ReadableNode can't be null");
        Node.Builder nodeBuilder = Node.newBuilder();
        nodeBuilder.setId(readableNode.getId());
        nodeBuilder.setName(readableNode.getName());
        nodeBuilder.setInfo(readableNode.getInfo());
        nodeBuilder.setGrump(readableNode.getGrump());
        nodeBuilder.setTraffic(toTraffic(readableNode.getTraffic()));
        for (String networkName : readableNode.getTopology().keySet()) {
            Topology t = toTraffic(readableNode.getTopology().get(networkName));
            nodeBuilder.getMutableTopology().put(networkName, t);
        }
        nodeBuilder.setTimestamp(readableNode.getTimestamp());
        return nodeBuilder.build();
    }

    /**
     * Converts this <code>Node</code> object to a human-readable <code>ReadableNode</code>.
     *
     * @param node a <code>Node</code> object.
     * @return a human-readable <code>ReadableNode</code>
     */
    public static ReadableNode toReadable (Node node)
    {
        Objects.requireNonNull(node, "Node can't be null");
        ReadableNode.Builder readNodeBuilder = ReadableNode.newBuilder();
        readNodeBuilder.setId(node.getId());
        readNodeBuilder.setName(node.getName());
        readNodeBuilder.setInfo(node.getInfo());
        readNodeBuilder.setGrump(node.getGrump());
        readNodeBuilder.setTraffic(toReadable(node.getTraffic()));
        for (String networkName : node.getTopology().keySet()) {
            ReadableTopology rt = toReadable(node.getTopology().get(networkName));
            readNodeBuilder.getMutableTopology().put(networkName, rt);
        }
        readNodeBuilder.setTimestamp(node.getTimestamp());
        return readNodeBuilder.build();
    }

    /**
     * Converts this <code>Link</code> object to a human-readable <code>ReadableLink</code>.
     *
     * @param link a <code>Link</code> object.
     * @return a human-readable <code>ReadableLink</code>
     */
    public static ReadableLink toReadable (Link link)
    {
        Objects.requireNonNull(link, "Link can't be null");
        String srcIp = Utils.convertIPToString(link.getIpSrc());
        String destIp = Utils.convertIPToString(link.getIpDst());

        return ReadableLink.newBuilder().setDescription(link.getDescription())
                .addAllStats(link.getStatsList())
                .setMocketSensor(link.getMocketSensor())
                .setIpSrc(srcIp)
                .setIpDst(destIp)
                .setTimestamp(link.getTimestamp())
                .build();
    }

    /**
     * Converts this <code>ReadableLink</code> object to a compact, machine optimized <code>Link</code>.
     *
     * @param readableLink a <code>ReadableLink</code> object.
     * @return a compact, machine optimized <code>Link</code>, null if conversion of ip to integer fails
     */
    public static Link toLink (ReadableLink readableLink)
    {
        Objects.requireNonNull(readableLink, "Link can't be null");
        Integer srcIp = Utils.convertIPToInteger(readableLink.getIpSrc());
        if (srcIp == null) {
            log.warn("Unable to convert: " + readableLink.getIpSrc() + " to integer, returning null");
            return null;
        }
        Integer destIp = Utils.convertIPToInteger(readableLink.getIpDst());
        if (destIp == null) {
            log.warn("Unable to convert: " + readableLink.getIpDst() + " to integer, returning null");
            return null;
        }

        return Link.newBuilder().setDescription(readableLink.getDescription())
                .addAllStats(readableLink.getStatsList())
                .setMocketSensor(readableLink.getMocketSensor())
                .setIpSrc(srcIp)
                .setIpDst(destIp)
                .setTimestamp(readableLink.getTimestamp())
                .build();
    }

    /**
     * Converts this <code>Traffic</code> object to a human-readable <code>ReadableTraffic</code>.
     *
     * @param traffic a <code>Traffic</code> object.
     * @return a human-readable <code>ReadableTraffic</code>
     */
    public static ReadableTraffic toReadable (Traffic traffic)
    {
        Objects.requireNonNull(traffic, "Traffic can't be null");
        ReadableTraffic rt;
        Map<String, ReadableSource> readableSources = new HashMap<>();
        for (Integer srcIpInt : traffic.getSources().keySet()) {
            String srcIp = Utils.convertIPToString(srcIpInt);
            Source s = traffic.getSources().get(srcIpInt);
            Map<String, ReadableLink> destinations = new HashMap<>();
            for (Integer destIpInt : s.getDestinations().keySet()) {
                String destIp = Utils.convertIPToString(destIpInt);
                Link l = s.getDestinations().get(destIpInt);
                ReadableLink rl = ReadableLink.newBuilder()
                        .setDescription(l.getDescription())
                        .addAllStats(l.getStatsList())
                        .setMocketSensor(l.getMocketSensor())
                        .setIpSrc(srcIp)
                        .setIpDst(destIp)
                        .setTimestamp(l.getTimestamp())
                        .build();
                destinations.put(destIp, rl);
            }
            ReadableSource rs = ReadableSource.newBuilder()
                    .putAllDestinations(destinations)
                    .build();
            readableSources.put(srcIp, rs);
            //
        }
        rt = ReadableTraffic.newBuilder()
                .putAllSources(readableSources)
                .build();

        return rt;
    }

    /**
     * Converts this <code>ReadableTraffic</code> object to a compact, machine optimized <code>Traffic</code>.
     *
     * @param readableTraffic a <code>ReadableTraffic</code> object.
     * @return a compact, machine optimized <code>Traffic</code>.
     */
    public static Traffic toTraffic (ReadableTraffic readableTraffic)
    {
        Objects.requireNonNull(readableTraffic, "Traffic can't be null");
        Traffic t;
        Map<Integer, Source> sources = new HashMap<>();
        for (String srcIp : readableTraffic.getSources().keySet()) {
            Integer srcIpInt = Utils.convertIPToInteger(srcIp);
            if (srcIpInt == null) {
                log.warn("Unable to convert: " + srcIp + " to integer, skipping source");
                continue;
            }
            ReadableSource rs = readableTraffic.getSources().get(srcIp);
            Map<Integer, Link> destinations = new HashMap<>();
            for (String destIp : rs.getDestinations().keySet()) {
                Integer destIpInt = Utils.convertIPToInteger(destIp);
                if (destIpInt == null) {
                    log.warn("Unable to convert: " + destIp + " to integer, skipping link");
                    continue;
                }
                ReadableLink rl = rs.getDestinations().get(destIp);
                Link l = Link.newBuilder().setDescription(rl.getDescription())
                        .addAllStats(rl.getStatsList())
                        .setIpSrc(srcIpInt)
                        .setIpDst(destIpInt)
                        .setTimestamp(rl.getTimestamp())
                        .build();
                destinations.put(destIpInt, l);
            }
            Source s = Source.newBuilder()
                    .putAllDestinations(destinations)
                    .build();
            sources.put(srcIpInt, s);
            //
        }
        t = Traffic.newBuilder()
                .putAllSources(sources)
                .build();

        return t;
    }


    /**
     * Converts this <code>Topology</code> object to a human-readable <code>ReadableTopology</code>.
     *
     * @param topo a <code>Topology</code> object.
     * @return a human-readable <code>ReadableTopology</code>
     */
    public static ReadableTopology toReadable (Topology topo)
    {
        Objects.requireNonNull(topo, "Topology can't be null");
        Map<String, ReadableHost> internals = new HashMap<>();
        for (Host h : topo.getInternals().values()) {
            String ip = Utils.convertIPToString(h.getIp());
            internals.put(ip, ReadableHost.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }
        Map<String, ReadableHost> localGws = new HashMap<>();
        for (Host h : topo.getLocalGws().values()) {
            String ip = Utils.convertIPToString(h.getIp());
            localGws.put(ip, ReadableHost.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }
        Map<String, ReadableHost> remoteGws = new HashMap<>();
        for (Host h : topo.getRemoteGws().values()) {
            String ip = Utils.convertIPToString(h.getIp());
            remoteGws.put(ip, ReadableHost.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }
        Map<String, ReadableHost> externals = new HashMap<>();
        for (Host h : topo.getExternals().values()) {
            String ip = Utils.convertIPToString(h.getIp());
            externals.put(ip, ReadableHost.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }

        return ReadableTopology.newBuilder()
                .setNetworkName(topo.getNetworkName())
                .setSubnetMask(topo.getSubnetMask())
                .putAllInternals(internals)
                .putAllLocalGws(localGws)
                .putAllRemoteGws(remoteGws)
                .putAllExternals(externals)
                .setTimestamp(topo.getTimestamp())
                .build();
    }

    /**
     * Packs this <code>GeneratedMessage</code> into a <code>Container</code>
     *
     * @param type the <code>DataType</code> of the object
     * @param gm   the <code>GeneratedMessage</code> instance
     * @return a container
     */
    public static Container toContainer (DataType type,
                                         MessageType messageType,
                                         TransportType transportType,
                                         String senderId,
                                         String recipientId,
                                         String dataNodeId,
                                         GeneratedMessage gm)
    {
        Container.Builder cb = Container.newBuilder();
        cb.setDataType(type);
        cb.setMessageType(messageType);
        cb.setTransportType(transportType);
        cb.setSenderId(senderId);
        cb.setRecipientId(recipientId);
        cb.setDataNodeId(dataNodeId);
        switch (type) {
            case INFO:
                cb.setInfo((Info) gm);
                cb.setTimestamp(((Info) gm).getTimestamp());
                break;
            case GROUP:
                List<Group> groupList = new ArrayList<>();
                groupList.add((Group) gm);
                cb.addAllGroups(groupList);
                break;
            case LINK:
                List<Link> linkList = new ArrayList<>();
                linkList.add((Link) gm);
                cb.addAllLinks(linkList);
                cb.setTimestamp(((Link) gm).getTimestamp());
                break;
            case TOPOLOGY:
                cb.setTopology((Topology) gm);
                cb.setTimestamp(((Topology) gm).getTimestamp());
                break;
            case TOPOLOGY_PARTS:
                cb.setTopologyParts((TopologyParts) gm);
                cb.setTimestamp(((TopologyParts) gm).getTimestamp());
                break;
            case NETWORK_HEALTH:
                cb.setNetworkHealth((NetworkHealth) gm);
                cb.setTimestamp(((NetworkHealth) gm).getCreationTime());
                break;
            case TRAFFIC:
                cb.setTraffic((Traffic) gm);
                break;
            case NODE:
                cb.setNode((Node) gm);
                cb.setTimestamp(((Node) gm).getTimestamp());
                break;
            default:
                log.error("Unsupported type passed to toContainer() method");
                throw new UnsupportedOperationException();
        }

        return cb.build();
    }

    /**
     * Packs this <code>List<Link></code> into a <code>Container</code>
     *
     * @param dataNodeId the id of the node associated with the data
     * @param linkList   the a list of links
     * @return a container
     */
    public static Container toContainer (String dataNodeId, List<Link> linkList)
    {
        Container.Builder cb = Container.newBuilder();
        cb.setDataType(DataType.LINK);
        cb.setDataNodeId(dataNodeId);
        cb.addAllLinks(linkList);
        cb.setTimestamp(TimeUtil.getCurrentTime());
        return cb.build();
    }


    /**
     * Packs this <code>GeneratedMessage</code> into a <code>Container</code>
     *
     * @param type       the <code>DataType</code> of the object
     * @param dataNodeId the id of the node associated with the data
     * @param gm         the <code>GeneratedMessage</code> instance
     * @return a container
     */
    public static Container toContainer (DataType type, String dataNodeId, GeneratedMessage gm)
    {
        Container.Builder cb = Container.newBuilder();
        cb.setDataType(type);
        cb.setDataNodeId(dataNodeId);
        switch (type) {
            case INFO:
                cb.setInfo((Info) gm);
                cb.setTimestamp(((Info) gm).getTimestamp());
                break;
            case GROUP:
                List<Group> groupList = new ArrayList<>();
                groupList.add((Group) gm);
                cb.addAllGroups(groupList);
                cb.setTimestamp(TimeUtil.getCurrentTime());
                break;
            case LINK:
                List<Link> linkList = new ArrayList<>();
                linkList.add((Link) gm);
                cb.addAllLinks(linkList);
                cb.setTimestamp(((Link) gm).getTimestamp());
                break;
            case TOPOLOGY:
                cb.setTopology((Topology) gm);
                cb.setTimestamp(((Topology) gm).getTimestamp());
                //TODO hack: unable to get correct Topology from NetSensor, write current Timestamp
//                cb.setTimestamp(TimeUtil.getCurrentTime());
                break;
            case TOPOLOGY_PARTS:
                cb.setTopologyParts((TopologyParts) gm);
                cb.setTimestamp(((TopologyParts) gm).getTimestamp());
                break;
            case NETWORK_HEALTH:
                cb.setNetworkHealth((NetworkHealth) gm);
                cb.setTimestamp(((NetworkHealth) gm).getCreationTime());
                break;
            case TRAFFIC:
                cb.setTraffic((Traffic) gm);
                cb.setTimestamp(TimeUtil.getCurrentTime());
                break;
            case NODE:
                cb.setNode((Node) gm);
                cb.setTimestamp(((Node) gm).getTimestamp());
                break;
            default:
                log.error("Unsupported type passed to toContainer() method");
                throw new UnsupportedOperationException();
        }

        return cb.build();
    }

    /**
     * Packs this <code>GeneratedMessage</code> into a <code>Container</code>
     *
     * @return a container
     */
    public static Container toContainer (Container c,
                                         DataType type,
                                         MessageType messageType,
                                         TransportType transportType,
                                         String senderId,
                                         String recipientId,
                                         String dataNodeId)
    {
        Container.Builder cb = Container.newBuilder(c);
        cb.setDataType(type);
        cb.setMessageType(messageType);
        cb.setTransportType(transportType);
        cb.setSenderId(senderId);
        cb.setRecipientId(recipientId);
        cb.setDataNodeId(dataNodeId);
        cb.setTimestamp(c.getTimestamp());
        return cb.build();
    }


    /**
     * Converts this <code>Topology</code> object to a human-readable <code>ReadableTopology</code>.
     *
     * @param readableTopology a <code>ReadableTopology</code> object.
     * @return a compact, machine optimized <codeTopology</code>
     */
    public static Topology toTraffic (ReadableTopology readableTopology)
    {
        Objects.requireNonNull(readableTopology, "Topology can't be null");
        Map<Integer, Host> internals = new HashMap<>();
        for (ReadableHost h : readableTopology.getInternals().values()) {
            Integer ip = Utils.convertIPToInteger(h.getIp());
            if (ip == null) {
                log.warn("Unable to convert " + h.getIp() + " to integer, skipping");
                continue;
            }
            internals.put(ip, Host.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }
        Map<Integer, Host> localGws = new HashMap<>();
        for (ReadableHost h : readableTopology.getLocalGws().values()) {
            Integer ip = Utils.convertIPToInteger(h.getIp());
            if (ip == null) {
                log.warn("Unable to convert " + h.getIp() + " to integer, skipping");
                continue;
            }
            localGws.put(ip, Host.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }
        Map<Integer, Host> remoteGws = new HashMap<>();
        for (ReadableHost h : readableTopology.getRemoteGws().values()) {
            Integer ip = Utils.convertIPToInteger(h.getIp());
            if (ip == null) {
                log.warn("Unable to convert " + h.getIp() + " to integer, skipping");
                continue;
            }
            remoteGws.put(ip, Host.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }
        Map<Integer, Host> externals = new HashMap<>();
        for (ReadableHost h : readableTopology.getExternals().values()) {
            Integer ip = Utils.convertIPToInteger(h.getIp());
            if (ip == null) {
                log.warn("Unable to convert " + h.getIp() + " to integer, skipping");
                continue;
            }
            externals.put(ip, Host.newBuilder()
                    .setIp(ip)
                    .setMac(h.getMac())
                    .setGatewayName(h.getGatewayName())
                    .setIsDefault(h.getIsDefault()).build());
        }

        return Topology.newBuilder()
                .setNetworkName(readableTopology.getNetworkName())
                .setSubnetMask(readableTopology.getSubnetMask())
                .putAllInternals(internals)
                .putAllLocalGws(localGws)
                .putAllRemoteGws(remoteGws)
                .putAllExternals(externals)
                .setTimestamp(readableTopology.getTimestamp())
                .build();
    }

    public static Node merge (Node oldNode, Link link)
    {
        Source.Builder sourceBuilder = null;
        Traffic.Builder trafficBuilder = Traffic.newBuilder(oldNode.getTraffic());
        Source mutSource = trafficBuilder.getMutableSources().get(link.getIpSrc());
        if (mutSource != null) {
            sourceBuilder = Source.newBuilder(mutSource);
            sourceBuilder.putAllDestinations(mutSource.getDestinations());
        }
        else {
            sourceBuilder = Source.newBuilder();
        }

        //take old Description
        Link oldLink = sourceBuilder.getMutableDestinations().get(link.getIpDst());

        //check timestamp, if new link is older don't merge
//        if (oldLink != null
//                && TimeUtil.distance(oldLink.getTimestamp(), link.getTimestamp()).getNanos() <= 0
//                && link.getMocketSensor() != null
//                && link.getMocketSensor().getRTT() == 0) {
//            int distance = TimeUtil.distance(oldLink.getTimestamp(), link.getTimestamp()).getNanos();
//            log.debug("!!! Not merging, older link " + toReadable(link).getIpSrc() + " -> " + toReadable(link)
//                    .getIpDst() + ": " + distance);
//            return oldNode;
//        }

        //replace link
        if (oldLink == null) {
            oldLink = link;
        }

        Link.Builder newLinkBuilder = Link.newBuilder(oldLink)
//                .setTimestamp(TimeUtil.getCurrentTime());
                .setTimestamp(Timestamp.newBuilder(link.getTimestamp())
                        .setSeconds(link.getTimestamp().getSeconds())
                        .setNanos(link.getTimestamp().getNanos())
                        .build());

        //merging NetSensor
        //TODO verify this
        if (!link.getStatsList().isEmpty()) {
            newLinkBuilder.clearStats();
            newLinkBuilder.addAllStats(link.getStatsList());
        }

        //merging MocketSensor
        if (link.getMocketSensor().getRTT() != 0) {
            log.debug("!!! Updating mockets RTT from new Link " + toReadable(link).getIpSrc() + " -> " + toReadable
                    (link).getIpDst());
            newLinkBuilder.setMocketSensor(link.getMocketSensor());
        }
        else {
            newLinkBuilder.setMocketSensor(oldLink.getMocketSensor());
        }

        sourceBuilder.getMutableDestinations().put(newLinkBuilder.getIpDst(), newLinkBuilder.build());
        Source source = sourceBuilder.build();
        trafficBuilder.getMutableSources().put(newLinkBuilder.getIpSrc(), source);
        return Node.newBuilder(oldNode)
                .setTraffic(trafficBuilder.build())
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();
    }

    public static Node merge (Node oldNode, Group group)
    {
        //check if we have groups
        Map<String, Group> groups = oldNode.getGrump().getGroups();
        if (groups == null) {
            groups = new HashMap<>();
        }

        Group newGroup;
        Group oldGroup = groups.get(group.getName());
        Map<String, Group> newGroups = new HashMap<>();
        if (oldGroup != null) {
            newGroup = Group.newBuilder(oldGroup).putAllMembers(group.getMembers()).build();
        }
        else {
            newGroup = group;
        }
        newGroups.put(newGroup.getName(), newGroup);

        Grump newGrump = Grump.newBuilder(oldNode.getGrump())
                .putAllGroups(newGroups)
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();

        return Node.newBuilder(oldNode)
                .setGrump(newGrump)
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();
    }

    public static Node merge (Node oldNode, Info info)
    {
        //replace old info
        return Node.newBuilder(oldNode)
                .setInfo(info)
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();
    }

    public static Node merge (Node oldNode, Traffic traffic)
    {
        Traffic.Builder trafficBuilder = Traffic.newBuilder(oldNode.getTraffic());
        for (Integer ipSrc : traffic.getSources().keySet()) {
            Source source = traffic.getSources().get(ipSrc);
            Source.Builder sourceBuilder = Source.newBuilder(source);
            sourceBuilder.putAllDestinations(source.getDestinations());
            trafficBuilder.getMutableSources().put(ipSrc, sourceBuilder.build());
        }
        Traffic newTraffic = trafficBuilder.build();
        return Node.newBuilder(oldNode)
                .setTraffic(newTraffic)
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();
    }

    public static Node merge (Node oldNode, Topology topology)
    {
        String networkName = topology.getNetworkName();
        Topology oldTopology = oldNode.getTopology().get(networkName);
        if (oldTopology == null) {
            oldTopology = Topology.newBuilder().build();
        }

        Topology.Builder topoBuilder = Topology.newBuilder(oldTopology);
        topoBuilder.setNetworkName(topology.getNetworkName());
        topoBuilder.setSubnetMask(topology.getSubnetMask());
        topoBuilder.putAllInternals(topology.getInternals());
        topoBuilder.putAllExternals(topology.getExternals());
        topoBuilder.putAllRemoteGws(topology.getRemoteGws());
        topoBuilder.putAllLocalGws(topology.getLocalGws());
        //topoBuilder.setTimestamp(TimeUtil.getCurrentTime());
        //updating timestamp
        topoBuilder.setTimestamp(Timestamp.newBuilder(topology.getTimestamp())
                .setSeconds(topology.getTimestamp().getSeconds())
                .setNanos(topology.getTimestamp().getNanos())
                .build());

        Node.Builder nodeBuilder = Node.newBuilder(oldNode);
        nodeBuilder.getMutableTopology().put(topology.getNetworkName(), topoBuilder.build());
        nodeBuilder.setTimestamp(TimeUtil.getCurrentTime());
        return nodeBuilder.build();
    }

    public static Node merge (Node oldNode, NetworkHealth networkHealth)
    {
        //replace completely NetworkHealth, no merge
        Node.Builder nodeBuilder = Node.newBuilder(oldNode);
        nodeBuilder.getMutableNetworkHealth().put(networkHealth.getNetworkName(), networkHealth);
        nodeBuilder.setTimestamp(TimeUtil.getCurrentTime());
        return nodeBuilder.build();
    }

    public static void printDetails (Container c)
    {
        try {
            if (c.getDataType().equals(DataType.LINK)) {
                List<Link> linkList = c.getLinksList();
                if (linkList == null) {
                    return;
                }
                for (Link l : linkList) {

                    log.debug(" * (1 of " + linkList.size() + ") LINK * " + toReadable(l).getIpSrc() +
                            " -> " + toReadable(l).getIpDst() +
                            " ts: " + JsonFormat.printer().print(l.getTimestamp()) +
                            " size: " + l.getSerializedSize());
                    break;
                }
            }
            else if (c.getDataType().equals(DataType.INFO)) {
                Info i = c.getInfo();
                if (i == null) {
                    return;
                }
                Node n = Node.newBuilder().setInfo(i).build();
                log.debug(" * INFO *" + Utils.getPrimaryIP(n) + " ts: " + JsonFormat.printer()
                        .print(i.getTimestamp())
                        + " size: " + i.getSerializedSize());
            }
            else if (c.getDataType().equals(DataType.NETWORK_HEALTH)) {
                NetworkHealth nh = c.getNetworkHealth();
                if (nh == null) {
                    return;
                }
                log.debug(" * NETWORK_HEALTH * ts: "
                        + JsonFormat.printer().print(nh.getCreationTime()) + " size: " + nh.getSerializedSize());
            }

        }
        catch (InvalidProtocolBufferException e) {
            e.printStackTrace();
        }
    }


    private static final Logger log = Logger.getLogger(ProtoUtils.class);
}
