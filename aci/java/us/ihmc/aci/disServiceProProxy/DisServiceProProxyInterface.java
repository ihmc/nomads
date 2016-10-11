package us.ihmc.aci.disServiceProProxy;

import java.util.List;
import java.util.Map;
import java.util.Properties;
import us.ihmc.aci.disServiceProxy.DisseminationServiceInterface;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface DisServiceProProxyInterface extends DisseminationServiceInterface, MatchmakingLogListener
{
    public void registerDisServiceProProxyListener (DisServiceProProxyListener listener)
        throws CommException;

    public void registerMatchmakingLogListener (MatchmakingLogListener listener)
        throws CommException;

    public int configureMetaDataFields (String xMLMetadataFields);

    public int configureMetaDataFields (String[] attributes, String[] values);

    public boolean configureProperties (Properties properties)
        throws CommException;

    public boolean setMetadataPossibleValues (String xMLMetadataValues)
        throws CommException;

    public boolean setMetadataPossibleValues (String[] attributes, String[] values)
        throws CommException;

    public boolean setRankingWeights (float coordRankWeight, float timeRankWeight,
                                      float expirationRankWeight, float impRankWeight,
                                      float predRankWeight, float targetWeight,
                                      boolean strictTarget)
        throws CommException;

    public boolean registerPath (NodePath path)
        throws CommException;

    public boolean setActualPath (String pathID)
        throws CommException;

    public boolean setNodeContext (String teamID, String missionID, String role)
        throws CommException;

    public int changePathProbability (String pathID, float fNewProbability);

    public int deregisterPath (String pathID);

    public NodePath getActualPath() throws CommException;

    public NodePath getPath();
    public NodePath getPathForPeer (String peerNodeId) throws CommException;

    public boolean setActualPosition (float fLatitude, float fLongitude, float fAltitude,
                                      String location, String note)
        throws CommException;

    public String getXMLMetaDataConfiguration();

    public void getMetaDataConfiguration (String[] attributes, String[] values);

    public List<String> getAllCachedMetaDataAsXML()
        throws CommException;

    public List<String> getAllCachedMetaDataAsXML (long startTimestamp, long endTimestamp)
        throws CommException;

    public List<String> getMatchingMetaDataAsXML (Map<String, String> attributeValueToMatch)
        throws CommException;

    public List<String> getMatchingMetaDataAsXML (Map<String, String> attributeValueToMatch, long startTimestamp, long endTimestamp)
        throws CommException;

    public PeerNodeContext getPeerNodeContext (String peerNodeId)
        throws CommException;

    public byte[] getMetaDataAsXML (String messageID)
        throws CommException;

    public byte[] getData (String messageID)
        throws CommException;

    public String pushPro (short iClientId, String  groupName, String objectId,
                           String instanceId, String xMLMetadata, byte[] data,
                           long expirationTime, short historyWindow, short tag)
        throws CommException;

    public String pushPro (short iClientId, String groupName, String objectId,
                           String instanceId, List<String> metaDataAttributes,
                           List<String> metaDataValues, byte[] data, long expirationTime,
                           short historyWindow, short tag)
        throws CommException;

    public String pushPro (short iClientId, String groupName, String objectId,
                           String instanceId, String[] metaDataAttributes,
                           String[] metaDataValues, byte[] data, long expirationTime,
                           short historyWindow, short tag)
        throws CommException;

    public String makeAvailablePro (short iClientId, String groupName, String objectId,
                                    String instanceId, String xMLMetadata, byte[] data,
                                    String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException;

    public String makeAvailablePro (short iClientId, String groupName, String objectId,
                                    String instanceId, String[] metaDataAttributes,
                                    String[] metaDataValues, byte[] data, String dataMimeType,
                                    long expirationTime, short historyWindow, short tag)
        throws CommException;

    public boolean pathRegistered (NodePath path, String nodeId, String team, String mission);

    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId);
}
