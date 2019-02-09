package us.ihmc.aci.netviewer.proxy;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.netviewer.util.Event;
import us.ihmc.aci.nodemon.custom.DisServiceContent;
import us.ihmc.aci.nodemon.custom.MocketsContent;
import us.ihmc.aci.nodemon.custom.ProcessContentType;
import us.ihmc.aci.nodemon.data.*;
import us.ihmc.aci.nodemon.data.node.BaseNode;
import us.ihmc.aci.nodemon.data.node.Node;
import us.ihmc.aci.nodemon.data.node.core.BaseNeighbor;
import us.ihmc.aci.nodemon.data.node.core.Neighbor;
import us.ihmc.aci.nodemon.data.node.info.HardwareAbstractionLayer;
import us.ihmc.aci.nodemon.data.node.info.NetworkInfo;
import us.ihmc.aci.nodemon.data.node.info.NodeInfo;
import us.ihmc.aci.nodemon.data.node.stats.BaseNodeStats;
import us.ihmc.aci.nodemon.data.node.stats.BaseProcess;
import us.ihmc.aci.nodemon.data.node.stats.NodeStats;
import us.ihmc.aci.nodemon.data.traffic.TrafficParticle;
import us.ihmc.aci.nodemon.proxy.BaseNodeMonProxy;
import us.ihmc.aci.nodemon.proxy.NodeMonProxy;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;
import us.ihmc.util.serialization.SerializationException;

import java.util.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;

/**
 * Connector to receive updates from the node monitor
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NodeMonConnector implements NodeMonProxyListener
{
    /**
     * Constructor
     * @param clientName client name
     * @param host node monitor ip address
     * @param port node monitor binding port
     * @throws SerializationException if problems occur during the <code>DataSerializer</code> instance
     */
    public NodeMonConnector (String clientName, String host, int port) throws SerializationException
    {
        _proxy = new BaseNodeMonProxy ((short) 0, clientName, host, port);
        _dataSerializer = new DataSerializer();
        _listeners = new CopyOnWriteArrayList<>();
        _proxy.registerNodeMonProxyListener (this);
        _callbacksQueue = new LinkedBlockingDeque<>();
        _tmpCallbackQueue = new ArrayList<>();
        _nodes = new HashMap<>();
        _nodesInfoMap = new HashMap<>();
        _primaryIpAddressNodeIdMap = new HashMap<>();
        _ipAddressNodeIdMap = new HashMap<>();

        startCallbacksQueueThread();
    }

    /**
     * Adds a new listener to be notified when node monitor updates arrive
     * @param listener listener instance to be notified when node monitor updates arrive
     */
    public void addNodeMonUpdatesListener (NodeMonUpdatesListener listener)
    {
        if (listener != null) {
            _listeners.add (listener);
        }
    }

    /**
     * Tries to open the connection with the <code>NodeMonProxy</code>
     */
    public void startup()
    {
        new Thread (new Runnable()
        {
            @Override
            public void run()
            {
                while (!_proxy.connect()) {
                    try {
                        Thread.sleep (3000);
                    }
                    catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

                // Requesting the world state info
                _proxy.getWorldState();
            }
        }).start();
    }

    /**
     * Threads that reads objects from the callbacks queue and informs the listeners about node monitor updates
     */
    private void startCallbacksQueueThread()
    {
        Thread thread = new Thread (new Runnable()
        {
            @Override
            public void run()
            {
                while (true) {
                    try {
                        Event el = _callbacksQueue.poll ((long) 5, TimeUnit.SECONDS);
                        if (el == null) {
                            continue;
                        }

                        switch (el.getType()) {
                            case newNode:
                                newNodeRequest (el);
                                break;
                            case updatedNode:
                                updateNodeRequest (el);
                                break;
                            case deadNode:
                                deadNodeRequest (el);
                                break;
                        }
                    }
                    catch (InterruptedException e) {
                        log.error ("Problem in reading from the callbacks queue", e);
                        return;
                    }
                }
            }
        });
        thread.setName (this.getClass().getSimpleName() + "Thread");
        thread.start();
    }

    /**
     * Handles a notification for a new node
     * @param el queued callback object
     */
    private void newNodeRequest (Event el)
    {
        _nodes.put (el.getNodeId(), el.getNode());

        for (Neighbor neighbor : el.getNode().getNodeCore().getNeighbors()) {
            String neighborId = neighbor.getNodeId();
            if (_nodes.get (neighborId) == null) {
                continue;   // TODO: do something when _nodes doesn't contain it
            }
            if (!_nodes.get (neighborId).getNodeCore().getNeighbors().contains (new BaseNeighbor (el.getNodeId()))) {
                _nodes.get (neighborId).getNodeCore().getNeighbors().add (new BaseNeighbor (el.getNodeId()));
            }
        }

        UpdateContainer.Builder ucBuilder = new UpdateContainer.Builder();
        ucBuilder.nodeId = el.getNodeId();
        ucBuilder.nodeName = el.getNode().getNodeCore().getName();
        if ((el.getNode().getNodeInfo() != null) && (el.getNode().getNodeInfo().getHardwareAbstractionLayer() != null) &&
                el.getNode().getNodeInfo().getHardwareAbstractionLayer().getNetworkInfo() != null) {
            NetworkInfo[] networkInfo = el.getNode().getNodeInfo().getHardwareAbstractionLayer().getNetworkInfo();
            if (networkInfo.length != 0) {
                List<String> ips = new ArrayList<>();
                for (NetworkInfo ni : networkInfo) {
                    ips.add(ni.getIPAddress());
                    if (ni.isPrimary()) {
                        _primaryIpAddressNodeIdMap.put (ni.getIPAddress(), el.getNodeId());
                    }
                }
                ucBuilder.ips (ips);
                _ipAddressNodeIdMap.put (el.getNodeId(), ips);
            }
        }

        _nodesInfoMap.put (el.getNodeId(), createNodeInfo (el.getNode()));
        ucBuilder.nodeInfo = _nodesInfoMap.get (el.getNodeId());
//        ucBuilder.aggrInTraffic = extractAggInTraffic (el.getNode());
//        ucBuilder.aggrOutTraffic = extractAggOutTraffic (el.getNode());

        ucBuilder.trafficByIPMap = extractTrafficByIPMap (el.getNode().getNodeStats());

        if (el.getNode().getNodeStats().getTrafficSummary() != null) {
            // TODO
//            ucBuilder.incomingSummary = el.getNode().getNodeStats().getSummary().getIncoming();
//            ucBuilder.outgoingSummary = el.getNode().getNodeStats().getSummary().getOutgoing();
        }

        ucBuilder.neighborLinkInfo = createLinksInfo (el.getNode());

        for (NodeMonUpdatesListener listener : _listeners) {
            listener.newNode (ucBuilder.build());
        }
    }

    /**
     * Handles a notification for data updates
     * @param el queued callback object
     */
    private void updateNodeRequest (Event el)
    {
        if (_nodes.get (el.getNodeId()) == null) {
            log.error ("Received node update without receiving newNode first");
            return;
        }
        Node node;
        UpdateContainer.Builder ucBuilder = new UpdateContainer.Builder();

        try {
            switch (el.getNmdType()) {
                case NodeStats:
                    NodeStats data = _dataSerializer.deserialize (el.getBUpdatedData(), BaseNodeStats.class);
                    node = _nodes.get (el.getNodeId());
//                    Collection<Process> processes = data.getProcesses();
                    Collection<BaseProcess> processes = data.getProcesses();
                    for (BaseProcess process : processes) {
                        node.getNodeStats().setProcess (process.getId(), process); // TODO: finish
                    }

                    ucBuilder.trafficByIPMap = extractTrafficByIPMap (data);

                    if (data.getTrafficSummary() != null) {
                        // TODO
//                        ucBuilder.incomingSummary = data.getSummary().getIncoming();
//                        ucBuilder.outgoingSummary = data.getSummary().getOutgoing();
                    }

                    break;  // TODO: add other nmdType

                case Node:
                    node = _dataSerializer.deserialize (el.getBUpdatedData(), BaseNode.class);
                    _nodes.put (el.getNodeId(), node);
                    break;

                case CPUInfo:
                case HardwareAbstractionLayer:
                case Neighbor:
                case NetworkInfo:
                case NodeInfo:
                case OperatingSystem:
                    log.warn ("Update of type " + el.getNmdType() + " shouldn't have arrived");
                    return;

                default:
                    log.warn ("Wrong " + NodeMonDataType.class.getSimpleName() + " - Ignoring it");
                    return;
            }
        }
        catch (SerializationException e) {
            log.error ("Problem in deserializing the object " + el.getNmdType(), e);
            return;
        }


        ucBuilder.nodeId = node.getId();
        ucBuilder.nodeName = node.getNodeCore().getName();
        _nodesInfoMap.put (node.getId(), createNodeInfo (node));
        ucBuilder.nodeInfo = _nodesInfoMap.get (node.getId());
//        ucBuilder.aggrInTraffic = extractAggInTraffic (node);
//        ucBuilder.aggrOutTraffic = extractAggOutTraffic (node);
        ucBuilder.neighborLinkInfo = createLinksInfo (node);

        for (NodeMonUpdatesListener listener : _listeners) {
            listener.updateNode (ucBuilder.build());
        }
    }

    /**
     * Creates the <code>String</code> containing the node info
     * @param node node instance to analyze
     * @return the <code>String</code> containing the node info
     */
    private String createNodeInfo (Node node)
    {
        StringBuilder sb = new StringBuilder();

        sb.append (SummaryBuilder.buildNodeId (node));
        sb.append (SummaryBuilder.buildNodeName (node));
        sb.append (SummaryBuilder.buildNeighbors (node));

        NodeInfo nodeInfo = node.getNodeInfo();
        if (nodeInfo != null) {
            sb.append (SummaryBuilder.buildOperatingSystem (nodeInfo.getOperatingSystem()));

            HardwareAbstractionLayer hardwareAbstractionLayer = node.getNodeInfo().getHardwareAbstractionLayer();
            if (hardwareAbstractionLayer != null) {
                sb.append (SummaryBuilder.buildCPIInfo (hardwareAbstractionLayer.getCPUInfo()));
                sb.append (SummaryBuilder.buildNetworkInfo (hardwareAbstractionLayer.getNetworkInfo()));
            }
        }

        for (BaseProcess process : node.getNodeStats().getProcesses()) {
            if (!process.getType().equals (ProcessContentType.DisService)) {
                continue;
            }

            sb.append (SummaryBuilder.buildDisServiceStats ((DisServiceContent) process.getContent()));
            break; // TODO: now only the first disservice custom process is considered
        }

        return sb.toString();
    }

    /**
     * Extracts from the <code>NodeStats</code> the traffic statistics grouped by ip
     * @param nodeStats <code>NodeStats</code> instance containing the statistics
     * @return the traffic statistics grouped by ip
     */
    private Map<String, TrafficParticle> extractTrafficByIPMap (NodeStats nodeStats)
    {
        Map<String, TrafficParticle> traffic = new HashMap<>();

        if ((nodeStats == null) || (nodeStats.getTrafficSummary() == null)) {
            return traffic;
        }

        for (Node knownNode : _nodes.values()) {
            // TODO
//            for (String ip : _ipAddressNodeIdMap.get (knownNode.getId())) {
//                if (nodeStats.getSummary().getByIP (ip) != null) {
//                    traffic.put (ip, nodeStats.getSummary().getByIP (ip));
//                }
//            }
        }

        return traffic;
    }

    /**
     * Creates the <code>String</code> containing the links info
     * @param node node instance to analyze
     * @return the <code>String</code> containing the links info
     */
    private Map<String, NeighborLinkInfo> createLinksInfo (Node node)
    {
        Map<String, NeighborLinkInfo> linksInfo = new HashMap<>();
        for (Neighbor neighbor : node.getNodeCore().getNeighbors()) {
            StringBuilder sb = new StringBuilder();
            for (BaseProcess process : node.getNodeStats().getProcesses()) {
                switch (process.getType()) {
                    case Mockets:
                        MocketsContent content = (MocketsContent) process.getContent();
                        if (!neighbor.getNodeId().equals (_primaryIpAddressNodeIdMap.get (content.getRemoteAddr()))) {
                            continue;
                        }
                        sb.append (SummaryBuilder.buildMockets (content));
                        break;
                }
            }

            linksInfo.put (neighbor.getNodeId(), NeighborLinkInfo.create (sb.toString()));
        }

        return linksInfo;
    }

    /**
     * Handles a notification for a dead node
     * @param el queued callback object
     */
    private void deadNodeRequest (Event el)
    {
        _nodes.remove (el.getNodeId());

        for (NodeMonUpdatesListener listener : _listeners) {
            listener.deadNode (el.getNodeId());
        }
    }

    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing NodeMonProxyListener                                                                -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public String getId ()
    {
        return null;
    }

    @Override
    public synchronized void worldStateUpdate (Collection<Node> worldState)
    {
        log.info ("Received worldStateUpdate notification");
        _receivedWorldState = true;

        for (Node n : worldState) {
            if ((n == null) || (n.getId() == null)) {
                continue;
            }

            _callbacksQueue.add (new Event (n));
            log.info ("Added node " + n.getNodeCore().getName() + " to the callbacks queue (from worldStateUpdate)");

            _callbacksQueue.addAll (_tmpCallbackQueue);
        }
    }

    @Override
    public synchronized void updateData (String nodeId, NodeMonDataType type, byte[] data)
    {
        log.info ("Received updateData notification");
        if ((data == null) || (nodeId == null)) {
            return;
        }
        log.info ("Updated node id: " + nodeId);

        if (!_receivedWorldState) {
            _tmpCallbackQueue.add (new Event (nodeId, type, data));
        }
        else {
            _callbacksQueue.add (new Event (nodeId, type, data));
        }
        log.info("Added updated data to the callbacks queue (from update)");
    }

    @Override
    public synchronized void newNode (Node n)
    {
        log.info ("Received newNode notification");
        if ((n == null) || (n.getId() == null)) {
            return;
        }
        log.info ("New node id: " + n.getId());

        if (!_receivedWorldState) {
            _tmpCallbackQueue.add (new Event (n));
        }
        else {
            _callbacksQueue.add (new Event (n));
        }
        log.info ("Added node " + n.getNodeCore().getName() + " to the callbacks queue (from newNode)");
    }

    @Override
    public synchronized void deadNode (String nodeId)
    {
        log.info ("Received deadNode notification for " + nodeId);
        if (nodeId == null) {
            return;
        }

        if (!_receivedWorldState) {
            _tmpCallbackQueue.add (new Event(nodeId));
        }
        else {
            _callbacksQueue.add (new Event(nodeId));
        }
        log.info ("Added node " + nodeId + " to the callbacks queue (from deadNode)");
    }

    @Override
    public void connectionClosed()
    {
        log.info ("Received a connection closed call");
        for (NodeMonUpdatesListener listener : _listeners) {
            listener.connectionClosed();
        }

        // Restarting the thread to try to connect to the node monitor proxy
        startup();
    }

    // <-------------------------------------------------------------------------------------------------------------->


    private final NodeMonProxy _proxy;
    private final DataSerializer _dataSerializer;
    private final List<NodeMonUpdatesListener> _listeners;
    private final BlockingQueue<Event> _callbacksQueue;
    private boolean _receivedWorldState = false;
    private final List<Event> _tmpCallbackQueue;   // used to store callback messages received before the world state update
    private final Map<String, Node> _nodes;
    private final Map<String, String> _nodesInfoMap;
    private final Map<String, String> _primaryIpAddressNodeIdMap;
    private final Map<String, List<String>> _ipAddressNodeIdMap;    // key = node id, value = list of ips

    private static final Logger log = LoggerFactory.getLogger (NodeMonConnector.class);
}
