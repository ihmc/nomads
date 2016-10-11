package us.ihmc.aci.disServiceProProxy;

import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.disServiceProxy.QueuedDisseminationServiceProxy;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class QueuedDisServiceProProxy extends QueuedDisseminationServiceProxy
                                      implements DisServiceProProxyInterface, DisServiceProProxyListener
{
    public abstract class DSProProxyEvent
    {
        public final String _nodeId;

        protected DSProProxyEvent (String nodeId)
        {
            _nodeId = nodeId;
        }
    }

    public class RegisteredNewPath extends DSProProxyEvent
    {
        public final NodePath _path;
        public final String _team;
        public final String _mission;

        RegisteredNewPath (NodePath path, String nodeId, String team, String mission)
        {
            super (nodeId);

            _path = path;
            _team = team;
            _mission = mission;
        }
    }

    public class UpdatedPosition extends DSProProxyEvent
    {
        public final float _latitude;
        public final float _longitude;
        public final float _altitude;

        UpdatedPosition (float latitude, float longitude, float altitude, String nodeId)
        {
            super (nodeId);

            _latitude = latitude;
            _longitude = longitude;
            _altitude = altitude;
        }
    }

    private final AtomicBoolean _queueArrivedDSProProxyEvents = new AtomicBoolean (true);
    private final Queue<DSProProxyEvent> _arrivedDSProProxyEvents = new ConcurrentLinkedQueue<DSProProxyEvent>();

    public QueuedDisServiceProProxy()
    {
        this (new DisServiceProProxy());
    }

    public QueuedDisServiceProProxy (short applicationId)
    {
        this (new DisServiceProProxy (applicationId));
    }

    public QueuedDisServiceProProxy (short applicationId, long reinitializationAttemptInterval)
    {
        this (new DisServiceProProxy (applicationId, reinitializationAttemptInterval));
    }

    public QueuedDisServiceProProxy (short applicationId, String host, int iPort)
    {
    	this (new DisServiceProProxy (applicationId, host, iPort));
    }

    public QueuedDisServiceProProxy (short applicationId, String host, int port,
                                     long reinitializationAttemptInterval)
    {
        this (new DisServiceProProxy (applicationId, host, port, reinitializationAttemptInterval));
    }

    protected QueuedDisServiceProProxy (DisServiceProProxy proxy)
    {
        super (proxy);
    }

    @Override
    public int init() throws Exception
    {
        int rc = 0;
        try {
            rc = _proxy.init();

            // Register itself
            ((DisServiceProProxy) _proxy).registerDisServiceProProxyListener (this);
            _proxy.registerPeerStatusListener(this);
        }
        catch (CommException ex) {
            Logger.getLogger (QueuedDisServiceProProxy.class.getName()).log(Level.SEVERE, null, ex);
            return -1;
        }

        return rc;
    }

    public void registerDisServiceProProxyListener(DisServiceProProxyListener listener)
        throws CommException
    {
        ((DisServiceProProxy) _proxy).registerDisServiceProProxyListener (listener);
    }

    public void registerMatchmakingLogListener(MatchmakingLogListener listener)
        throws CommException
    {
        ((DisServiceProProxy) _proxy).registerMatchmakingLogListener (listener);
    }

    public int configureMetaDataFields(String xMLMetadataFields)
    {
        return ((DisServiceProProxy) _proxy).configureMetaDataFields(xMLMetadataFields);
    }

    public int configureMetaDataFields(String[] attributes, String[] values)
    {
        return ((DisServiceProProxy) _proxy).configureMetaDataFields(attributes, values);
    }

    public boolean configureProperties(Properties properties)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).configureProperties(properties);
    }

    public boolean setMetadataPossibleValues(String xMLMetadataValues)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).setMetadataPossibleValues(xMLMetadataValues);
    }

    public boolean setMetadataPossibleValues(String[] attributes, String[] values)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).setMetadataPossibleValues(attributes, values);
    }

    public boolean setRankingWeights(float coordRankWeight, float timeRankWeight, float expirationRankWeight, float impRankWeight, float predRankWeight, float targetWeight, boolean strictTarget)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).setRankingWeights(coordRankWeight, timeRankWeight, expirationRankWeight, impRankWeight, predRankWeight, targetWeight, strictTarget);
    }

    public boolean registerPath(NodePath path)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).registerPath (path);
    }

    public boolean setActualPath(String pathID)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).setActualPath (pathID);
    }

    public boolean setNodeContext(String teamID, String missionID, String role)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).setNodeContext(teamID, missionID, role);
    }

    public int changePathProbability(String pathID, float fNewProbability)
    {
        return ((DisServiceProProxy) _proxy).changePathProbability(pathID, fNewProbability);
    }

    public int deregisterPath(String pathID)
    {
        return ((DisServiceProProxy) _proxy).deregisterPath(pathID);
    }

    public NodePath getActualPath() throws CommException
    {
        return ((DisServiceProProxy) _proxy).getActualPath();
    }

    public NodePath getPath()
    {
        return ((DisServiceProProxy) _proxy).getPath();
    }

    public NodePath getPathForPeer(String peerNodeId) throws CommException
    {
        return ((DisServiceProProxy) _proxy).getPathForPeer (peerNodeId);
    }

    public boolean setActualPosition(float fLatitude, float fLongitude, float fAltitude, String location, String note)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).setActualPosition(fLatitude, fLongitude, fAltitude, location, note);
    }

    public String getXMLMetaDataConfiguration()
    {
        return ((DisServiceProProxy) _proxy).getXMLMetaDataConfiguration();
    }

    public void getMetaDataConfiguration(String[] attributes, String[] values)
    {
        ((DisServiceProProxy) _proxy).getMetaDataConfiguration(attributes, values);
    }

    public List<String> getAllCachedMetaDataAsXML()
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).getAllCachedMetaDataAsXML();
    }

    public List<String> getAllCachedMetaDataAsXML(long startTimestamp, long endTimestamp)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).getAllCachedMetaDataAsXML(startTimestamp, endTimestamp);
    }

    public List<String> getMatchingMetaDataAsXML(Map<String, String> attributeValueToMatch)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).getMatchingMetaDataAsXML(attributeValueToMatch);
    }

    public List<String> getMatchingMetaDataAsXML(Map<String, String> attributeValueToMatch, long startTimestamp, long endTimestamp)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).getMatchingMetaDataAsXML(attributeValueToMatch, startTimestamp, endTimestamp);
    }

    public PeerNodeContext getPeerNodeContext(String peerNodeId)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).getPeerNodeContext(peerNodeId);
    }

    public byte[] getMetaDataAsXML(String messageID)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).getMetaDataAsXML(messageID);
    }

    public byte[] getData(String messageID) throws CommException
    {
        return ((DisServiceProProxy) _proxy).getData(messageID);
    }

    public String[] search(String groupName, String query) throws CommException
    {
        return ((DisServiceProProxy) _proxy).search(groupName, query);
    }

    public String pushPro(short iClientId, String groupName, String objectId, String instanceId, String xMLMetadata, byte[] data, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).pushPro(iClientId, groupName, objectId, instanceId, xMLMetadata, data, expirationTime, historyWindow, tag);
    }

    public String makeAvailablePro(short iClientId, String groupName, String objectId, String instanceId, String xMLMetadata, byte[] data, String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).makeAvailablePro(iClientId, groupName, objectId, instanceId, xMLMetadata, data, dataMimeType, expirationTime, historyWindow, tag);
    }

    public String makeAvailablePro(short iClientId, String groupName, String objectId, String instanceId, String[] metaDataAttributes, String[] metaDataValues, byte[] data, String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return ((DisServiceProProxy) _proxy).makeAvailablePro(iClientId, groupName, objectId, instanceId, metaDataAttributes, metaDataValues, data, dataMimeType, expirationTime, historyWindow, tag);
    }

    @Override
    public boolean pathRegistered (NodePath path, String nodeId, String team, String mission)
    {

        if (_queueArrivedDSProProxyEvents.get()) {
            _LOGGER.info(String.format("Added pathRegistered event from nodeId: %s", nodeId));
            _arrivedDSProProxyEvents.add (new RegisteredNewPath (path, nodeId, team, mission));
        }
        return true;
    }

    @Override
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        if (_queueArrivedDSProProxyEvents.get()) {
            _LOGGER.info(String.format("Added positionUpdated event from nodeId: %s", nodeId));
            _arrivedDSProProxyEvents.add (new UpdatedPosition (latitude, longitude, altitude, nodeId));
        }
        return true;
    }

    /**
     * Retrieves and removes the head of the arrived data queue, or returns null
     * if this queue is empty.
     */
    public DSProProxyEvent getDSProProxyEvents()
    {
        return _arrivedDSProProxyEvents.poll();
    }

    public String pushPro(short iClientId, String groupName, String objectId, String instanceId, List<String> metaDataAttributes, List<String> metaDataValues, byte[] data, long expirationTime, short historyWindow, short tag) throws CommException {
        return ((DisServiceProProxy) _proxy).pushPro (iClientId, groupName, objectId, instanceId,
                                                      metaDataAttributes, metaDataValues,
                                                      data, expirationTime, historyWindow, tag);
    }

    public String pushPro(short iClientId, String groupName, String objectId, String instanceId, String[] metaDataAttributes, String[] metaDataValues, byte[] data, long expirationTime, short historyWindow, short tag) throws CommException {
        return ((DisServiceProProxy) _proxy).pushPro (iClientId, groupName, objectId, instanceId,
                                                      metaDataAttributes, metaDataValues,
                                                      data, expirationTime, historyWindow, tag);
    }

    public void informationMatched(String localNodeID, String peerNodeID, String matchedObjectID, String matchedObjectName, String[] rankDescriptors, float[] partialRanks, float[] weights, String comment, String operation)
    {
        ((DisServiceProProxy) _proxy).informationMatched (localNodeID, peerNodeID, matchedObjectID, matchedObjectName,
                                                          rankDescriptors, partialRanks, weights, comment, operation);
    }

    public void informationSkipped(String localNodeID, String peerNodeID, String skippedObjectID, String skippedObjectName, String[] rankDescriptors, float[] partialRanks, float[] weights, String comment, String operation)
    {
        ((DisServiceProProxy) _proxy).informationSkipped (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                                          rankDescriptors, partialRanks, weights, comment, operation);
    }

    private static final Logger _LOGGER = Logger.getLogger (QueuedDisServiceProProxy.class.getName());
}

