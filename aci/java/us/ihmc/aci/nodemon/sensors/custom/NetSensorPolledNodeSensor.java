package us.ihmc.aci.nodemon.sensors.custom;

import com.google.protobuf.Timestamp;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.Controller;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.util.Config;

import java.io.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.util.*;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * NetSensorPolledNodeSensor.java
 * <p/>
 * Class <code>NetSensorPolledNodeSensor</code> incorporates a <code>UDPDatagramSocket</code> that receives packets
 * from a NetSensor component running in localhost.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NetSensorPolledNodeSensor implements Runnable, PolledNodeSensor
{
    public NetSensorPolledNodeSensor (NodeMon nodeMon, int port) throws SocketException
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        if (port < 0 || port > 65535) {
            throw new IllegalArgumentException("Port has to be between 0 and 65535");
        }

        _datagramSocket = new DatagramSocket(port);
        _unpackController = new UnpackController();
        _unpackController.start();
        (new Thread(this, "NetSensorPolledNodeSensorThread")).start();
    }

    private class UnpackController implements Runnable, Controller
    {
        UnpackController ()
        {
            _isRunning = new AtomicBoolean(false);
            int maxQueueSize = Config.getIntegerValue(Conf.NodeMon.SENSORS_NETSENSOR_QUEUE_SIZE,
                    DefaultValues.NodeMon.SENSORS_NETSENSOR_QUEUE_SIZE);
            _updates = new ArrayBlockingQueue<>(maxQueueSize);
            _topologyMap = new HashMap<>();
            _linkList = new ArrayList<>();
        }

        public void addUpdate (Container update)
        {
            _updates.add(update);
        }


        public List<Link> getLinks ()
        {
            return _linkList;
        }

        public Map<String, Topology> getTopology ()
        {
            if (_topologyMap == null) {
                return null;
            }

            return new HashMap<>(_topologyMap);
        }

        @Override
        public boolean isRunning ()
        {
            return _isRunning.get();
        }

        @Override
        public void start ()
        {
            (new Thread(this, "UnpackController")).start();
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
            while (_isRunning.get()) {
                Container c;
                try {

                    TimeUnit.MILLISECONDS.sleep(50);
                    c = _updates.poll(Long.MAX_VALUE, TimeUnit.DAYS);

                    DataType dt = c.getDataType();
                    if (dt == null) {
                        log.error("Received update with null DataType, ignoring");
                        continue;
                    }

                    log.debug("{*NS* QS: " + _updates.size() + "} Received packet " + dt + " of size: " + c
                            .getSerializedSize());

                    switch (dt) {
                        case LINK:
                            if (c.getLinksList() == null) {
                                throw new IllegalArgumentException("Links is null in container message");
                            }

                            synchronized (linkLock) {
                                for (Link l : c.getLinksList()) {
                                    //set Timestamp, taken from NetSensor's Container
                                    Link newLink = Link.newBuilder(l).setTimestamp(c.getTimestamp()).build();
                                    _linkList.add(newLink);
                                }
                                log.debug("{*NS* QS: " + _updates.size() + "} links to be processed: " + _linkList
                                        .size());
                            }

                            break;
                        case TOPOLOGY_PARTS:
                            TopologyParts topologyParts = c.getTopologyParts();
                            if (topologyParts == null) {
                                throw new IllegalArgumentException("TopologyParts is null in container message");
                            }

                            Map<Integer, Host> internals = new HashMap<>();
                            for (Host h : topologyParts.getInternalsList()) {
                                internals.put(h.getIp(), h);
                            }
                            Map<Integer, Host> localGws = new HashMap<>();
                            for (Host h : topologyParts.getLocalGwsList()) {
                                localGws.put(h.getIp(), h);
                            }
                            Map<Integer, Host> remoteGws = new HashMap<>();
                            for (Host h : topologyParts.getRemoteGwsList()) {
                                remoteGws.put(h.getIp(), h);
                            }
                            Map<Integer, Host> externals = new HashMap<>();
                            for (Host h : topologyParts.getExternalsList()) {
                                externals.put(h.getIp(), h);
                            }

                            Topology oldTopology = _topologyMap.get(topologyParts.getNetworkName());
                            Topology.Builder topoBuilder;
                            if (oldTopology == null) {
                                topoBuilder = Topology.newBuilder();
                            }
                            else {
                                topoBuilder = Topology.newBuilder(oldTopology);
                            }

                            Topology newTopology = topoBuilder
                                    .setNetworkName(topologyParts.getNetworkName())
                                    .setSubnetMask(topologyParts.getSubnetMask())
                                    .putAllInternals(internals)
                                    .putAllLocalGws(localGws)
                                    .putAllRemoteGws(remoteGws)
                                    .putAllExternals(externals)
                                    .setTimestamp(Timestamp.newBuilder(c.getTimestamp())
                                            .setSeconds(c.getTimestamp().getSeconds())
                                            .setNanos(c.getTimestamp().getNanos())
                                            .build())
                                    .build();

                            _topologyMap.put(newTopology.getNetworkName(), newTopology);
                            break;
                        default:
                            throw new IllegalArgumentException("Unsupported DataType: " + dt);
                    }


                }
                catch (Exception e) {
                    log.error("Error while unpacking: ", e);
                }
            }
        }


        private Map<String, Topology> _topologyMap;
        private final List<Link> _linkList;
        private final BlockingQueue<Container> _updates;
        private final AtomicBoolean _isRunning;
        private final Logger log = Logger.getLogger(UnpackController.class);
    }

    @Override
    public void update (DataType type)
    {
        if (!(type.equals(DataType.LINK) ||
                type.equals(DataType.TOPOLOGY))) {
            return;
            //throw new IllegalArgumentException("Sensor doesn't support type " + type);
        }

        switch (type) {
            case LINK:
                if (_unpackController.getLinks() == null) {
                    return;
                }

                synchronized (linkLock) {
                    for (Link link : _unpackController.getLinks()) {

                        if (link == null) {
                            continue;
                        }

                        if (link.getStatsList() == null) {
                            continue;
                        }

                        if (link.getStatsList().size() == 0) {
                            continue;
                        }

                        Container c = ProtoUtils.toContainer(
                                DataType.LINK,
                                _nodeMon.getWorldState().getLocalNodeId(),
                                link);
                        _nodeMon.updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
                    }
                    _unpackController.getLinks().clear();
                }


                break;
            case TOPOLOGY:
                if (_unpackController.getTopology() == null) {
                    return;
                }

                Map<String, Topology> currentTopology = _unpackController.getTopology();
                for (Topology topology : currentTopology.values()) {
                    Container c = ProtoUtils.toContainer(
                            DataType.TOPOLOGY,
                            _nodeMon.getWorldState().getLocalNodeId(),
                            topology);

                    _nodeMon.updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
                }
                break;

            default:
                log.warn("Received update request for not supported type: " + type);
        }
    }

    public void run ()
    {
        DatagramPacket dgPacket = new java.net.DatagramPacket(new byte[65535], 65535);

        while (!_terminate) {
            try {
                _datagramSocket.receive(dgPacket);
            }
            catch (IOException e) {
                e.printStackTrace();
                return;
            }
            //System.out.println ("Received a new packet of size " + dgPacket.getLength());
            ByteArrayInputStream bais = new ByteArrayInputStream(dgPacket.getData());
            DataInputStream dis = new DataInputStream(bais);
            log.trace("{*NS*} Received datagram of size: " + dgPacket.getLength());
            try {
                processMessage(dis, dgPacket.getLength());
            }
            catch (Exception e) {
                log.error("Unable to read NetSensor message, format error ", e);
                continue;
            }
        }
    }

    private void processMessage (DataInputStream dis, int dataLength)
            throws java.io.IOException
    {
        byte buf[] = new byte[dataLength];
        dis.readFully(buf);
        Container c = ProtoSerializer.deserialize(buf);
        _unpackController.addUpdate(c);
    }


    private final Object linkLock = new Object();
    private DatagramSocket _datagramSocket;

    private final NodeMon _nodeMon;
    private boolean _terminate = false;
    private UnpackController _unpackController;
    private static final Logger log = Logger.getLogger(NetSensorPolledNodeSensor.class);
}
