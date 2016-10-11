package us.ihmc.aci.dspro2.legacy;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyInterface;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyListener;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProProxy.PeerNodeContext;
import us.ihmc.aci.disServiceProxy.ConnectionStatusListener;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyListener;
import us.ihmc.aci.disServiceProxy.ListenerAlreadyRegisteredException;
import us.ihmc.aci.disServiceProxy.PeerStatusListener;
import us.ihmc.aci.disServiceProxy.Utils;
import us.ihmc.aci.dspro2.DSProProxy;
import us.ihmc.aci.dspro2.DSProProxy.DataWrapper;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class LegacyDSProProxy implements DisServiceProProxyInterface
{
    private final DSProProxy _proxy;

    public LegacyDSProProxy (DSProProxy proxy)
    {
        _proxy = proxy;
    }

    public void registerDisServiceProProxyListener(DisServiceProProxyListener listener) throws CommException
    {
        _proxy.registerDSProProxyListener (new LegacyDSProProxyListener (listener));
    }

    public void registerMatchmakingLogListener(MatchmakingLogListener listener) throws CommException
    {
        _proxy.registerMatchmakingLogListener (listener);
    }

    public void registerSearchListener(us.ihmc.aci.disServiceProxy.SearchListener listener)
    {
        try {
            _proxy.registerSearchListener (new LegacySearchListener (listener));
        } 
        catch (CommException ex) {
            Logger.getLogger(LegacyDSProProxy.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public int configureMetaDataFields(String xMLMetadataFields)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public int configureMetaDataFields(String[] attributes, String[] values)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean configureProperties(Properties properties) throws CommException
    {
        return _proxy.configureProperties (properties);
    }

    public boolean setMetadataPossibleValues(String xMLMetadataValues) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean setMetadataPossibleValues(String[] attributes, String[] values) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                                      float impRankWeight, float predRankWeight, float targetWeight, boolean strictTarget)
        throws CommException
    {
        return _proxy.setRankingWeights (coordRankWeight, timeRankWeight, expirationRankWeight,
                                         impRankWeight, predRankWeight, targetWeight, strictTarget);
    }

    public boolean registerPath (NodePath path)
        throws CommException
    {
        return _proxy.registerPath (path);
    }

    public boolean setActualPath(String pathID) throws CommException
    {
        return _proxy.setCurrentPath (pathID);
    }

    public boolean setNodeContext(String teamID, String missionID, String role)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public int changePathProbability(String pathID, float fNewProbability)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public int deregisterPath(String pathID)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public NodePath getActualPath() throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public NodePath getPath()
    {
        try {
            return _proxy.getCurrentPath();
        }
        catch (CommException ex) {
            return null;
        }
    }

    public NodePath getPathForPeer(String peerNodeId) throws CommException
    {
        return _proxy.getPathForPeer (peerNodeId);
    }

    public boolean setActualPosition (float fLatitude, float fLongitude, float fAltitude,
                                      String location, String note)
        throws CommException
    {
        return _proxy.setCurrentPosition (fLatitude, fLongitude, fAltitude, location, note);
    }

    public String getXMLMetaDataConfiguration()
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void getMetaDataConfiguration(String[] attributes, String[] values)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public List<String> getAllCachedMetaDataAsXML() throws CommException
    {
        return _proxy.getAllCachedMetaDataAsXML();
    }

    public List<String> getAllCachedMetaDataAsXML (long startTimestamp, long endTimestamp)
        throws CommException
    {
        return _proxy.getAllCachedMetaDataAsXML (startTimestamp, endTimestamp);
    }

    public List<String> getMatchingMetaDataAsXML(Map<String, String> attributeValueToMatch)
        throws CommException
    {
        return _proxy.getMatchingMetaDataAsXML (attributeValueToMatch);
    }

    public List<String> getMatchingMetaDataAsXML(Map<String, String> attributeValueToMatch, long startTimestamp, long endTimestamp)
        throws CommException
    {
        return _proxy.getMatchingMetaDataAsXML (attributeValueToMatch, startTimestamp, endTimestamp);
    }

    public PeerNodeContext getPeerNodeContext(String peerNodeId) throws CommException
    {
        return _proxy.getPeerNodeContext (peerNodeId);
    }

    public byte[] getMetaDataAsXML(String messageID) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public byte[] getData(String messageID) throws CommException
    {
        DataWrapper wr = _proxy.getData (messageID);
        if (wr == null) {
            return null;
        }
        return wr._data;
    }

    public String search (String groupName, String queryType, String queryQualifiers, byte[] query)
        throws CommException
    {
        return _proxy.search (groupName, queryType, queryQualifiers, query);
    }

    public String pushPro (short iClientId, String groupName, String objectId, String instanceId,
                           String xMLMetadata, byte[] data,
                           long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.addMessage (groupName, objectId, instanceId, xMLMetadata, data, (int) expirationTime);
    }

    public String pushPro (short iClientId, String groupName, String objectId, String instanceId, List<String> metaDataAttributes,
                           List<String> metaDataValues, byte[] data, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.addMessage (groupName, objectId, instanceId,
                                  metaDataAttributes.toArray(new String[metaDataAttributes.size()]),
                                  metaDataValues.toArray(new String[metaDataValues.size()]),
                                  data, (int) expirationTime);
    }

    public String pushPro (short iClientId, String groupName, String objectId, String instanceId,
                           String[] metaDataAttributes, String[] metaDataValues,
                           byte[] data, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.addMessage (groupName, objectId, instanceId,
                                  metaDataAttributes, metaDataValues,
                                  data, (int) expirationTime);
    }

    public String makeAvailablePro (short iClientId, String groupName, String objectId, String instanceId,
                                    String xMLMetadata, byte[] data,
                                    String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.chunkAndAddMessage (groupName, objectId, instanceId, xMLMetadata,
                                          data, dataMimeType, (int) expirationTime);
    }

    public String makeAvailablePro (short iClientId, String groupName, String objectId, String instanceId,
                                    String[] metaDataAttributes, String[] metaDataValues,
                                    byte[] data, String dataMimeType, long expirationTime, short historyWindow, short tag)
        throws CommException
    {
        return _proxy.chunkAndAddMessage (groupName, objectId, instanceId,
                                          metaDataAttributes, metaDataValues,
                                          data, dataMimeType, (int) expirationTime);
    }

    public boolean pathRegistered(NodePath path, String nodeId, String team, String mission)
    {
        return _proxy.pathRegistered (path, nodeId, team, mission);
    }

    public boolean positionUpdated(float latitude, float longitude, float altitude, String nodeId)
    {
        return _proxy.positionUpdated (latitude, longitude, altitude, nodeId);
    }

    public int init() throws Exception
    {
        return _proxy.init();
    }

    public void reinitialize()
    {
        _proxy.reinitialize();
    }

    public boolean isInitialized()
    {
        return _proxy.isInitialized();
    }

    public String getNodeId() throws CommException
    {
        return _proxy.getNodeId();
    }

    public List<String> getPeerList()
    {
        return _proxy.getPeerList();
    }

    public String store (String groupName, String objectId, String instanceId,
                         String mimeType, byte[] metaData, byte[] data, long expiration,
                         short historyWindow, short tag, byte priority)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void push(String msgId) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public String push (String groupName, String objectId, String instanceId,
                        String mimeType, byte[] metaData, byte[] data, long expiration,
                        short historyWindow, short tag, byte priority)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public String makeAvailable (String groupName, String objectId, String instanceId,
                                 byte[] metadata, byte[] data,
                                 String dataMimeType, long expiration, short historyWindow,
                                 short tag, byte priority) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void cancel(String id)
        throws CommException, ProtocolException
    {
        _proxy.cancel (id);
    }

    public void cancel (short tag)
        throws CommException, ProtocolException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean addFilter(String groupName, short tag)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean removeFilter(String groupName, short tag) throws CommException {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean requestMoreChunks(String groupName, String senderNodeId, int seqId)
        throws CommException
    {
        try {
            return _proxy.requestMoreChunks (Utils.getChunkMessageID (senderNodeId, groupName, seqId), null);
        }
        catch (Exception ex) {
            Logger.getLogger(LegacyDSProProxy.class.getName()).log(Level.SEVERE, null, ex);
            return false;
        }
    }

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

    public byte[] retrieve(String id, int timeout)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public int retrieve(String id, String filePath)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean request(String groupName, short tag, short historyLength, long timeout)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean subscribe (String groupName, byte priority, boolean groupReliable,
                              boolean msgReliable, boolean sequenced)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean subscribe (String groupName, short tag, byte priority, boolean groupReliable,
                              boolean msgReliable, boolean sequenced)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean subscribe (String groupName, byte predicateType, String predicate, byte priority,
                              boolean groupReliable, boolean msgReliable, boolean sequenced)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean unsubscribe(String groupName) throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public boolean unsubscribe(String groupName, short tag)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void registerDisseminationServiceProxyListener(DisseminationServiceProxyListener listener)
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void registerDisseminationServiceProxyListener(short clientId, DisseminationServiceProxyListener listener)
        throws ListenerAlreadyRegisteredException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void registerPeerStatusListener (PeerStatusListener listener)
    {
        try {
            _proxy.registerDSProProxyListener (new LegacyPeerStatusListener (listener));
        }
        catch (CommException ex) {
            Logger.getLogger(LegacyDSProProxy.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void registerConnectionStatusListener (ConnectionStatusListener listener)
    {
        _proxy.registerConnectionStatusListener (listener);
    }

    public boolean resetTransmissionHistory()
        throws CommException
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public void dataArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                String instanceId, String mimeType, byte[] data, int metadataLength, short tag,
                                byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead
    }

    public void chunkArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                 String instanceId, String mimeType, byte[] data, short nChunks, short totNChunks,
                                 String chunhkedMsgId, short tag, byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead
    }

    public void metadataArrived (String msgId, String sender, String groupName, int seqNum, String objectId,
                                    String instanceId, String mimeType, byte[] metadata, boolean dataChunked,
                                    short tag, byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead)
    }

    public void dataAvailable (String msgId, String sender, String groupName, int seqNum, String objectId,
                                  String instanceId, String mimeType, String id, byte[] metadata, short tag,
                                  byte priority, String queryId)
    {
        // This method will never be called, because the DisseminationServiceListener
        // is no registered (the LegacyDSProProxyListener is registered instead)
    }

    public void newPeer(String peerID)
    {
        // This method will never be called, because the PeerStatusListener
        // is no registered (the LegacyPeerStatusListener is registered instead)
    }

    public void deadPeer(String peerID)
    {
        // This method will never be called, because the PeerStatusListener
        // is no registered (the LegacyPeerStatusListener is registered instead)
    }

    public void informationMatched (String localNodeID, String peerNodeID, String matchedObjectID,
                                    String matchedObjectName, String[] rankDescriptors, float[] partialRanks,
                                    float[] weights, String comment, String operation)
    {
        _proxy.informationMatched (localNodeID, peerNodeID, matchedObjectID, matchedObjectName,
                                   rankDescriptors, partialRanks, weights, comment, operation);
    }

    public void informationSkipped (String localNodeID, String peerNodeID, String skippedObjectID,
                                    String skippedObjectName, String[] rankDescriptors, float[] partialRanks,
                                    float[] weights, String comment, String operation)
    {
        _proxy.informationSkipped (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                   rankDescriptors, partialRanks, weights, comment, operation);
    }

    public void searchArrived (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
    {
        _proxy.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);
    }

    public void searchReplyArrived (String queryId, Collection<String> matchingIds, String responderNodeId)
    {
        _proxy.searchReplyArrived (queryId, matchingIds, responderNodeId);
    }

    public void searchReplyArrived(String queryId, byte[] reply, String responderNodeId)
    {
        _proxy.searchReplyArrived (queryId, reply, responderNodeId);
    }

    public void replyToSearch(String queryId, Collection<String> disServiceMsgIds)
        throws CommException
    {
        _proxy.replyToSearch(queryId, disServiceMsgIds);
    }
}
