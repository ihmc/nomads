package us.ihmc.aci.nodemon.client;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.proxy.BaseNodeMonProxy;
import us.ihmc.aci.nodemon.proxy.NodeMonProxy;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;
import us.ihmc.aci.nodemon.util.Utils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;

/**
 * NodeMonClient.java
 * <p/>
 * Class <code>NodeMonClient</code> provides a sample of how to use the NodeMonProxy client API.
 * It implements all the necessary callbacks and prints the data to the default logger as JSON data.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NodeMonClient implements NodeMonProxyListener
{
    public NodeMonClient (String host, int port)
    {
        _host = Objects.requireNonNull(host, "ClientId can't be null");
        _port = port;
        _proxy = new BaseNodeMonProxy((short) 0, "NodeMonClient", host, port);
        _proxy.registerNodeMonProxyListener(this);
    }

    /**
     * Connects the proxy to the provided host and port.
     *
     * @return true if the connection was successful, false otherwise
     */
    public boolean connect ()
    {
        if (_proxy == null) {
            return false;
        }
        return _proxy.connect();
    }

    /**
     * Requests the current world state.
     *
     * @return true if the request was successful, false otherwise
     */
    public boolean requestWorldState ()
    {
        if (_proxy == null) {
            return false;
        }

        return _proxy.getWorldState();
    }

    @Override
    public void worldStateUpdate (Collection<Node> worldState)
    {
        //log.debug("*** start callback worldStateUpdate ***");
        if (worldState == null) {
            log.error("worldState is null, this should never happen");
            return;
        }
        if (worldState.size() == 0) {
            log.error("worldState is empty, this should never happen");
            return;
        }

        for (Node n : worldState) {
            try {
                log.debug(JsonFormat.printer().print(ProtoUtils.toReadable(n)));
            }
            catch (InvalidProtocolBufferException e) {
                log.error("Error while printing the node: " + n.getId(), e);
            }
        }

        //Java 8 ONLY
//        worldState.forEach(node -> {
//            try {
//                JsonFormat.printer().print(ProtoUtils.toReadable(node));
//            }
//            catch (InvalidProtocolBufferException e) {
//                log.error("Error while printing the node: " + node.getId(), e);
//            }
//        });
        //log.debug("*** end callback worldStateUpdate ***");

    }

    @Override
    public void updateData (String nodeId, byte[] data)
    {
        //log.debug("*** start callback updateData ***");
        if (nodeId == null) log.error("nodeId is null, this should never happen");
        if (data == null) log.error("data is null, this should never happen");
        if (nodeId == null || data == null) {
            return;
        }

        Container c;
        try {
            c = ProtoSerializer.deserialize(data);
        }
        catch (InvalidProtocolBufferException e) {
            log.error("Error while deserializing data ", e);
            return;
        }

        DataType dataType = c.getDataType();
        switch (dataType) {
            case INFO:
                Info info = c.getInfo();
                break;
            case GROUP:
                List<Group> groupList = c.getGroupsList();
                break;
            case LINK:
                List<Link> linkList = c.getLinksList();
                for (Link l : linkList) {

                    ReadableLink rl = ProtoUtils.toReadable(l);

//                    try {
//                        log.debug(JsonFormat.printer().print(rl));
//                    }
//                    catch (InvalidProtocolBufferException e) {
//                        e.printStackTrace();
//                    }

                    int rcvdFiveSec = 0;
                    int rcvsMin = 0;
                    for (Stat s : l.getStatsList()) {
                        rcvdFiveSec += s.getReceivedFiveSec();
                        rcvsMin += s.getReceivedMinute();
                    }


                    log.debug(" -> msg " + c.getDataType()
                            + " of size: " + l.getSerializedSize()
                            + " rfd: " + c.getDataNodeId()
                            + " age: " + TimeUtil.distance(l.getTimestamp(), TimeUtil.getCurrentTime())
                            .getSeconds() + " sec "
                            + rl.getIpSrc() + " -> " + rl.getIpDst() + " RCVD 5sec: " + rcvdFiveSec + " RCVS Min: " +
                            rcvsMin);

                }
                break;
            case TOPOLOGY:
                Topology topology = c.getTopology();
                break;
            case NETWORK_HEALTH:
                NetworkHealth networkHealth = c.getNetworkHealth();
                break;
            case NODE:
                Node node = c.getNode();
                break;
            default:
                log.error("Unsupported type: " + dataType);
                throw new UnsupportedOperationException();
        }


        log.debug("*** nodeId: " + nodeId + " dataType: " + dataType + " data size: " + data.length);
//        try {
//            log.debug(JsonFormat.printer().print(c));
//        }
//        catch (InvalidProtocolBufferException e) {
//            log.error("Error while deserializing data ", e);
//        }
        //log.debug("*** end callback updateData ***");
    }

    @Override
    public void connectionClosed ()
    {
        log.debug("*** start callback connectionClosed ***");
        log.debug("*** end callback connectionClosed ***");
    }

    public static void main (String[] args) throws InterruptedException
    {
        NodeMonClient client;

        try {
            String arch = System.getProperty("os.arch");
            boolean isAndroid = arch.contains("arm");
            if (!isAndroid) {
                Utils.initLogger(args[2], args[3]);
            }
            log.info("+++ System architecture is: " + arch + " +++");

            if (!Utils.isValidPort(args[1])) {
                throw new NumberFormatException("Not a valid port");
            }

            client = new NodeMonClient(args[0], Integer.parseInt(args[1]));
        }
        catch (ArrayIndexOutOfBoundsException | NumberFormatException | NullPointerException e) {
            log.error("Input arguments (host, port) not specified correctly", e);
            return;
        }

        if (!client.connect()) {
            log.error("Unable to connect client to " + args[0] + ":" + args[1]);
            return;
        }

        while (true) {
            TimeUnit.MILLISECONDS.sleep(10000);
            client.requestWorldState();
        }
    }

    private final String _host;
    private final int _port;
    private final NodeMonProxy _proxy;
    private static final Logger log = Logger.getLogger(NodeMonClient.class);
}
