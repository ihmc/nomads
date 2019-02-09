package us.ihmc.aci.nodemon.controllers.throughput;

import com.google.protobuf.Timestamp;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.Controller;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.scheduler.Scheduler;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.util.Config;

import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * ThroughputController.java
 * <p/>
 * Class <code>ThroughputController</code> regulates the throughput at which the <code>NodeMon</code> sends updates.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ThroughputController implements Runnable, Controller
{
    public ThroughputController (NodeMon nodeMon,
                                 Scheduler scheduler,
                                 TransportType type,
                                 int mtu,
                                 int queueSize)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _transportType = Objects.requireNonNull(type, "TransportType can't be null");
        _scheduler = Objects.requireNonNull(scheduler, "Scheduler can't be null");
        _isRunning = new AtomicBoolean(false);

        _isLocalDelivery = (!_transportType.equals(TransportType.TCP)) &&
                Config.getBooleanValue(Conf.NodeMon.NETWORK_LOCAL_ENABLE, DefaultValues.NodeMon.NETWORK_LOCAL_ENABLE);
        _isMastersDelivery = (!_transportType.equals(TransportType.TCP)) &&
                Config.getBooleanValue(Conf.NodeMon.NETWORK_MASTERS_ENABLE, DefaultValues.NodeMon
                        .NETWORK_MASTERS_ENABLE);
        _isClientsDelivery = _transportType.equals(TransportType.TCP);

        log.debug("ThroughputController init: " + _transportType + " LD: " + _isLocalDelivery + " MD: " + _isMastersDelivery
                + " CD: " + _isClientsDelivery);

        //node has to be a master to enable master delivery
//        boolean isMaster = Config.getBooleanValue(Conf.NodeMon.GROUPS_IS_MASTER,
//                DefaultValues.NodeMon.GROUPS_IS_MASTER);
        MTU = mtu;
        QUEUE_SIZE = queueSize;
        _outputQueue = new ArrayBlockingQueue<>(QUEUE_SIZE);
    }

    public void addMessage (Container c)
    {
        _outputQueue.add(c);
    }

    @Override
    public boolean isRunning ()
    {
        return _isRunning.get();
    }

    @Override
    public void start ()
    {
        (new Thread(this, _transportType + " Thread")).start();
    }

    @Override
    public void stop ()
    {
        _isRunning.set(false);
    }

    @Override
    public void run ()
    {
        _isRunning.set(true);
        int sentDataSize = 0;

        while (_isRunning.get()) {


            Container c = null;
            try {

                c = _outputQueue.poll(Long.MAX_VALUE, TimeUnit.DAYS);
                if (c == null) {
                    log.warn("Polled null message, skipping");
                    continue;
                }


                //if size of the message > MAX THROUGHPUT, split
                if (c.getSerializedSize() > MTU) {
                    log.trace(_transportType + " Message of type: " + c.getDataType() + " and size: " + c
                            .getSerializedSize()
                            + " exceeds max allowed throughput of " + MTU);

                    switch (c.getDataType()) {
                        case LINK:
                            List<Link> packedList = new ArrayList<>();
                            for (Link l : c.getLinksList()) {
                                if (sentDataSize >= MTU) {
                                    addMessageToScheduler(ProtoUtils.toContainer(c.getDataNodeId(), packedList));
                                    sentDataSize = 0;
                                    packedList = new ArrayList<>();
                                }
                                packedList.add(l);
                                sentDataSize += l.getSerializedSize();
                            }

                            //send leftovers
                            if (packedList.size() > 0) {
                                addMessageToScheduler(ProtoUtils.toContainer(c.getDataNodeId(), packedList));
                            }

                            break;
                        case TOPOLOGY:
                            Topology topology = c.getTopology();
                            Topology.Builder topoBuilder = Topology.newBuilder();
                            topoBuilder.setNetworkName(topology.getNetworkName());
                            topoBuilder.setSubnetMask(topology.getSubnetMask());
                            Map<Integer, Host> internals = new HashMap<>();
                            //internals
                            for (Integer ip : topology.getInternals().keySet()) {
                                Host h = topology.getInternals().get(ip);
                                sentDataSize += h.getSerializedSize();
                                if (sentDataSize < MTU) {
                                    internals.put(ip, h);
                                }
                                else {
                                    topoBuilder.putAllInternals(internals);
                                    topoBuilder.setTimestamp(Timestamp.newBuilder(topology.getTimestamp())
                                            .setSeconds(topology.getTimestamp().getSeconds())
                                            .setNanos(topology.getTimestamp().getNanos())
                                            .build());
                                    Topology topo = topoBuilder.build();
                                    addMessageToScheduler(ProtoUtils.toContainer(c.getDataType(), c.getDataNodeId(),
                                            topo));
                                    //TimeUnit.MILLISECONDS.sleep(THROUGHPUT_INTERVAL);
                                    topoBuilder = Topology.newBuilder();
                                    topoBuilder.setNetworkName(topology.getNetworkName());
                                    topoBuilder.setSubnetMask(topology.getSubnetMask());
                                    internals = new HashMap<>();
                                    internals.put(ip, h);
                                    sentDataSize = h.getSerializedSize(); //reset sent data (0 + current Host)
                                }
                            }

                            //externals
                            Map<Integer, Host> externals = new HashMap<>();
                            for (Integer ip : topology.getExternals().keySet()) {
                                Host h = topology.getExternals().get(ip);
                                sentDataSize += h.getSerializedSize();
                                if (sentDataSize < MTU) {
                                    externals.put(ip, h);
                                }
                                else {
                                    topoBuilder.putAllInternals(internals);
                                    topoBuilder.putAllExternals(externals);
                                    topoBuilder.setTimestamp(Timestamp.newBuilder(topology.getTimestamp())
                                            .setSeconds(topology.getTimestamp().getSeconds())
                                            .setNanos(topology.getTimestamp().getNanos())
                                            .build());
                                    Topology topo = topoBuilder.build();
                                    addMessageToScheduler(ProtoUtils.toContainer(c.getDataType(), c.getDataNodeId(),
                                            topo));
                                    //TimeUnit.MILLISECONDS.sleep(THROUGHPUT_INTERVAL);
                                    topoBuilder = Topology.newBuilder();
                                    topoBuilder.setNetworkName(topology.getNetworkName());
                                    topoBuilder.setSubnetMask(topology.getSubnetMask());
                                    internals = new HashMap<>();
                                    externals = new HashMap<>();
                                    externals.put(ip, h);
                                    sentDataSize = h.getSerializedSize(); //reset sent data (0 + current Host)
                                }
                            }

                            //add gateways, usually small
                            topoBuilder.putAllInternals(internals);
                            topoBuilder.putAllExternals(externals);
                            topoBuilder.putAllLocalGws(topology.getLocalGws());
                            topoBuilder.putAllRemoteGws(topology.getRemoteGws());
                            topoBuilder.setTimestamp(Timestamp.newBuilder(topology.getTimestamp())
                                    .setSeconds(topology.getTimestamp().getSeconds())
                                    .setNanos(topology.getTimestamp().getNanos())
                                    .build());
                            Topology finalTopology = topoBuilder.build();
                            addMessageToScheduler(ProtoUtils.toContainer(c.getDataType(), c.getDataNodeId(),
                                    finalTopology));
                            sentDataSize = finalTopology.getSerializedSize();
                            break;
                    }
                }
                else {
                    //does not exceed the maximum allowed throughput size
                    addMessageToScheduler(c);
                    sentDataSize += c.getSerializedSize();
                }
            }
            catch (Exception e) {
                log.error("Error while processing packet", e);
            }
        }
    }

    private void addMessageToScheduler (Container c)
    {
        String senderId = _nodeMon.getWorldState().getLocalNode().getId();
        String recipientId;
        if (_isLocalDelivery || _isClientsDelivery) {
            recipientId = Config.getStringValue(Conf.NodeMon.GROUPS_LOCAL, DefaultValues.NodeMon.GROUPS_LOCAL);
            Container localReady = ProtoUtils.toContainer(c,
                    c.getDataType(),
                    MessageType.UPDATE_DATA,
                    _transportType,
                    senderId,
                    recipientId,
                    c.getDataNodeId());

            //show only if directed to clients
            _scheduler.addOutgoingMessage(localReady);
        }

        if (_isMastersDelivery) {
            recipientId = Config.getStringValue(Conf.NodeMon.GROUPS_MASTERS, DefaultValues.NodeMon
                    .GROUPS_MASTERS);
            Container mastersReady = ProtoUtils.toContainer(c,
                    c.getDataType(),
                    MessageType.UPDATE_DATA,
                    _transportType,
                    senderId,
                    recipientId,
                    c.getDataNodeId());

            _scheduler.addOutgoingMessage(mastersReady);
//            log.debug(_transportType + " -> msg " + mastersReady.getDataType()
//                    + " of size: " + mastersReady.getSerializedSize()
//                    + " rfd: " + mastersReady.getDataNodeId()
//                    + " age: " + TimeUtil.distance(mastersReady.getTimestamp(), TimeUtil.getCurrentTime())
//                    .getSeconds() + " sec"
//                    + " to ->: " + mastersReady.getRecipientId()
//                    + " QS: " + _outputQueue.size() + "/" + QUEUE_SIZE);
        }
    }

    private boolean _isLocalDelivery;
    private boolean _isMastersDelivery;
    private boolean _isClientsDelivery;
    protected Scheduler _scheduler;
    protected NodeMon _nodeMon;
    protected TransportType _transportType;
    protected AtomicBoolean _isRunning;
    private final int MTU;
    private final int QUEUE_SIZE;
    private final BlockingQueue<Container> _outputQueue;
    protected static Logger log = Logger.getLogger(ThroughputController.class);
}
