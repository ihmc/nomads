package us.ihmc.aci.dspro2;

import java.io.ByteArrayOutputStream;
import java.net.ConnectException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.naming.LimitExceededException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProProxy.PeerNodeContext;
import us.ihmc.aci.disServiceProxy.ConnectionStatusListener;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxy;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Reassembler;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.sync.ConcurrentProxy;
import us.ihmc.util.StringUtil;
import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Interval;

import static java.nio.charset.StandardCharsets.UTF_8;

public interface DSProProxyInterface extends Fragmenter, Reassembler, MatchmakingLogListener, DSProProxyListener, SearchListener
{
    class DataWrapper
    {
        public DataWrapper (byte[] data, boolean hasMoreChunks, String queryId)
        {
            _data = data;
            _hasMoreChunks = hasMoreChunks;
            _queryId = queryId;
        }

        public final byte[] _data;
        public final boolean _hasMoreChunks;
        public final String _queryId;
    }

    int init();
    void reinitialize();
    void disconnect();
    boolean isInitialized();
    boolean configureProperties (Properties properties) throws CommException;
    boolean setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                               float impRankWeight, float predRankWeight, float targetWeight,
                               boolean strictTarget) throws CommException;

    boolean addPeer (short adaptorType, String networkInterface, String remoteAddress, short port)
            throws CommException;

    void addUserId (String userId) throws CommException;
    void setMissionId (String missionId) throws CommException;
    void setRole (String role) throws CommException;
    void setNodeType (String nodeType) throws CommException;

    void setDefaultUsefulDistance (int usefulDistanceInMeters) throws CommException;
    void setUsefulDistance (String dataMimeType, int usefulDistanceInMeters) throws CommException;

    void setRangeOfInfluence (String milStd2525Symbol, int rangeOfInfluenceInMeters) throws CommException;

    void setSelectivity (float fThreshold) throws CommException;

    void setRankingWeights (float coordRankWeight, float timeRankWeight, float expirationRankWeight,
                            float impRankWeight, float sourceReliabilityRankWeigth,
                            float informationContentRankWeigth, float predRankWeight,
                            float targetWeight, boolean bStrictTarget,
                            boolean bConsiderFuturePathSegmentForMatchmacking) throws CommException;

    boolean registerPath (NodePath path) throws CommException;
    boolean setCurrentPath (String pathID) throws CommException;
    boolean setCurrentPosition (float fLatitude, float fLongitude, float fAltitude, String location, String note)
            throws CommException;

    String getSessionId() throws CommException;
    String getNodeId() throws CommException;
    String getNodeContext() throws CommException;
    String getNodeContext(String nodeId) throws CommException;
    List<String> getPeerList() throws CommException;
    NodePath getPathForPeer (String peerNodeId) throws CommException;

    String getDSProId (String objectId, String instanceId) throws CommException;
    List<String> getAllCachedMetadata() throws CommException;
    List<String> getAllCachedMetadata(long startTimestamp, long endTimestamp) throws CommException;
    List<String> getMatchingMetadata(Map<String, String> attributeValueToMatch) throws CommException;
    List<String> getMatchingMetadata(Map<String, String> attributeValueToMatch, long startTimestamp,
                                            long endTimestamp) throws CommException;

    NodePath getCurrentPath() throws CommException;

    void changeEncryptionKey (byte[] key) throws CommException;

    // Publish

    String addMessage (String groupName, String objectId, String instanceId, String jsonMetadata,
                              byte[] data, long expirationTime) throws CommException;

    String addMessage (String groupName, String objectId, String instanceId, Map<Object, Object> metadata,
                              byte[] data, long expirationTime) throws CommException;

    String addMessage (String groupName, String objectId, String instanceId, String[] metaDataAttributes,
                              String[] metaDataValues, byte[] data, long expirationTime) throws CommException;

    String addChunk (String groupName, String objectId, String instanceId, String jsonMetadata,
                            ChunkWrapper chunk, long expirationTime) throws CommException;
    String addChunk (String groupName, String objectId, String instanceId, Map<Object, Object> metadata,
                            ChunkWrapper chunk, long expirationTime) throws CommException;
    void addAdditionalChunk (String messageId, ChunkWrapper chunk) throws CommException;

    String addChunkedMessage (String groupName, String objectId, String instanceId, String jsonMetadata,
                              List<ChunkWrapper> chunks, String dataMimeType, long expirationTime)
            throws CommException;
    String addChunkedMessage (String groupName, String objectId, String instanceId, Map<Object, Object> metadata,
                                     List<ChunkWrapper> chunks, String dataMimeType, long expirationTime)
            throws CommException;


    String chunkAndAddMessage (String groupName, String objectId, String instanceId, String jsonMetadata,
                               byte[] data, String dataMimeType, long expirationTime) throws CommException;
    String chunkAndAddMessage (String groupName, String objectId, String instanceId, String[] metaDataAttributes,
                                      String[] metaDataValues, byte[] data, String dataMimeType, long expirationTime)
            throws CommException;
    String chunkAndAddMessage (String groupName, String objectId, String instanceId, Map metadata, byte[] data,
                                      String dataMimeType, long expirationTime) throws CommException;


    String disseminateMessage (String groupName, String objectId, String instanceId, byte[] data,
                               long expirationTime) throws CommException;
    String disseminateMessageMetadata (String groupName, String objectId, String instanceId, String metadata,
                                       byte[] data, String mimeType, long expirationTime) throws CommException;
    String disseminateMessageMetadata (String groupName, String objectId, String instanceId, byte[] metadata,
                                       byte[] data, String mimeType, long expirationTime) throws CommException;

    boolean subscribe (String groupName, byte priority, boolean groupReliable, boolean msgReliable,
                       boolean sequenced) throws CommException;

    // Cancel

    void cancel (String dsproId) throws CommException, ProtocolException;
    void cancel (String objectId, String instanceId) throws CommException, ProtocolException;

    // Requests

    void requestCustomAreaChunk (String chunkedMsgId, String mimeType, long ui32StartXPixel, long ui32EndXPixel,
                                        long ui32StartYPixel, long ui32EndYPixel, byte ui8CompressionQuality,
                                        long timeoutInMilliseconds) throws CommException;
    void requestCustomTimeChunk (String chunkedMsgId, String mimeType, long startTime, long endTime,
                                        byte ui8CompressionQuality, long timeoutInMilliseconds) throws CommException;

    boolean requestMoreChunks (String messageId) throws CommException;
    boolean requestMoreChunks (String messageId, String callbackParameters) throws CommException;

    // Get Data
    DataWrapper getData (String messageID) throws CommException;
    DataWrapper getData (String messageID, String callbackParameters) throws CommException;

    // Search
    String search (String groupName, String queryType, String queryQualifiers, byte[] query) throws CommException;
    String search (String groupName, String queryType, String queryQualifiers, byte[] query,
                          long timeoutInMilliseconds) throws CommException;

    void replyToSearch (String queryId, Collection<String> disServiceMsgIds) throws CommException;
    void replyToQuery (String queryId, byte[] reply) throws CommException;

    // Register Listeners

    int registerDSProProxyListener (DSProProxyListener listener) throws CommException;
    void deregisterDSProProxyListener (DSProProxyListener listener) throws CommException;

    int registerSearchListener (SearchListener listener) throws CommException;
    void deregisterSearchListener (SearchListener listener) throws CommException;

    int registerLocalChunkFragmenter (String mimeType, Fragmenter fragmenter)
            throws CommException, ProtocolException;

    int registerChunkFragmenter (String mimeType, Fragmenter fragmenter)
            throws CommException, ProtocolException;

    int registerChunkReassembler (String mimeType, Reassembler reassembler)
            throws CommException, ProtocolException;

    int registerLocalChunkReassembler (String mimeType, Reassembler reassembler);

    byte[] reassemble (Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations,
                       String mimeType, byte totalNumberOfChunks, byte compressionQuality);

    void registerCtrlMsgListener (CtrlMsgListener listener) throws CommException;

    int registerMatchmakingLogListener (MatchmakingLogListener listener) throws CommException;
    void deregisterMatchmakingLogListener (int clientId, MatchmakingLogListener listener) throws CommException;

    void registerConnectionStatusListener (ConnectionStatusListener listener);
}
