package us.ihmc.aci.dspro2.legacy;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyInterface;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyListener;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProxy.ConnectionStatusListener;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyListener;
import us.ihmc.aci.disServiceProxy.ListenerAlreadyRegisteredException;
import us.ihmc.aci.disServiceProxy.PeerStatusListener;
import us.ihmc.aci.disServiceProxy.Utils;
import us.ihmc.aci.dspro2.DSProProxyInterface;
import us.ihmc.aci.dspro2.DSProProxyInterface.DataWrapper;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class LegacyDSProProxy implements DisServiceProProxyInterface
{
    private static Logger LOGGER = LoggerFactory.getLogger(LegacyDSProProxy.class);
    private final DSProProxyInterface _proxy;

    public LegacyDSProProxy (DSProProxyInterface proxy)
    {
        _proxy = proxy;
    }

    @Override
    public void registerDisServiceProProxyListener(DisServiceProProxyListener listener) throws CommException
    {
        _proxy.registerDSProProxyListener (new LegacyDSProProxyListener (listener));
    }

    @Override
    public void registerMatchmakingLogListener(MatchmakingLogListener listener) throws CommException
    {
        _proxy.registerMatchmakingLogListener (listener);
    }

    @Override
    public void registerSearchListener(us.ihmc.aci.disServiceProxy.SearchListener listener)
    {
        try {
            _proxy.registerSearchListener (new LegacySearchListener (listener));
        } 
        catch (CommException ex) {
            LOGGER.warn(StringUtil.getStackTraceAsString(ex));
        }
    }

    @Override
    public int configureMetaDataFields(String xMLMetadataFields)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public int configureMetaDataFields(String[] attributes, String[] values)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean configureProperties(Properties properties) throws CommException
    {
        return _proxy.configureProperties (properties);
    }

    @Override
    public boolean setMetadataPossibleValues(String xMLMetadataValues) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean setMetadataPossibleValues(String[] attributes, String[] values) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                                      float impRankWeight, float predRankWeight, float targetWeight, boolean strictTarget)
        throws CommException
    {
        return _proxy.setRankingWeights (coordRankWeight, timeRankWeight, expirationRankWeight,
                                         impRankWeight, predRankWeight, targetWeight, strictTarget);
    }

    @Override
    public boolean registerPath (NodePath path)
        throws CommException
    {
        return _proxy.registerPath (path);
    }

    @Override
    public boolean setActualPath(String pathID) throws CommException
    {
        return _proxy.setCurrentPath (pathID);
    }

    @Override
    public boolean setNodeContext(String teamID, String missionID, String role)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public int changePathProbability(String pathID, float fNewProbability)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public int deregisterPath(String pathID)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public NodePath getActualPath() throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public NodePath getPath()
    {
        try {
            return _proxy.getCurrentPath();
        }
        catch (CommException ex) {
            return null;
        }
    }

    @Override
    public boolean setActualPosition (float fLatitude, float fLongitude, float fAltitude,
                                      String location, String note)
        throws CommException
    {
        return _proxy.setCurrentPosition (fLatitude, fLongitude, fAltitude, location, note);
    }

    @Override
    public String getXMLMetaDataConfiguration()
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void getMetaDataConfiguration(String[] attributes, String[] values)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public List<String> getAllCachedMetaDataAsXML() throws CommException
    {
        return _proxy.getAllCachedMetadata();
    }

    @Override
    public List<String> getAllCachedMetaDataAsXML (long startTimestamp, long endTimestamp)
        throws CommException
    {
        return _proxy.getAllCachedMetadata(startTimestamp, endTimestamp);
    }

    @Override
    public List<String> getMatchingMetaDataAsXML(Map<String, String> attributeValueToMatch)
        throws CommException
    {
        return _proxy.getMatchingMetadata(attributeValueToMatch);
    }

    @Override
    public List<String> getMatchingMetaDataAsXML(Map<String, String> attributeValueToMatch, long startTimestamp, long endTimestamp)
        throws CommException
    {
        return _proxy.getMatchingMetadata(attributeValueToMatch, startTimestamp, endTimestamp);
    }

    @Override
    public byte[] getMetaDataAsXML(String messageID) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public byte[] getData(String messageID) throws CommException
    {
        DataWrapper wr = _proxy.getData (messageID);
        if (wr == null) {
            return null;
        }
        return wr._data;
    }

    @Override
    public String search (String groupName, String queryType, String queryQualifiers, byte[] query)
        throws CommException
    {
        return _proxy.search (groupName, queryType, queryQualifiers, query);
    }

    @Override
    public String pushPro (short iClientId, String groupName, String objectId, String instanceId,
                           String xMLMetadata, byte[] data,
                           long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.addMessage (groupName, objectId, instanceId, xMLMetadata, data, (int) expirationTime);
    }

    @Override
    public String pushPro (short iClientId, String groupName, String objectId, String instanceId, List<String> metaDataAttributes,
                           List<String> metaDataValues, byte[] data, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.addMessage (groupName, objectId, instanceId,
                                  metaDataAttributes.toArray(new String[metaDataAttributes.size()]),
                                  metaDataValues.toArray(new String[metaDataValues.size()]),
                                  data, (int) expirationTime);
    }

    @Override
    public String pushPro (short iClientId, String groupName, String objectId, String instanceId,
                           String[] metaDataAttributes, String[] metaDataValues,
                           byte[] data, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.addMessage (groupName, objectId, instanceId,
                                  metaDataAttributes, metaDataValues,
                                  data, (int) expirationTime);
    }

    @Override
    public String makeAvailablePro (short iClientId, String groupName, String objectId, String instanceId,
                                    String xMLMetadata, byte[] data,
                                    String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.chunkAndAddMessage (groupName, objectId, instanceId, xMLMetadata,
                                          data, dataMimeType, (int) expirationTime);
    }

    @Override
    public String makeAvailablePro (short iClientId, String groupName, String objectId, String instanceId,
                                    String[] metaDataAttributes, String[] metaDataValues,
                                    byte[] data, String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.chunkAndAddMessage (groupName, objectId, instanceId,
                                          metaDataAttributes, metaDataValues,
                                          data, dataMimeType, (int) expirationTime);
    }

    @Override
    public boolean pathRegistered(NodePath path, String nodeId, String team, String mission)
    {
        return _proxy.pathRegistered (path, nodeId, team, mission);
    }

    @Override
    public boolean positionUpdated(float latitude, float longitude, float altitude, String nodeId)
    {
        return _proxy.positionUpdated (latitude, longitude, altitude, nodeId);
    }

    @Override
    public int init() throws Exception
    {
        return _proxy.init();
    }

    @Override
    public void reinitialize()
    {
        _proxy.reinitialize();
    }

    @Override
    public boolean isInitialized()
    {
        return _proxy.isInitialized();
    }

    @Override
    public String getNodeId() throws CommException
    {
        return _proxy.getNodeId();
    }

    @Override
    public List<String> getPeerList()
    {
        try {
            return _proxy.getPeerList();
        }
        catch(CommException ex) {
            throw new RuntimeException(ex);
        }
    }

    @Override
    public String store (String groupName, String objectId, String instanceId,
                         String mimeType, byte[] metaData, byte[] data, long expiration,
                         short historyWindow, short tag, byte priority)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void push(String msgId) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public String push (String groupName, String objectId, String instanceId,
                        String mimeType, byte[] metaData, byte[] data, long expiration,
                        short historyWindow, short tag, byte priority)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public String makeAvailable (String groupName, String objectId, String instanceId,
                                 byte[] metadata, byte[] data,
                                 String dataMimeType, long expiration, short historyWindow,
                                 short tag, byte priority) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void cancel(String id)
        throws CommException, ProtocolException
    {
        _proxy.cancel (id);
    }

    @Override
    public void cancel (short tag)
        throws CommException, ProtocolException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean addFilter(String groupName, short tag)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean removeFilter(String groupName, short tag) throws CommException {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean requestMoreChunks(String groupName, String senderNodeId, int seqId)
        throws CommException
    {
        try {
            return _proxy.requestMoreChunks (Utils.getChunkMessageID (senderNodeId, groupName, seqId), null);
        }
        catch (Exception ex) {
            LOGGER.warn(StringUtil.getStackTraceAsString(ex));
            return false;
        }
    }

    @Override
    public boolean requestMoreChunks(String messageId)
        throws CommException
    {
        return requestMoreChunks(messageId, null);
    }

    public boolean requestMoreChunks(String messageId, String callbackParameters)
        throws CommException
    {
        return _proxy.requestMoreChunks (messageId, callbackParameters);
    }

    @Override
    public byte[] retrieve(String id, int timeout)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public int retrieve(String id, String filePath)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean request(String groupName, short tag, short historyLength, long timeout)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean subscribe (String groupName, byte priority, boolean groupReliable,
                              boolean msgReliable, boolean sequenced)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean subscribe (String groupName, short tag, byte priority, boolean groupReliable,
                              boolean msgReliable, boolean sequenced)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean subscribe (String groupName, byte predicateType, String predicate, byte priority,
                              boolean groupReliable, boolean msgReliable, boolean sequenced)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean unsubscribe(String groupName) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public boolean unsubscribe(String groupName, short tag)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void registerDisseminationServiceProxyListener(DisseminationServiceProxyListener listener)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void registerDisseminationServiceProxyListener(short clientId, DisseminationServiceProxyListener listener)
        throws ListenerAlreadyRegisteredException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void registerPeerStatusListener (PeerStatusListener listener)
    {
        try {
            _proxy.registerDSProProxyListener (new LegacyPeerStatusListener (listener));
        }
        catch (CommException ex) {
            LOGGER.warn(StringUtil.getStackTraceAsString(ex));
        }
    }

    @Override
    public void registerConnectionStatusListener (ConnectionStatusListener listener)
    {
        _proxy.registerConnectionStatusListener (listener);
    }

    @Override
    public boolean resetTransmissionHistory()
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void dataArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                String instanceId, String mimeType, byte[] data, int metadataLength, short tag,
                                byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead
    }

    @Override
    public void chunkArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                 String instanceId, String mimeType, byte[] data, short nChunks, short totNChunks,
                                 String chunhkedMsgId, short tag, byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead
    }

    @Override
    public void metadataArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                    String instanceId, String mimeType, byte[] metadata, boolean dataChunked,
                                    short tag, byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead)
    }

    @Override
    public void dataAvailable (String msgId, String sender, String groupName, int seqNum, String objectId,
                                  String instanceId, String mimeType, String id, byte[] metadata, short tag,
                                  byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead)
    }

    @Override
    public void newPeer(String peerID)
    {
        // This method will never be called, because the PeerStatusListener
        // is no registered (the LegacyPeerStatusListener is registered instead)
    }

    @Override
    public void deadPeer(String peerID)
    {
        // This method will never be called, because the PeerStatusListener
        // is no registered (the LegacyPeerStatusListener is registered instead)
    }

    @Override
    public void informationMatched (String localNodeID, String peerNodeID, String matchedObjectID,
                                    String matchedObjectName, String[] rankDescriptors, float[] partialRanks,
                                    float[] weights, String comment, String operation)
    {
        _proxy.informationMatched (localNodeID, peerNodeID, matchedObjectID, matchedObjectName,
                                   rankDescriptors, partialRanks, weights, comment, operation);
    }

    @Override
    public void informationSkipped (String localNodeID, String peerNodeID, String skippedObjectID,
                                    String skippedObjectName, String[] rankDescriptors, float[] partialRanks,
                                    float[] weights, String comment, String operation)
    {
        _proxy.informationSkipped (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                   rankDescriptors, partialRanks, weights, comment, operation);
    }

    @Override
    public void searchArrived (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
    {
        _proxy.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);
    }

    @Override
    public void searchReplyArrived (String queryId, Collection<String> matchingIds, String responderNodeId)
    {
        _proxy.searchReplyArrived (queryId, matchingIds, responderNodeId);
    }

    @Override
    public void searchReplyArrived(String queryId, byte[] reply, String responderNodeId)
    {
        _proxy.searchReplyArrived (queryId, reply, responderNodeId);
    }

    @Override
    public void replyToSearch(String queryId, Collection<String> disServiceMsgIds)
        throws CommException
    {
        _proxy.replyToSearch(queryId, disServiceMsgIds);
    }
}
