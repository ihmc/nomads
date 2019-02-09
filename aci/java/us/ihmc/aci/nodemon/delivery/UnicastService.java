package us.ihmc.aci.nodemon.delivery;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.Controller;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.msg.Messenger;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.net.*;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * UnicastService.java
 * <p/>
 * Class <code>UnicastService</code> takes care of delivering and receiving data from a <code>DatagramSocket</code>
 * to deliver to masters in case multicast isn't available.
 */
public class UnicastService implements Runnable, Controller, Messenger
{

    public UnicastService (NodeMon nodeMon, int port) throws SocketException
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        if (port < 0 || port > 65535) {
            throw new IllegalArgumentException("Port has to be between 0 and 65535");
        }

        _serverDatagramSocket = new DatagramSocket(port);
        _clientDatagramSocket = new DatagramSocket();
        _isRunning = new AtomicBoolean(false);
    }

    @Override
    public boolean isRunning ()
    {
        return _isRunning.get();
    }

    @Override
    public void start ()
    {
        (new Thread(this, "UnicastServiceThread")).start();

    }

    @Override
    public void stop ()
    {
        _isRunning.set(false);
    }

    @Override
    public void run ()
    {

        log.debug(UnicastService.class.getSimpleName() + "Thread started");
        _isRunning.set(true);
        DatagramPacket dgPacket = new java.net.DatagramPacket(new byte[65535], 65535);


        while (_isRunning.get()) {
            try {
                _serverDatagramSocket.receive(dgPacket);
            }
            catch (IOException e) {
                e.printStackTrace();
                return;
            }
            //System.out.println ("Received a new packet of size " + dgPacket.getLength());
            ByteArrayInputStream bais = new ByteArrayInputStream(dgPacket.getData());
            DataInputStream dis = new DataInputStream(bais);
            log.debug("{*NM-Uni*} Received datagram of size: " + dgPacket.getLength());
            try {
                processMessage(dis, dgPacket.getLength());
            }
            catch (IOException e) {
                e.printStackTrace();
                // This is a parsing / packet format error - so continue
                continue;
            }
        }
    }

    private void processMessage (DataInputStream dis, int dataLength)
            throws java.io.IOException
    {
        byte buf[] = new byte[dataLength];
        dis.readFully(buf);
        onMessage(null, null, buf);
    }

    @Override
    public void broadcastMessage (String groupName, byte[] message)
    {

        Node localNode = _nodeMon.getWorldState().getLocalNodeCopy();

        Group masters = localNode.getGrump().getGroups().get(Config.getStringValue(Conf.NodeMon.GROUPS_MASTERS));

        for (String master : masters.getMembers().keySet()) {

            String ip = masters.getMembers().get(master);

            if (!Utils.isValidIPv4Address(ip)) {
                log.error("Specified IP: " + ip + " in Grump config not a valid IP");
                return;
            }

            log.debug("Sending UDP datagram to node: " + master + " with IP: " + ip);
            sendDatagram(ip, message);
        }
    }

    @Override
    public void sendMessage (String nodeId, byte[] message)
    {

        if (!Utils.isValidIPv4Address(nodeId)) {
            log.warn(nodeId + " not a valid IP address, trying to determine node IP...");

            Node n = _nodeMon.getWorldState().getNode(nodeId);
            if (n == null) {
                log.error(nodeId + " not found in WorldState, unable to send message!");
                return;
            }

            String ip = Utils.getPrimaryIP(n);
            if (ip == null) {
                log.error("Unable to determine IP of node: " + nodeId + " unable to send message");
                return;
            }

            //ip was determined, send datagram
            log.debug("Sending datagram to IP: " + ip);
            sendDatagram(ip, message);
            return;
        }

        log.debug("Sending datagram to: " + nodeId);
        //id has to be an ip or hostname
        sendDatagram(nodeId, message);
    }

    private void sendDatagram (String ip, byte[] message)
    {

        DatagramPacket out = null;
        try {
            out = new DatagramPacket(message, message.length, InetAddress.getByName(ip), 8501);
            _clientDatagramSocket.send(out);
        }
        catch (UnknownHostException e) {
            log.error("Unable to determine InetAddress of host: " + ip, e);
        }
        catch (IOException e) {
            log.error("Unable to send packet to IP: " + ip, e);
        }
    }

    @Override
    public void onMessage (String groupName, String nodeId, byte[] message)
    {
        //TODO unify this method with multicast version inside GroupManagerService
        Container c;
        try {
            c = ProtoSerializer.deserialize(message);
        }
        catch (InvalidProtocolBufferException | ClassCastException e) {
            log.debug("Unable to deserialize Container message", e);
            return;
        }

        DataType dt = c.getDataType();
        if (dt == null) {
            log.error("Received Container message with null DataType, ignoring it");
            return;
        }
        //TransportType transportType = (groupName == null ? TransportType.UNICAST : TransportType.MULTICAST);

        Container repack = Container.newBuilder(c)
                .setMessageType(MessageType.UPDATE_DATA)
                .setRecipientId(_nodeMon.getWorldState().getLocalNodeId())
                .build();

        log.debug(repack.getTransportType() + " <- ### Recvd msg " + c.getDataType()
                + " of size: " + c.getSerializedSize()
                + " rfd: " + c.getDataNodeId()
                + " age: " + TimeUtil.distance(c.getTimestamp(), TimeUtil.getCurrentTime())
                .getSeconds() + " sec");
        ProtoUtils.printDetails(c);

        log.trace("+++ Adding message to to NodeMon scheduler (incoming): " + repack);
        _nodeMon.getScheduler().addIncomingMessage(repack);
        log.trace("*** Adding message to to NodeMonProxy scheduler (outgoing): " + repack);
        _nodeMon.getProxyScheduler().addOutgoingMessage(repack);
    }

    protected AtomicBoolean _isRunning;
    private DatagramSocket _serverDatagramSocket;
    private DatagramSocket _clientDatagramSocket;
    private final NodeMon _nodeMon;
    private static final Logger log = Logger.getLogger(UnicastService.class);
}
