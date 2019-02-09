package us.ihmc.aci.nodemon.discovery;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.grpMgr.PeerInfo;
import us.ihmc.aci.grpMgr.Util;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.msg.Messenger;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

/**
 * DiscoveryService.java
 * <p/>
 * Class <code>DiscoveryService</code> handles the discovery of nodes along the network.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DiscoveryService implements Discoverer, Messenger
{
    public DiscoveryService (NodeMon nodeMon, DiscoveryType dt) throws IOException
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _dt = Objects.requireNonNull(dt, "DiscoveryType can't be null");
    }

    public void init () throws IOException
    {
        log.debug("Initializing DiscoveryService..");
        switch (_dt) {
            case GROUP_MANAGER:
                _gms = new GroupManagerService(this, _nodeMon);
                try {

                    _gms.init(Config.getStringValue(Conf.NodeMon.LOG_DIR) + File.separator + "GroupManager.log",
                            Utils.getAutoConfigFile(Config.getStringValue(Conf.NodeMon.CONF_FILE)));
                    log.debug("DiscoveryService initialized successfully");
                }
                catch (IOException e) {
                    log.error("Error while initializing the GroupManager discovery service");
                    throw e;
                }
                break;
        }
    }

    public void printLocalPeerInfo ()
    {
        log.debug("*** TESTING NODE INFO ***");
        PeerInfo pi = _gms.getGroupManager().getPeerInfo("juventus");
        String[] groups = pi.getGroups();

        if (groups != null) {
            log.debug("*** Printing groups of local node ***");
            for (String group : groups) {
                log.debug(group);
            }
            log.debug("***");
        }
        else {
            log.warn("Groups is null");
        }
        log.debug("NODE group data state seq no: " + pi.getGrpDataStateSeqNo());
        log.debug("NODE name: " + pi.getNodeName());
        log.debug("NODE UUID: " + pi.getNodeUUID());
        InetAddress inetAddress = pi.getAddress();
        log.debug("NODE InetAddress: " + inetAddress);
        log.debug("NODE port: " + pi.getPort());
        log.debug("NODE state seq no: : " + pi.getStateSeqNo());
        log.debug("NODE Public Key: " + pi.getPubKey());
        log.debug("NODE ping count: " + pi.getPingCount());
        log.debug("NODE ping count reset time: " + pi.getPingCountResetTime());
        log.debug("NODE ping interval: " + pi.getPingInterval());
        log.debug("NODE last contact time: " + pi.getLastContactTime());
        log.debug("***");
    }

    @Override
    public void broadcastMessage (String groupName, byte[] message)
    {
        log.trace("Broadcasting message to group: " + groupName);
        switch (_dt) {
            case GROUP_MANAGER:
                _gms.broadcastMessage(groupName, message);
                break;
            default:
                log.error("Protocol not supported for broadcasting a message");
        }
    }

    @Override
    public void sendMessage (String nodeId, byte[] message)
    {
        log.trace("Sending message to node: " + nodeId);
        switch (_dt) {
            case GROUP_MANAGER:
                _gms.sendMessage(nodeId, message);
                break;
            default:
                log.error("Protocol not supported for sending a message");
        }

    }

    @Override
    public void onMessage (String groupName, String nodeId, byte[] message)
    {
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

        Container repack = Container.newBuilder(c)
                .setMessageType(MessageType.UPDATE_DATA)
                .setRecipientId(_nodeMon.getWorldState().getLocalNodeId())
                .setSenderId(nodeId)
                .build();

        //log.debug("### Received msg: " + repack.toString());
        log.debug(repack.getTransportType() + " <- ### Recvd msg " + c.getDataType()
                + " of size: " + c.getSerializedSize()
                + " rfd: " + c.getDataNodeId()
                + " age: " + TimeUtil.distance(c.getTimestamp(), TimeUtil.getCurrentTime())
                .getSeconds() + " sec");
        ProtoUtils.printDetails(c);


        log.trace("+++ Adding message to to NodeMon scheduler (incoming): " + repack);
        _nodeMon.getScheduler().addIncomingMessage(repack);

        //TODO make this configurable like PROXY_DIRECT_LINK_ENABLED
        log.trace("*** Adding message to to NodeMonProxy scheduler (outgoing): " + repack);
        _nodeMon.getProxyScheduler().addOutgoingMessage(repack);
    }

    @Override
    public void newNode (String nodeId)
    {

    }

    @Override
    public void deadNode (String nodeId)
    {

    }

    @Override
    public void groupListChange (String nodeUUID)
    {

    }

    @Override
    public void newGroupMember (String groupName, String memberUUID)
    {
        Map<String, String> members = new HashMap<>();
        members.put(memberUUID, "");
        Group group = Group.newBuilder()
                .setName(groupName)
                .putAllMembers(members)
                .build();

        Container c = ProtoUtils.toContainer(DataType.GROUP, _nodeMon.getWorldState().getLocalNodeId(), group);
        _nodeMon.updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
    }

    public GroupManagerService getGroupManagerService ()
    {
        return _gms;
    }

    private GroupManagerService _gms;

    private final DiscoveryType _dt;
    private final NodeMon _nodeMon;

    private static final Logger log = Logger.getLogger(GroupManagerService.class);
}
