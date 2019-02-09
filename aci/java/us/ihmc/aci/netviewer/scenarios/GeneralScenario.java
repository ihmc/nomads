package us.ihmc.aci.netviewer.scenarios;

import com.google.gson.Gson;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.netviewer.scenarios.json.EventsQueueWrapper;
import us.ihmc.aci.netviewer.scenarios.json.NVType;
import us.ihmc.aci.netviewer.scenarios.json.Remote;
import us.ihmc.aci.netviewer.scenarios.json.WorldStateWrapper;
import us.ihmc.aci.netviewer.util.Event;
import us.ihmc.aci.netviewer.util.EventType;
import us.ihmc.aci.nodemon.BaseWorldState;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.WorldState;
import us.ihmc.aci.nodemon.custom.DisServiceContent;
import us.ihmc.aci.nodemon.custom.MocketsContent;
import us.ihmc.aci.nodemon.custom.ProcessContentType;
import us.ihmc.aci.nodemon.data.*;
import us.ihmc.aci.nodemon.data.Process;
import us.ihmc.aci.nodemon.data.info.BaseHardwareAbstractionLayer;
import us.ihmc.aci.nodemon.data.info.BaseNodeInfo;
import us.ihmc.aci.nodemon.data.traffic.BaseTraffic;
import us.ihmc.aci.nodemon.discovery.DiscoveryService;
import us.ihmc.aci.nodemon.proxy.BaseNodeMonProxyServer;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;
import us.ihmc.aci.nodemon.scheduler.NodeMonProxyScheduler;
import us.ihmc.aci.nodemon.scheduler.ProxyScheduler;
import us.ihmc.aci.nodemon.scheduler.Scheduler;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.serialization.SerializationException;
import us.ihmc.util.serialization.SerializerFactory;
import us.ihmc.util.serialization.SerializerType;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;

/**
 * Abstract class that implements the basic methods for a generic scenario
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public abstract class GeneralScenario implements NodeMon
{
    /**
     * Constructor
     * @param worldStateFile name of the file containing the world state configuration
     * @param eventsFile name of the file containing the update events configuration
     * @throws IOException if any problem in the IO occurs
     * @throws SerializationException if serialization problems occur
     */
    protected GeneralScenario (String worldStateFile, String eventsFile) throws IOException, SerializationException
    {
        _dataSerializer = new DataSerializer();

        Gson gson = new Gson();

        // Initializing the World State data structure
        BufferedReader br = new BufferedReader (new FileReader (worldStateFile));
        WorldStateWrapper ws = gson.fromJson (br, WorldStateWrapper.class);

        _worldState = new BaseWorldState (ws.getLocal().getId(), ws.getLocal().getName());

        List<Neighbor> neighbors = new ArrayList<>();
        for (String id : ws.getLocal().getNeighbors()) {
            neighbors.add (new BaseNeighbor (id));
        }
        _worldState.getNode (ws.getLocal().getId()).getNeighbors().addAll (neighbors);
        _worldState.getNode (ws.getLocal().getId()).setNodeInfo (new BaseNodeInfo (Utils.createDefaultOS(),
                new BaseHardwareAbstractionLayer (Utils.createDefaultCPUInfoArray (1), Utils.createDefaultNetInfoArray (
                        ws.getLocal().getIps()))));

        for (Remote node : ws.getRemote()) {
            _worldState.put (node.getId(), Utils.createNode (node.getId(), node.getName(), node.getIps(),
                    node.getNeighbors()));
        }


        // Initializing the Scenario Events Queue
        br = new BufferedReader (new FileReader (eventsFile));
        EventsQueueWrapper[] eqArray = gson.fromJson (br, EventsQueueWrapper[].class);

        _scenarioEventsQueue = new LinkedBlockingDeque<>();
        for (EventsQueueWrapper eq : eqArray) {
            switch (EventType.valueOf (eq.getType())) {
                case newNode:
                    _scenarioEventsQueue.add (new ScenarioEvent (new Event (Utils.createNode (eq.getId(), eq.getName(),
                            eq.getIps(), eq.getNeighbors())), eq.getSleep()));
                    break;
                case deadNode:
                    _scenarioEventsQueue.add (new ScenarioEvent (new Event (eq.getId()), eq.getSleep()));
                    break;
                case updatedNode:
                    _scenarioEventsQueue.add (generateScenarioUpdateEvent (eq));
                    break;
            }
        }


        // Initializing the proxy components
        BaseNodeMonProxyServer proxyServer = BaseNodeMonProxyServer.getInstance (this);
        _proxyScheduler = new NodeMonProxyScheduler (this, proxyServer, SerializerFactory.getSerializer (
                SerializerType.KRYO));
        _proxyScheduler.start();

        proxyServer.init();
        proxyServer.start();

        _player = new ScenarioPlayer (_proxyScheduler);

        acceptCommandsConnection();
    }

    /**
     * Creates the scenario update event from the event queue wrapper object
     * @param eq event queue wrapper object read from the file
     * @return the scenario update event
     * @throws SerializationException if serialization problems occur
     */
    private ScenarioEvent generateScenarioUpdateEvent (EventsQueueWrapper eq) throws SerializationException
    {
        switch (NodeMonDataType.valueOf (eq.getNmDataType())) {
            case Process:
                return generateProcessUpdateEvent (eq);

            case CustomProcess:
                return generateCustomProcessUpdateEvent (eq);
        }

        return null;
    }

    /**
     * Creates the process update event from the event queue wrapper object
     * @param eq event queue wrapper object read from the file
     * @return the process update event
     * @throws SerializationException if serialization problems occur
     */
    private ScenarioEvent generateTrafficUpdateEvent (EventsQueueWrapper eq) throws SerializationException
    {
        switch (NVType.valueOf (eq.getNvType())) {
            case aggrIn:
                process = new BaseProcess.Builder()
                        .byteReceived (eq.getValue())
                        .build();
                break;

            case aggrOut:
                process = new BaseProcess.Builder()
                        .byteSent (eq.getValue())
                        .build();
                break;

            case inFrom:
                process = new BaseProcess.Builder()
                        .byteReceived (eq.getValue())
                        .remoteAddress (eq.getRemoteAddr())
                        .build();
                break;

            case outFrom:
                process = new BaseProcess.Builder()
                        .byteSent(eq.getValue())
                        .remoteAddress (eq.getRemoteAddr())
                        .build();
                break;
        }

        NodeStats nodeStats = new BaseNodeStats();
        nodeStats.put (eq.getNvType() + "_" + eq.getId(), process);
        byte[] data = _dataSerializer.serialize (nodeStats);

        return new ScenarioEvent (new Event (eq.getId(), NodeMonDataType.valueOf (eq.getNmDataType()), data),
                eq.getSleep());
    }

    /**
     * Creates the custom process update event from the event queue wrapper object
     * @param eq event queue wrapper object read from the file
     * @return the custom process update event
     * @throws SerializationException if serialization problems occur
     */
    private ScenarioEvent generateProcessUpdateEvent (EventsQueueWrapper eq) throws SerializationException
    {
        Process process = null;
        String processId = eq.getNvType() + "_" + eq.getId();
        switch (NVType.valueOf (eq.getNvType())) {
            case disservice:
                DisServiceContent dContent = new DisServiceContent();
                dContent.setDataMessagesReceived (eq.getDataMessagesReceived());
                dContent.setDataBytesReceived (eq.getDataBytesReceived());
                dContent.setDataFragmentsReceived (eq.getDataFragmentsReceived());
                dContent.setDataFragmentBytesReceived (eq.getDataFragmentBytesReceived());
                dContent.setMissingFragmentRequestMessagesSent (eq.getMissingFragmentRequestMessagesSent());
                dContent.setMissingFragmentRequestBytesSent (eq.getMissingFragmentRequestBytesSent());
                dContent.setMissingFragmentRequestMessagesReceived (eq.getMissingFragmentRequestMessagesReceived());
                dContent.setMissingFragmentRequestBytesReceived (eq.getMissingFragmentRequestBytesReceived());
                dContent.setDataCacheQueryMessagesSent(eq.getDataCacheQueryMessagesSent());
                dContent.setDataCacheQueryBytesSent (eq.getDataCacheQueryBytesSent());
                dContent.setDataCacheQueryMessagesReceived (eq.getDataCacheQueryMessagesReceived());
                dContent.setDataCacheQueryBytesReceived (eq.getDataCacheQueryBytesReceived());
                dContent.setTopologyStateMessagesSent (eq.getTopologyStateMessagesSent());
                dContent.setTopologyStateBytesSent (eq.getTopologyStateBytesSent());
                dContent.setTopologyStateMessagesReceived (eq.getTopologyStateMessagesReceived());
                dContent.setTopologyStateBytesReceived (eq.getTopologyStateBytesReceived());
                dContent.setKeepAliveMessagesSent (eq.getKeepAliveMessagesSent());
                dContent.setKeepAliveMessagesReceived (eq.getKeepAliveMessagesReceived());
                dContent.setQueryMessagesSent (eq.getQueryMessagesSent());
                dContent.setQueryMessagesReceived (eq.getQueryMessagesReceived());
                dContent.setQueryHitsMessagesSent (eq.getQueryHitsMessagesSent());
                dContent.setQueryMessagesReceived (eq.getQueryMessagesReceived());

                process = new BaseProcess (processId, ProcessContentType.DisService, dContent);
                break;

            case mockets:
                MocketsContent mContent = new MocketsContent();
                mContent.setLocalAddr (eq.getLocalAddr());
                mContent.setRemoteAddr (eq.getRemoteAddr());
                mContent.setLastContactTime (eq.getLastContactTime());
                mContent.setSentBytes (eq.getSentBytes());
                mContent.setSentPackets (eq.getSentPackets());
                mContent.setRetransmits (eq.getRetransmits());
                mContent.setReceivedBytes (eq.getReceivedBytes());
                mContent.setReceivedPackets (eq.getReceivedPackets());
                mContent.setDuplicatedDiscardedPackets (eq.getDuplicatedDiscardedPackets());
                mContent.setNoRoomDiscardedPackets (eq.getNoRoomDiscardedPackets());
                mContent.setReassemblySkippedDiscardedPackets (eq.getReassemblySkippedDiscardedPackets());
                mContent.setEstimatedRTT (eq.getEstimatedRTT());
                mContent.setUnacknowledgedDataSize (eq.getUnacknowledgedDataSize());
                mContent.setUnacknowledgedQueueSize (eq.getUnacknowledgedQueueSize());
                mContent.setPendingDataSize (eq.getPendingDataSize());
                mContent.setPendingPacketQueueSize (eq.getPendingPacketQueueSize());
                mContent.setReliableSequencedDataSize (eq.getReliableSequencedDataSize());
                mContent.setReliableSequencedPacketQueueSize (eq.getReliableSequencedPacketQueueSize());
                mContent.setReliableUnsequencedDataSize (eq.getReliableUnsequencedDataSize());
                mContent.setReliableUnsequencedPacketQueueSize (eq.getReliableUnsequencedPacketQueueSize());

                process = new BaseProcess (processId, ProcessContentType.Mockets, mContent);
                break;
        }

        NodeStats nodeStats = new BaseNodeStats();
        nodeStats.addProcess (processId, process);
        byte[] data = _dataSerializer.serialize (nodeStats);

        return new ScenarioEvent (new Event (eq.getId(), NodeMonDataType.valueOf (eq.getNmDataType()), data),
                eq.getSleep());
    }

    /**
     * Accepts a tcp connection with the remote netviewer
     */
    protected void acceptCommandsConnection()
    {
        (new Thread (new Runnable()
        {
            @Override
            public void run()
            {
                try {
                    ServerSocket serverSocket = new ServerSocket (COMMANDS_PORT);
                    Socket s = serverSocket.accept();
                    startCommandsReceiver (s);
                }
                catch (IOException e) {
                    log.error ("Problem in accepting the socket connection", e);
                }
            }
        }, "AcceptCommandsThread")).start();
    }

    /**
     * Handles command messages coming from the netviewer
     * @param s socket connection opened with the netviewer
     */
    protected void startCommandsReceiver (final Socket s)
    {
        (new Thread (new Runnable()
        {
            @Override
            public void run()
            {
                CommHelper ch = new CommHelper (s);
                while (true) {
                    try {
                        Command command = Command.valueOf (ch.receiveLine());
                        switch (command) {
                            case play:
                                if (!_player.hasStarted()) {
                                    _player.start (_scenarioEventsQueue);
                                }
                                else {
                                    _player.play();
                                }
                                break;
                            case pause:
                                _player.pause();
                                break;
                            case stop:
                                _player.stop();
                                log.info ("Closing the scenario player");
                                System.exit (0);
                        }
                    }
                    catch (CommException e) {
                        log.error ("Problem in receiving command", e);
                        return;
                    }
                    catch (IllegalArgumentException e) {
                        log.error ("Problem in reading the command", e);
                    }
                }
            }
        }, "CommandsReceiverThread")).start();
    }

    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing ProxyScheduler                                                                      -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public void registerNodeMonProxyListener (NodeMonProxyListener listener)
    {
    }

    @Override
    public void unregisterNodeMonProxyListener (NodeMonProxyListener listener)
    {
    }

    @Override
    public void init() throws IOException, SerializationException
    {
    }

    @Override
    public void addNodeSensor (PolledNodeSensor nodeSensor)
    {
    }

    @Override
    public void updateData (String nodeId, NodeMonDataType type, Data data)
    {
    }

    @Override
    public Scheduler getScheduler()
    {
        return null;
    }

    @Override
    public ProxyScheduler getProxyScheduler()
    {
        return _proxyScheduler;
    }

    @Override
    public WorldState getWorldState()
    {
        return _worldState;
    }

    @Override
    public DiscoveryService getDiscoveryService()
    {
        return null;
    }

    // <-------------------------------------------------------------------------------------------------------------->


    private final DataSerializer _dataSerializer;
    protected final WorldState _worldState;
    protected final ProxyScheduler _proxyScheduler;
    protected final ScenarioPlayer _player;
    protected final BlockingQueue<ScenarioEvent> _scenarioEventsQueue;
    public final static int COMMANDS_PORT = 12321;

    private static final Logger log = LoggerFactory.getLogger (GeneralScenario.class);
}
