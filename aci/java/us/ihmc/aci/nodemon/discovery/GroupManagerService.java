package us.ihmc.aci.nodemon.discovery;

import org.apache.log4j.Logger;
import us.ihmc.aci.grpMgr.GroupManager;
import us.ihmc.aci.grpMgr.GroupManagerListener;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.msg.Messenger;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.aci.nodemon.util.Strings;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;

import java.io.File;
import java.io.IOException;
import java.util.Objects;

/**
 * GroupManagerService.java
 * <p/>
 * Class <code>GroupManagerService</code> handles the discovery of nodes along the network using
 * <code>GroupManager</code>.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class GroupManagerService implements GroupManagerListener, Messenger
{
    GroupManagerService (DiscoveryService ds, NodeMon nodeMon)
    {
        _ds = Objects.requireNonNull(ds, "DiscoveryService can't be null");
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _gm = new GroupManager();
        _timeout = Config.getIntegerValue(Conf.NodeMon.MESSENGER_TIMEOUT,
                DefaultValues.NodeMon.MESSENGER_TIMEOUT);
        _isMaster = Config.getBooleanValue(Conf.NodeMon.GROUPS_IS_MASTER,
                DefaultValues.NodeMon.GROUPS_IS_MASTER);
        _localGroup = Config.getStringValue(Conf.NodeMon.GROUPS_LOCAL,
                DefaultValues.NodeMon.GROUPS_LOCAL);
        _mastersGroup = Config.getStringValue(Conf.NodeMon.GROUPS_MASTERS,
                DefaultValues.NodeMon.GROUPS_MASTERS);
    }

    void init (String logFile, String configFile) throws IOException
    {
        _gm.setLogger(logFile);


        boolean isAutoNetworkConfig = Config.getBooleanValue(Conf.NodeMon.NETWORK_CONFIG_AUTO_ENABLE,
                DefaultValues.NodeMon.NETWORK_CONFIG_AUTO_ENABLE);
        String config = configFile;
        if (isAutoNetworkConfig) {
            config = Utils.getAutoConfigFile(configFile);
        }
        _gm.init(config);
        _gm.setGroupManagerListener(this);

        _gm.createPublicPeerGroup(_localGroup, null);
        _ds.newGroupMember(_localGroup, _nodeMon.getWorldState().getLocalNodeId());
        log.debug("*** Created LOCAL public group: " + _localGroup);

        if (_isMaster) {
            log.debug("*** Node is configured as MASTER, joining MASTERS group");
            _gm.createPublicPeerGroup(_mastersGroup, null);
            _ds.newGroupMember(_mastersGroup, _nodeMon.getWorldState().getLocalNodeId());
        }

        _gm.start();
        log.debug("GroupManager discovery service started");
    }

    public GroupManager getGroupManager ()
    {
        return _gm;
    }

    @Override
    public void broadcastMessage (String groupName, byte[] message)
    {
        _gm.broadcastPeerMessageToGroupAsBytes(groupName, message);
    }

    @Override
    public void sendMessage (String nodeId, byte[] message)
    {
        _gm.sendPeerMessageAsBytes(nodeId, message, true, _timeout);
    }

    @Override
    public void onMessage (String groupName, String nodeId, byte[] message)
    {
        //fake stub
    }

    @Override
    public void newPeer (String nodeUUID)
    {
        log.debug("New peer found with nodeUUID: " + nodeUUID);
        _ds.newNode(nodeUUID);
    }

    @Override
    public void deadPeer (String nodeUUID)
    {

        log.debug("Dead peer found with nodeUUID: " + nodeUUID);
        _ds.deadNode(nodeUUID);
    }

    @Override
    public void groupListChange (String nodeUUID)
    {
        log.debug("Group list has changed: " + nodeUUID);
        _ds.groupListChange(nodeUUID);

    }

    @Override
    public void newGroupMember (String groupName, String memberUUID, byte[] data)
    {
        log.debug("New member: " + memberUUID + " for group: " + groupName);
        _ds.newGroupMember(groupName, memberUUID);
    }

    @Override
    public void groupMemberLeft (String groupName, String memberUUID)
    {
        log.debug("Member: " + memberUUID + " left group: " + groupName);
    }

    @Override
    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {

    }

    @Override
    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] data)
    {

    }

    @Override
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {

    }

    @Override
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {

    }

    @Override
    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {


        log.trace("### Message received. Sender: " + nodeUUID + (groupName != null ? " towards group ->: " +
                groupName : ""));

        if (groupName == null) {
            log.error("Received message with NO group, ERROR, check discovery service");
            return;
        }

        if (groupName.equals(_localGroup) || (_isMaster && groupName.equals(_mastersGroup))) {
            _ds.onMessage(groupName, nodeUUID, data);
        }
        else {
            log.trace("Not a member of group: " + groupName + " skipping message.");
        }
    }

    @Override
    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String peerSearchUUID)
    {

    }

    @Override
    public void peerNodeDataChanged (String nodeUUID)
    {

    }

    @Override
    public void peerUnhealthy (String nodeUUID)
    {
        log.debug("Found unhealthy peer: " + nodeUUID);

    }

    @Override
    public void hopCountChanged (int hopCount)
    {
        log.debug("HopCount changed: " + hopCount);
    }

    private final DiscoveryService _ds;
    private final GroupManager _gm;
    private final int _timeout;
    private final boolean _isMaster;
    private String _localGroup = null;
    private String _mastersGroup = null;
    private final NodeMon _nodeMon;

    private static final Logger log = Logger.getLogger(GroupManagerService.class);
}
