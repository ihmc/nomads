package us.ihmc.aci.dspro2;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProxy.ConnectionStatusListener;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Reassembler;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Interval;

public class AbstractDSProProxy implements DSProProxyInterface
{
    protected final DSProProxyInterface proxy;

    public AbstractDSProProxy(DSProProxyInterface proxy)
    {
        this.proxy = proxy;
    }

    @Override
    public int init()
    {
        return proxy.init();
    }

    @Override
    public void reinitialize()
    {
        proxy.reinitialize();
    }

    @Override
    public void disconnect()
    {
        proxy.disconnect();
    }

    @Override
    public boolean isInitialized()
    {
        return proxy.isInitialized();
    }

    @Override
    public boolean configureProperties (Properties properties) throws CommException
    {
        return proxy.configureProperties (properties);
    }

    @Override
    public boolean setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                                      float impRankWeight, float predRankWeight, float targetWeight,
                                      boolean strictTarget) throws CommException
    {
        return proxy.setRankingWeights (coordRankWeight, timeRankWeight, expirationRankWeight, impRankWeight,
                                        predRankWeight, targetWeight, strictTarget);
    }

    @Override
    public void changeEncryptionKey (byte[] key) throws CommException
    {
        proxy.changeEncryptionKey (key);
    }

    @Override
    public boolean addPeer (short adaptorType, String networkInterface, String remoteAddress, short port)
                throws CommException
    {
        return proxy.addPeer (adaptorType, networkInterface, remoteAddress, port);
    }

    @Override
    public void addUserId (String userId) throws CommException
    {
        proxy.addUserId (userId);
    }

    @Override
    public void setMissionId (String missionId) throws CommException
    {
        proxy.setMissionId (missionId);
    }

    @Override
    public void setRole (String role) throws CommException
    {
        proxy.setRole (role);
    }

    @Override
    public void setNodeType (String nodeType) throws CommException
    {
        proxy.setNodeType (nodeType);
    }

    @Override
    public void setDefaultUsefulDistance (int usefulDistanceInMeters) throws CommException
    {
        proxy.setDefaultUsefulDistance (usefulDistanceInMeters);
    }

    @Override
    public void setUsefulDistance (String dataMimeType, int usefulDistanceInMeters) throws CommException
    {
        proxy.setUsefulDistance (dataMimeType, usefulDistanceInMeters);
    }

    @Override
    public void setRangeOfInfluence (String milStd2525Symbol, int rangeOfInfluenceInMeters) throws CommException
    {
        proxy.setRangeOfInfluence (milStd2525Symbol, rangeOfInfluenceInMeters);
    }

    @Override
    public void setSelectivity (float fThreshold) throws CommException
    {
        proxy.setSelectivity (fThreshold);
    }

    @Override
    public void setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                                   float impRankWeight, float sourceReliabilityRankWeigth,
                                   float informationContentRankWeigth, float predRankWeight,
                                   float targetWeight, boolean bStrictTarget,
                                   boolean bConsiderFuturePathSegmentForMatchmacking) throws CommException
    {

    }

    @Override
    public boolean registerPath (NodePath path) throws CommException
    {
        return proxy.registerPath (path);
    }

    @Override
    public boolean setCurrentPath (String pathID) throws CommException
    {
        return proxy.setCurrentPath (pathID);
    }

    @Override
    public boolean setCurrentPosition (float fLatitude, float fLongitude, float fAltitude, String location, String note)
            throws CommException
    {
        return proxy.setCurrentPosition (fLatitude, fLongitude, fAltitude, location, note);
    }

    @Override
    public String getSessionId() throws CommException
    {
        return proxy.getSessionId();
    }

    @Override
    public String getNodeId() throws CommException
    {
        return proxy.getNodeId();
    }

    @Override
    public String getNodeContext() throws CommException
    {
        return proxy.getNodeContext();
    }

    @Override
    public String getNodeContext(String nodeId) throws CommException
    {
        return proxy.getNodeContext (nodeId);
    }

    @Override
    public List<String> getPeerList() throws CommException
    {
        return proxy.getPeerList();
    }

    @Override
    public String getDSProId (String objectId, String instanceId) throws CommException
    {
        return proxy.getDSProId (objectId, instanceId);
    }

    @Override
    public NodePath getPathForPeer (String peerNodeId) throws CommException
    {
        return proxy.getPathForPeer (peerNodeId);
    }


    @Override
    public List<String> getAllCachedMetadata() throws CommException
    {
        return proxy.getAllCachedMetadata();
    }

    @Override
    public List<String> getAllCachedMetadata(long startTimestamp, long endTimestamp) throws CommException
    {
        return proxy.getAllCachedMetadata(startTimestamp, endTimestamp);
    }

    @Override
    public List<String> getMatchingMetadata(Map<String, String> attributeValueToMatch) throws CommException
    {
        return proxy.getMatchingMetadata(attributeValueToMatch);
    }

    @Override
    public List<String> getMatchingMetadata(Map<String, String> attributeValueToMatch, long startTimestamp,
                                            long endTimestamp) throws CommException
    {
        return proxy.getMatchingMetadata(attributeValueToMatch, startTimestamp, endTimestamp);
    }

    @Override
    public NodePath getCurrentPath() throws CommException
    {
        return proxy.getCurrentPath();
    }

    @Override
    public String addMessage (String groupName, String objectId, String instanceId, String jsonMetadata,
                              byte[] data, long expirationTime) throws CommException
    {
        return proxy.addMessage (groupName, objectId, instanceId, jsonMetadata, data, expirationTime);
    }

    @Override
    public String addMessage (String groupName, String objectId, String instanceId, Map<Object, Object> metadata,
                              byte[] data, long expirationTime) throws CommException
    {
        return proxy.addMessage (groupName, objectId, instanceId, metadata, data, expirationTime);
    }

    @Override
    public String addMessage (String groupName, String objectId, String instanceId, String[] metaDataAttributes,
                              String[] metaDataValues, byte[] data, long expirationTime) throws CommException
    {
        return proxy.addMessage (groupName, objectId, instanceId, metaDataAttributes, metaDataValues, data,
                                 expirationTime);
    }

    @Override
    public String addChunk (String groupName, String objectId, String instanceId, String jsonMetadata,
                            ChunkWrapper chunk, long expirationTime) throws CommException
    {
        return proxy.addChunk (groupName, objectId, instanceId, jsonMetadata, chunk, expirationTime);
    }

    @Override
    public String addChunk (String groupName, String objectId, String instanceId, Map<Object, Object> metadata,
                            ChunkWrapper chunk, long expirationTime) throws CommException
    {
        return proxy.addChunk (groupName, objectId, instanceId, metadata, chunk, expirationTime);
    }

    @Override
    public void addAdditionalChunk (String messageId, ChunkWrapper chunk) throws CommException
    {
        proxy.addAdditionalChunk (messageId, chunk);
    }

    @Override
    public String addChunkedMessage (String groupName, String objectId, String instanceId, String jsonMetadata,
                                     List<ChunkWrapper> chunks, String dataMimeType, long expirationTime)
            throws CommException
    {
        return proxy.addChunkedMessage (groupName, objectId, instanceId, jsonMetadata, chunks, dataMimeType, expirationTime);
    }

    @Override
    public String addChunkedMessage (String groupName, String objectId, String instanceId, Map<Object, Object> metadata,
                                     List<ChunkWrapper> chunks, String dataMimeType, long expirationTime)
            throws CommException
    {
        return proxy.addChunkedMessage (groupName, objectId, instanceId, metadata, chunks, dataMimeType, expirationTime);
    }

    @Override
    public String chunkAndAddMessage (String groupName, String objectId, String instanceId, String jsonMetadata,
                                      byte[] data, String dataMimeType, long expirationTime) throws CommException
    {
        return proxy.chunkAndAddMessage (groupName, objectId, instanceId, jsonMetadata, data, dataMimeType, expirationTime);
    }

    @Override
    public String chunkAndAddMessage (String groupName, String objectId, String instanceId, String[] metaDataAttributes,
                                      String[] metaDataValues, byte[] data, String dataMimeType, long expirationTime)
            throws CommException
    {
        return proxy.chunkAndAddMessage (groupName, objectId, instanceId, metaDataAttributes, metaDataValues, data,
                                         dataMimeType, expirationTime);
    }

    @Override
    public String chunkAndAddMessage (String groupName, String objectId, String instanceId, Map metadata, byte[] data,
                                      String dataMimeType, long expirationTime) throws CommException
    {
        return proxy.chunkAndAddMessage (groupName, objectId, instanceId, metadata, data, dataMimeType, expirationTime);
    }

    @Override
    public String disseminateMessage (String groupName, String objectId, String instanceId, byte[] data,
                                      long expirationTime) throws CommException
    {
        return proxy.disseminateMessage (groupName, objectId, instanceId, data, expirationTime);
    }

    @Override
    public String disseminateMessageMetadata (String groupName, String objectId, String instanceId, String metadata,
                                              byte[] data, String mimeType, long expirationTime) throws CommException
    {
        return proxy.disseminateMessageMetadata (groupName, objectId, instanceId, metadata, data, mimeType, expirationTime);
    }

    @Override
    public String disseminateMessageMetadata (String groupName, String objectId, String instanceId, byte[] metadata,
                                              byte[] data, String mimeType, long expirationTime) throws CommException
    {
        return proxy.disseminateMessageMetadata (groupName, objectId, instanceId, metadata, data, mimeType,
                                                 expirationTime);
    }

    @Override
    public boolean subscribe (String groupName, byte priority, boolean groupReliable, boolean msgReliable,
                              boolean sequenced) throws CommException
    {
        return proxy.subscribe (groupName, priority, groupReliable, msgReliable, sequenced);
    }

    // Cancel

    @Override
    public void cancel (String dsproId) throws CommException, ProtocolException
    {
        proxy.cancel (dsproId);
    }

    @Override
    public void cancel (String objectId, String instanceId) throws CommException, ProtocolException
    {
        proxy.cancel (objectId, instanceId);
    }

    // Requests

    @Override
    public void requestCustomAreaChunk (String chunkedMsgId, String mimeType, long ui32StartXPixel, long ui32EndXPixel,
                                        long ui32StartYPixel, long ui32EndYPixel, byte ui8CompressionQuality,
                                        long timeoutInMilliseconds) throws CommException
    {
        proxy.requestCustomAreaChunk (chunkedMsgId, mimeType, ui32StartXPixel, ui32EndXPixel,
                                      ui32StartYPixel, ui32EndYPixel, ui8CompressionQuality,
                                      timeoutInMilliseconds);
    }

    @Override
    public void requestCustomTimeChunk (String chunkedMsgId, String mimeType, long startTime, long endTime,
                                        byte ui8CompressionQuality, long timeoutInMilliseconds) throws CommException
    {
        proxy.requestCustomTimeChunk (chunkedMsgId, mimeType, startTime, endTime, ui8CompressionQuality, timeoutInMilliseconds);
    }

    @Override
    public boolean requestMoreChunks (String messageId) throws CommException
    {
        return proxy.requestMoreChunks (messageId);
    }

    @Override
    public boolean requestMoreChunks (String messageId, String callbackParameters) throws CommException
    {
        return proxy.requestMoreChunks (messageId, callbackParameters);
    }

    // Get Data

    @Override
    public DataWrapper getData (String messageID) throws CommException
    {
        return proxy.getData (messageID);
    }

    @Override
    public DataWrapper getData (String messageID, String callbackParameters) throws CommException
    {
        return proxy.getData (messageID, callbackParameters);
    }

    @Override
    public List<ChunkWrapper> fragment (byte[] data, String inputMimeType,
                                        byte nChunks, byte compressionQuality)
    {
        return proxy.fragment (data, inputMimeType, nChunks, compressionQuality);
    }

    @Override
    public byte[] extract (byte[] data, String inputMimeType, byte nChunks,
                           byte compressionQuality, Collection<Interval> intervals)
    {
        return proxy.extract (data, inputMimeType, nChunks, compressionQuality, intervals);
    }

    @Override
    public byte[] reassemble (Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations,
                              String mimeType, byte totalNumberOfChunks, byte compressionQuality)
    {
        return proxy.reassemble (chunks, annotations, mimeType, totalNumberOfChunks, compressionQuality);
    }

    // Search

    @Override
    public String search (String groupName, String queryType, String queryQualifiers, byte[] query) throws CommException
    {
        return proxy.search (groupName, queryType, queryQualifiers, query);
    }

    @Override
    public String search (String groupName, String queryType, String queryQualifiers, byte[] query,
                          long timeoutInMilliseconds) throws CommException
    {
        return proxy.search (groupName, queryType, queryQualifiers, query, timeoutInMilliseconds);
    }

    @Override
    public void replyToSearch (String queryId, Collection<String> disServiceMsgIds) throws CommException
    {
        proxy.replyToSearch (queryId, disServiceMsgIds);
    }

    @Override
    public void replyToQuery (String queryId, byte[] reply) throws CommException
    {
        proxy.replyToQuery (queryId, reply);
    }

    // Register Listeners

    @Override
    public int registerDSProProxyListener (DSProProxyListener listener) throws CommException
    {
        return proxy.registerDSProProxyListener (listener);
    }

    @Override
    public void deregisterDSProProxyListener (DSProProxyListener listener) throws CommException
    {
        proxy.deregisterDSProProxyListener (listener);
    }

    @Override
    public int registerSearchListener (SearchListener listener) throws CommException
    {
        return proxy.registerSearchListener (listener);
    }

    @Override
    public void deregisterSearchListener (SearchListener listener) throws CommException
    {
        proxy.deregisterSearchListener (listener);
    }

    @Override
    public int registerLocalChunkFragmenter (String mimeType, Fragmenter fragmenter)
            throws CommException, ProtocolException
    {
        return proxy.registerLocalChunkFragmenter (mimeType, fragmenter);
    }

    @Override
    public int registerChunkFragmenter (String mimeType, Fragmenter fragmenter)
            throws CommException, ProtocolException
    {
        return proxy.registerChunkFragmenter (mimeType, fragmenter);
    }

    @Override
    public int registerChunkReassembler (String mimeType, Reassembler reassembler)
            throws CommException, ProtocolException
    {
        return proxy.registerChunkReassembler (mimeType, reassembler);
    }

    @Override
    public int registerLocalChunkReassembler (String mimeType, Reassembler reassembler)
    {
        return proxy.registerLocalChunkReassembler (mimeType, reassembler);
    }

    @Override
    public void registerCtrlMsgListener (CtrlMsgListener listener) throws CommException
    {
        proxy.registerCtrlMsgListener (listener);
    }

    @Override
    public int registerMatchmakingLogListener (MatchmakingLogListener listener) throws CommException
    {
        return proxy.registerMatchmakingLogListener (listener);
    }

    @Override
    public void deregisterMatchmakingLogListener (int clientId, MatchmakingLogListener listener) throws CommException
    {
        proxy.deregisterMatchmakingLogListener (clientId, listener);
    }

    @Override
    public void registerConnectionStatusListener (ConnectionStatusListener listener)
    {
        proxy.registerConnectionStatusListener (listener);
    }

    // Callbacks

    @Override
    public boolean dataArrived (String id, String groupName, String objectId, String instanceId,
                                String annotatedObjMsgId, String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String queryId)

    {
        return proxy.dataArrived(id, groupName, objectId, instanceId, annotatedObjMsgId, mimeType, data,
                                 chunkNumber, totChunksNumber, queryId);
    }

    @Override
    public boolean metadataArrived (String id, String groupName, String objectId, String instanceId,
                                    String jsonMetadata, String referredDataId, String queryId)
    {
        return proxy.metadataArrived(id, groupName, objectId, instanceId, jsonMetadata, referredDataId, queryId);
    }

    @Override
    public boolean dataAvailable (String id, String groupName, String objectId, String instanceId,
                                       String referredDataId, String mimeType, byte[] metadata, String queryId)
    {
        return proxy.dataAvailable (id, groupName, objectId, instanceId, referredDataId, mimeType, metadata, queryId);
    }

    @Override
    public void newNeighbor (String peerID)
    {
        proxy.newNeighbor(peerID);
    }

    @Override
    public void deadNeighbor (String peerID)
    {
        proxy.deadNeighbor(peerID);
    }

    @Override
    public boolean pathRegistered (NodePath path, String nodeId, String team, String mission)
    {
        return proxy.pathRegistered(path, nodeId, team, mission);
    }

    @Override
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        return proxy.positionUpdated (latitude, longitude, altitude, nodeId);
    }

    @Override
    public void searchArrived (String queryId, String groupName, String querier,
                                  String queryType, String queryQualifiers, byte[] query)
    {
        proxy.searchArrived (queryId, groupName, querier, queryType, queryQualifiers, query);
    }

    @Override
    public void searchReplyArrived (String queryId, Collection<String> matchingIds,
                                    String responderNodeId)
    {
        proxy.searchReplyArrived (queryId, matchingIds, responderNodeId);
    }

    @Override
    public void searchReplyArrived (String queryId, byte[] reply, String responderNodeId)
    {
        proxy.searchReplyArrived (queryId, reply, responderNodeId);
    }

    @Override
    public void informationMatched (String localNodeID, String peerNodeID,
                                    String skippedObjectID, String skippedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights,
                                    String comment, String operation)
    {
        proxy.informationMatched (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                  rankDescriptors, partialRanks, weights, comment, operation);
    }

    @Override
    public void informationSkipped (String localNodeID, String peerNodeID,
                                    String skippedObjectID, String skippedObjectName,
                                    String[] rankDescriptors,
                                    float[] partialRanks, float[] weights,
                                    String comment, String operation)
    {
        proxy.informationSkipped (localNodeID, peerNodeID, skippedObjectID, skippedObjectName,
                                  rankDescriptors, partialRanks, weights, comment, operation);
    }
}

