/**
 * DS Pro Proxy
 *
 * @author Lorenzo Campioni
 */

package us.ihmc.aci.dspro2;

import com.google.protobuf.*;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.StatusRuntimeException;
import io.grpc.netty.GrpcSslContexts;
import io.grpc.netty.NettyChannelBuilder;
import io.grpc.Status;
import us.ihmc.aci.util.dspro.LogUtils;
import us.ihmc.crypto.abe.proto.rpc.*;
import us.ihmc.crypto.abe.proto.*;
import us.ihmc.util.Base64Encoder;
import us.ihmc.util.Base64Decoder;
import us.ihmc.util.Base64FormatException;
import us.ihmc.aci.util.dspro.NodePath;
import javax.net.ssl.SSLException;
import java.io.*;
import java.io.FileInputStream;
import org.slf4j.Logger;

import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Reassembler;
import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.Interval;

import java.util.*;
import java.util.concurrent.TimeUnit;

import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

import javax.xml.stream.XMLStreamException;

import us.ihmc.aci.util.dspro.MetadataElement;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonElement;

public class ABEDSProProxy extends AbstractDSProProxy implements DSProProxyListener {
    private static final String APP_METADATA = MetadataElement.applicationMetadata.name();
    private static final String ABE_EQUATION = MetadataElement.abeEquation.name();
    private static final String ABE_IV = MetadataElement.abeIV.name();
    private static final String ABE_ENCRYPTED_METADATA = MetadataElement.abeEncryptedFields.name();
    private static final String ABE_TEST = "ABE_Test";

    private String _ipABEService;
    private int _portABEService;

    private String _sessionFileName = null;
    private HashMap<Integer,AttributeJava> _attributeMap = new HashMap<>();
    private String _cfgABEfile;
    private String _certificateFile;
    private ABEServiceGrpc.ABEServiceBlockingStub _blockingStub = null;
    private Session _session = null;
    private String _debugEquation = null;
    private static Logger _logger = LogUtils.getLogger(ABEDSProProxy.class);
    private ManagedChannel _channel = null;

    private final Map<String, Fragmenter> _mimeTypeToFragmenter = new HashMap<>();
    private final Map<String, Reassembler> _mimeTypeToReassembler = new HashMap<>();

    private final List<DSProProxyListener> _listenerPro = new ArrayList<>();
    private final List<UndecryptableMessageListener> _undecryptableMessageListeners = new ArrayList<>();

    public ABEDSProProxy(DSProProxyInterface proxy, String abeCfgFile, String certificateFile, String host, int port) {
        super(proxy);
        _cfgABEfile = abeCfgFile;
        _certificateFile = certificateFile;
        _ipABEService = host;
        _portABEService = port;
    }

    public ABEDSProProxy(DSProProxyInterface proxy) {
        this(proxy, null, null, "127.0.0.1", 50051);
    }

    public ABEDSProProxy(DSProProxyInterface proxy, String abeCfgFile) {
        this(proxy, abeCfgFile, null, "127.0.0.1", 50051);
    }

    public ABEDSProProxy(DSProProxyInterface proxy, String abeCfgFile, String certificateFile) {
        this(proxy, abeCfgFile, certificateFile, "127.0.0.1", 50051);
    }

    private int initBlockingStub() {
        boolean sslEnabled = (_certificateFile != null);
        if (!sslEnabled) {
            //no ssl on server
            _channel = ManagedChannelBuilder
                    .forAddress(_ipABEService, _portABEService)
                    .maxInboundMessageSize(Integer.MAX_VALUE)
                    .usePlaintext()
                    .build();
        } else {
            //ssl on server
            try {
                File cert = new File(_certificateFile);
                _channel = NettyChannelBuilder
                        .forAddress (_ipABEService, _portABEService)
                        .sslContext (GrpcSslContexts.forClient().trustManager (cert).build())
                        .maxInboundMessageSize(Integer.MAX_VALUE)
                        .overrideAuthority("ABEService")
                        .build();
                FileInputStream fis = new FileInputStream (cert);
                byte[] data = new byte [(int) cert.length()];
                fis.read (data);
                fis.close();
            } catch (SSLException e) {
                _logger.error("TLS initialization: SSL error {}",e.getMessage());
                return -1;
            } catch (Exception e) {
                _logger.error("TLS initialization: generic error {}",e.getMessage());
                return -1;
            }
        }
        if (_channel == null) {
            _logger.error("initBlockingStub: gRPC Managed Channel null");
            return -1;
        }
        _blockingStub = ABEServiceGrpc.newBlockingStub (_channel);
        if(_blockingStub ==  null){
            _logger.error("initBlockingStub: blockingStub null");
            return -1;
        }
        return 0;
    }


    public synchronized int initABE() {
        if (initBlockingStub() < 0){
            _logger.error("initABE: gRPC ABE Service init blocking stub");
            return -1;
        }
        if (serviceABELogin() < 0){
            _logger.error("initABE: gRPC ABE Service Login failure");
            return -1;
        }
        
        return 0;
    }
    public int addApplicationAttributes(Iterable <Attribute> attList) {
        if(_attributeMap == null) {
            return -1;
        }
        for (Attribute attVer : attList) {
            AttributeVersionJava newAttJava = new AttributeVersionJava(attVer);
            if( _attributeMap.containsKey(newAttJava.id)){
                _attributeMap.get(newAttJava.id).updateVersion(newAttJava);
            }
            else {
                AttributeJava att = new AttributeJava(newAttJava.id, "attribute_" + newAttJava.id);
                att.versions.add(newAttJava);
                _attributeMap.put(newAttJava.id, att);
            }
        }
        return serviceADDAttributes(_attributeMap.values());
    }

    public synchronized int initAttributesFromFile() {
        try {
            return staticInitialization(_cfgABEfile);
        } catch(Exception e) {
            _logger.error("initAttributesFromFile: Exception {}", e.getMessage());
            return -1;
        }
    } 
    //-- interface override --
    @Override
    public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission) {
        return this.pathRegistered (_listenerPro, path, nodeId, teamId, mission);
    }

    private boolean pathRegistered (Collection<DSProProxyListener> listeners, NodePath path, String nodeId,
                                          String team, String mission)
    {
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                _logger.warn("pathRegistered: error:listeners is empty");
                return false;
            }
            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.pathRegistered(path, nodeId, team, mission)) {
                    returnValue = true;
                }
            }
            return returnValue;
        }
    }

    @Override
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId) {
        return this.positionUpdated (_listenerPro, latitude, longitude, altitude, nodeId);
    }

    private boolean positionUpdated (Collection<DSProProxyListener> listeners, float latitude, float longitude,
                                           float altitude, String nodeId)
    {
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                _logger.warn("positionUpdated: error:listeners is empty");
                return false;
            }

            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.positionUpdated(latitude, longitude, altitude, nodeId)) {
                    returnValue = true;
                }
            }
            return returnValue;
        }
    }

    @Override
    public void newNeighbor (String peerID) {
        this.newNeighbor (_listenerPro, peerID);
    }

    private void newNeighbor (Collection<DSProProxyListener> listeners, String peerID)
    {
        if (listeners.isEmpty()) {
            _logger.warn("newNeighbor: error: Listeners is empty");
            return;
        }

        for (DSProProxyListener listener : listeners) {
            listener.newNeighbor(peerID);
        }
    }
    @Override
    public void deadNeighbor (String peerID) {
        this.deadNeighbor (_listenerPro, peerID);
    }

    private void deadNeighbor (Collection<DSProProxyListener> listeners, String peerID)
    {
        if (listeners.isEmpty()) {
            _logger.warn("newNeighbor: Listeners is empty");
            return;
        }
        for (DSProProxyListener listener : listeners) {
            listener.newNeighbor(peerID);
        }
    }

    @Override
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                              String xmlMedatada, byte[] data, long expirationTime) 
        throws CommException 
    {
        return addMessage (groupName, objectId, instanceId, xmlMedatada, data, expirationTime, stringToEquation(_debugEquation)); 
    }

    @Override
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                              Map<Object, Object> metadata, byte[] data, long expirationTime)
        throws CommException
    {
        return addMessage (groupName, objectId, instanceId, metadata, data, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                              String[] metaDataAttributes, String[] metaDataValues,
                              byte[] data, long expirationTime)
        throws CommException
    {
        return addMessage (groupName, objectId, instanceId, metaDataAttributes, metaDataValues, data, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String addChunk (String groupName, String objectId, String instanceId,
                            String xmlMetadata, ChunkWrapper chunk, long expirationTime)
        throws CommException
    {
         return addChunk (groupName, objectId, instanceId, xmlMetadata, chunk, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String addChunk (String groupName, String objectId, String instanceId,
                            Map<Object, Object> metadata, ChunkWrapper chunk, long expirationTime)
        throws CommException
    {
        return addChunk (groupName, objectId, instanceId, metadata, chunk, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized void addAdditionalChunk (String messageId, ChunkWrapper chunk) 
        throws CommException 
    {
        addAdditionalChunk (messageId, chunk, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String addChunkedMessage (String groupName, String objectId, String instanceId,
                                                  String xmlMetadata, List<ChunkWrapper> chunks,
                                                  String dataMimeType, long expirationTime)
        throws CommException
    {
        return addChunkedMessage (groupName, objectId, instanceId, xmlMetadata, chunks, dataMimeType, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String addChunkedMessage (String groupName, String objectId, String instanceId,
                                                  Map<Object,Object> metadata, List<ChunkWrapper> chunks,
                                                  String dataMimeType, long expirationTime)
        throws CommException
    {
        return addChunkedMessage (groupName, objectId, instanceId, metadata, chunks, dataMimeType, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   String[] metaDataAttributes, String[] metaDataValues,
                                                   byte[] data, String dataMimeType,
                                                   long expirationTime)
        throws CommException
    {
        return chunkAndAddMessage (groupName, objectId, instanceId, metaDataAttributes, metaDataValues, data, dataMimeType, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                               String xmlMedatada, byte[] data, String dataMimeType,
                                               long expirationTime)
        throws CommException
    {
        return chunkAndAddMessage (groupName, objectId, instanceId, xmlMedatada, data, dataMimeType, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   Map metadata, byte[] data, String dataMimeType,
                                                   long expirationTime)
        throws CommException
    {
        return chunkAndAddMessage (groupName, objectId, instanceId, metadata, data, dataMimeType, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public synchronized String disseminateMessage (String groupName, String objectId, String instanceId,
                                      byte[] data, long expirationTime)
        throws CommException
    {
        return disseminateMessage (groupName, objectId, instanceId, data, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public String disseminateMessageMetadata (String groupName, String objectId, String instanceId, String metadata,
                                              byte[] data, String mimeType, long expirationTime)
        throws CommException
    {
        return disseminateMessageMetadata (groupName, objectId, instanceId, metadata, data, mimeType, expirationTime, stringToEquation(_debugEquation));
    }

    @Override
    public String disseminateMessageMetadata (String groupName, String objectId, String instanceId, byte[] metadata,
                                              byte[] data, String mimeType, long expirationTime)
        throws CommException
    {
        return disseminateMessageMetadata (groupName, objectId, instanceId, metadata, data, mimeType, expirationTime, stringToEquation(_debugEquation));
    }


    @Override
    public int registerDSProProxyListener (DSProProxyListener listener) 
        throws CommException
    {
        if (listener == null) {
            _logger.warn ("registerDSProProxyListener: Listener is null");
            return -1;
        }
        if(_listenerPro.isEmpty()){
            super.registerDSProProxyListener (this);
            _logger.info ("registerDSProProxyListener: registered ABEProxy as a DSProProxyListener");
        }
        _listenerPro.add(listener);
        return (_listenerPro.size() - 1);
    }
    @Override
    public synchronized void deregisterDSProProxyListener (DSProProxyListener listener)
        throws CommException
    {
        if (listener == null) {
            _logger.warn ("deregisterDSProProxyListener: Listener is null");
            return;
        }
        _listenerPro.remove (listener);
        if (_listenerPro.size() == 0) {
            super.deregisterDSProProxyListener (this);
            _logger.info ("deregisterDSProProxyListener: no application registered as listener, deregistered ABEProxy as a DSProProxyListener");
        }

    }

    public synchronized int registerUndecryptableMessageListener (UndecryptableMessageListener listener) 
    {
        if (listener == null) {
            _logger.warn ("registerUndecryptableMessageListener: Listener is null");
            return -1;
        }
        _undecryptableMessageListeners.add(listener);
        return (_undecryptableMessageListeners.size() - 1);
    }
    
    public synchronized void deregisterUndecryptableMessageListener (UndecryptableMessageListener listener)
        throws CommException
    {
        if (listener == null) {
            _logger.warn ("deregisterUndecryptableMessageListener: Listener is null");
            return;
        }
        _undecryptableMessageListeners.remove (listener);
    }

    @Override
    public synchronized int registerLocalChunkFragmenter (String mimeType, Fragmenter fragmenter)
            throws CommException
    {
        _mimeTypeToFragmenter.put (mimeType, fragmenter);
        return 0;
    }

    @Override
    public synchronized int registerChunkReassembler (String mimeType, Reassembler reassembler)
            throws CommException, ProtocolException
    {
        return super.registerChunkReassembler (mimeType, reassembler);
    }

    @Override
    public synchronized int registerLocalChunkReassembler (String mimeType, Reassembler reassembler)
    {
        _mimeTypeToReassembler.put (mimeType, reassembler);
        return 0;
    }

    @Override
    public byte[] extract (byte[] data, String inputMimeType, byte nChunks,
                           byte compressionQuality, Collection<Interval> intervals) {
        synchronized (_mimeTypeToFragmenter) {
            if (_mimeTypeToFragmenter.containsKey(inputMimeType)) {
                return _mimeTypeToFragmenter.get(inputMimeType).extract(data, inputMimeType, nChunks, compressionQuality, intervals);
            }
        }
        return null;
    }

    @Override
    public byte[] reassemble (Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations,
                              String mimeType, byte totalNumberOfChunks, byte compressionQuality) {
        _logger.debug ("reassemble: Called reassemble for mimetype " + mimeType);
        synchronized (_mimeTypeToReassembler) {
            if (_mimeTypeToReassembler.containsKey(mimeType)) {
                return _mimeTypeToReassembler.get(mimeType).reassemble(chunks, annotations, mimeType,
                        totalNumberOfChunks, compressionQuality);
            }
        }
        return null;
    }

    //------ new API
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                              String xmlMetadata, byte[] data, long expirationTime, DisjunctionEquation ABEPolicyEquation) 
        throws CommException
    {
        Map<Object, Object> newMetadata = standardizeMetadata(xmlMetadata);
        return addMessage (groupName, objectId, instanceId, newMetadata, data, expirationTime, ABEPolicyEquation);
    }

    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                              String[] metaDataAttributes, String[] metaDataValues,
                              byte[] data, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        Map<Object, Object> newMetadata = standardizeMetadata(metaDataAttributes, metaDataValues);
        return addMessage (groupName, objectId, instanceId, newMetadata, data, expirationTime, ABEPolicyEquation);
    }

    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                              Map<Object, Object> metadata, byte[] data, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {   
        if (ABEPolicyEquation == null) {
            
        }
        ArrayList<String> encMetadata = new ArrayList<String>();
        encMetadata.add(APP_METADATA);
        if (encryptMetadata(ABEPolicyEquation, metadata, encMetadata) < 0) {
            return null;
        }
        byte [] encryptedPayload = null;
        if (data != null) {
            encryptedPayload = encryptData(ABEPolicyEquation, data);
            if (encryptedPayload == null) {
                return null;
            }
        }
        return super.addMessage (groupName, objectId, instanceId, metadata, encryptedPayload, expirationTime);
    }

    public synchronized String addChunk (String groupName, String objectId, String instanceId,
                            String xmlMetadata, ChunkWrapper chunk, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        Map<Object, Object> newMetadata = standardizeMetadata(xmlMetadata);
        return addChunk (groupName, objectId, instanceId, newMetadata, chunk, expirationTime, ABEPolicyEquation);
    }

    public synchronized String addChunk (String groupName, String objectId, String instanceId,
                            Map<Object, Object> metadata, ChunkWrapper chunk, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        ArrayList<String> encMetadata = new ArrayList<String>();
        encMetadata.add(APP_METADATA);
        if (encryptMetadata(ABEPolicyEquation, metadata, encMetadata) < 0) {
            return null;
        }
        byte [] encryptedPayload = encryptData(ABEPolicyEquation, chunk.getData());
        if (encryptedPayload == null) {
            return null;
        }
        chunk.setData(encryptedPayload);
        return super.addChunk (groupName, objectId, instanceId, metadata, chunk, expirationTime);
    }

    //should be used? does it need to recover the old policy applied to the original message?
    public synchronized void addAdditionalChunk (String messageId, ChunkWrapper chunk, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        byte [] encryptedPayload = encryptData(ABEPolicyEquation, chunk.getData());
        if (encryptedPayload == null) {
            return;
        }
        chunk.setData(encryptedPayload);
        super.addAdditionalChunk (messageId, chunk);
    }

    public synchronized String addChunkedMessage (String groupName, String objectId, String instanceId,
                                                  String xmlMetadata, List<ChunkWrapper> chunks,
                                                  String dataMimeType, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        Map<Object, Object> newMetadata = standardizeMetadata(xmlMetadata);
        return addChunkedMessage (groupName, objectId, instanceId, newMetadata, chunks, dataMimeType, expirationTime, ABEPolicyEquation);
    }

    public synchronized String addChunkedMessage (String groupName, String objectId, String instanceId,
                                                  Map<Object, Object> metadata, List<ChunkWrapper> chunks,
                                                  String dataMimeType, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        
        //Map<Object, Object> newMetadata = standardizeMetadata(metadata);
        ArrayList<String> encMetadata = new ArrayList<String>();
        encMetadata.add(APP_METADATA);
        if (encryptMetadata(ABEPolicyEquation, metadata, encMetadata) < 0) {
            return null;
        }
        if (chunks == null) {
            return null;
        }
        Iterator<ChunkWrapper> iter = chunks.iterator();
        while (iter.hasNext()) {
            ChunkWrapper chunk = iter.next();
            byte [] encryptedPayload = encryptData(ABEPolicyEquation, chunk.getData());
            if (encryptedPayload == null) {
                return null;
            }
            chunk.setData(encryptedPayload);
        }
        return super.addChunkedMessage (groupName, objectId, instanceId, metadata, chunks, dataMimeType, expirationTime);
    }

    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   String xmlMetadata, byte[] data, String dataMimeType,
                                                   long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        Map<Object,Object> newMetadata =  standardizeMetadata(xmlMetadata);
        return chunkAndAddMessage(groupName, objectId, instanceId, newMetadata, data, dataMimeType, expirationTime, ABEPolicyEquation);
    }

    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   String[] metaDataAttributes, String[] metaDataValues,
                                                   byte[] data, String dataMimeType,
                                                   long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        Map<Object,Object> newMetadata =  standardizeMetadata(metaDataAttributes, metaDataValues);
        return chunkAndAddMessage (groupName, objectId, instanceId, newMetadata, data, dataMimeType, expirationTime, ABEPolicyEquation);
    }

    public synchronized String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                                   Map<Object, Object> metadata, byte[] data, String dataMimeType,
                                                   long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {

        //CHUNKING WITH ABE NOT AVAILABLE YET!!!
        //use local chunk fragmenter only
        if (_mimeTypeToFragmenter.containsKey(dataMimeType)) {
            List<ChunkWrapper> chunks = _mimeTypeToFragmenter.get(dataMimeType).fragment(data, dataMimeType, (byte) 8, (byte) 100);
            return addChunkedMessage(groupName, objectId, instanceId, metadata, chunks, dataMimeType, expirationTime);
        }
        return addMessage (groupName, objectId, instanceId, metadata, data, expirationTime, ABEPolicyEquation);
    }

    public synchronized String disseminateMessage (String groupName, String objectId, String instanceId,
                                      byte[] data, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        byte [] encryptedData = encryptData(ABEPolicyEquation, data);
        if (encryptedData == null) {
            return null;
        }
        return super.disseminateMessage (groupName, objectId, instanceId, encryptedData, expirationTime);
    }
    public synchronized String disseminateMessageMetadata (String groupName, String objectId, String instanceId, String metadata,
                                              byte[] data, String mimeType, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        //not implemented yet
        return null;
    }

    public synchronized String disseminateMessageMetadata (String groupName, String objectId, String instanceId, byte[] metadata,
                                              byte[] data, String mimeType, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        //not implemented yet
        return null;
    }

    public synchronized String disseminateMessageMetadata (String groupName, String objectId, String instanceId, Map<Object,Object> metadata,
                                              byte[] data, String mimeType, long expirationTime, DisjunctionEquation ABEPolicyEquation)
        throws CommException
    {
        //not implemented yet
        return null;
    }

    //callbacks

    @Override
    public boolean dataArrived (String id, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId, String mimeType,
                                byte[] data, short chunkNumber, short totChunksNumber,
                                String queryId) 
    {

        //decrypt
        byte [] plaitext = serviceABEDecrypt(data);
        if (plaitext == null){
            return undecryptableDataArrived (_undecryptableMessageListeners ,id, groupName, objectId, instanceId, annotatedObjMsgId, mimeType, data, chunkNumber, totChunksNumber, queryId);
        }
        return dataArrived (_listenerPro ,id, groupName, objectId, instanceId, annotatedObjMsgId, mimeType, plaitext, chunkNumber, totChunksNumber, queryId);
    }

    private boolean dataArrived (Collection<DSProProxyListener> listeners,
                                       String id, String groupName, String objectId,
                                       String instanceId, String annotatedObjMsgId, String mimeType,
                                       byte[] data, short chunkNumber, short totChunksNumber, String queryId)

    {
        if (id == null || data == null || data.length == 0) {
            throw new NullPointerException("Null paramenters");
        }
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                _logger.warn("dataArrived: No listeners have been registered");
                return false;
            }

            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.dataArrived(id, groupName, objectId, instanceId,
                                         annotatedObjMsgId, mimeType, data,
                                         chunkNumber, totChunksNumber, queryId)) {
                    returnValue = true;
                }
            }

            return returnValue;
        }
    }

    @Override
    public boolean metadataArrived (String id, String groupName, String objectId, String instanceId,
                                    String jsonMetadata, String referredDataId, String queryId)
    {
        //decrypt
        String newJsonMetadata = decryptMetadata(jsonMetadata);
        if(newJsonMetadata == null){
            return undecryptableMetadataArrived (_undecryptableMessageListeners, id, groupName, objectId, instanceId,
                    jsonMetadata, referredDataId, queryId);
        }
        return metadataArrived (_listenerPro, id, groupName, objectId, instanceId, newJsonMetadata, referredDataId, queryId);
    }

    private boolean metadataArrived (Collection<DSProProxyListener> listeners, String id,
                                     String groupName, String objectId, String instanceId, String jsonMetadata,
                                     String referredDataId, String queryId)
    {
        if (id == null || jsonMetadata == null || jsonMetadata.length() == 0 || referredDataId == null) {
            throw new NullPointerException("Null paramenters");
        }

        synchronized (listeners) {
            if (listeners.isEmpty()) {
                _logger.warn("metadataArrived: No listeners have been registered");
                return false;
            }
            boolean returnValue = false;
            for (DSProProxyListener listener : listeners) {
                if (listener.metadataArrived(id, groupName, objectId, instanceId, jsonMetadata,
                        referredDataId, queryId)) {
                    returnValue = true;
                }
            }
            return returnValue;
        }
    }

    @Override
    public boolean dataAvailable (String id, String groupName, String objectId, String instanceId,
                                  String referredDataId, String mimeType, byte[] metadata, String queryId)
    {
        // TODO: fix this
        throw new UnsupportedOperationException("dataAvailable not yet supported by ABEDSProProxy");
    }

    private boolean undecryptableDataArrived (Collection<UndecryptableMessageListener> listeners,
                                       String id, String groupName, String objectId,
                                       String instanceId, String annotatedObjMsgId, String mimeType,
                                       byte[] data, short chunkNumber, short totChunksNumber, String queryId) 
    {
        if (id == null || data == null || data.length == 0) {
            throw new NullPointerException("Null paramenters");
        }
        synchronized (listeners) {
            if (listeners.isEmpty()) {
                _logger.info("undecryptableDataArrived: No listeners have been registered");
                //if there are no listener of encrypted messages dataArrived will return success
                return true;
            }

            boolean returnValue = false;
            for (UndecryptableMessageListener listener : listeners) {
                if (listener.undecryptableDataArrived(id, groupName, objectId, instanceId,
                                         annotatedObjMsgId, mimeType, data,
                                         chunkNumber, totChunksNumber, queryId)) {
                    returnValue = true;
                }
            }

            return returnValue;
        }
    }
    private boolean undecryptableMetadataArrived (Collection<UndecryptableMessageListener> listeners,
                                           String id, String groupName, String objectId,
                                           String instanceId, String jsonMetadata,
                                           String referredDataId, String queryId)
    {
        if (id == null || jsonMetadata == null || jsonMetadata.length() == 0 || referredDataId == null) {
            throw new NullPointerException("Null paramenters");
        }

        synchronized (listeners) {
            if (listeners.isEmpty()) {
                _logger.info("undecryptableMetadataArrived: No listeners have been registered");
                //if there are no listener of encrypted messages metadataArrived will return success
                return true;
            }
            boolean returnValue = false;
            for (UndecryptableMessageListener listener : listeners) {
                if (listener.undecryptableMetadataArrived(id, groupName, objectId, instanceId,
                        jsonMetadata, referredDataId, queryId)) {
                    returnValue = true;
                }
            }
            return returnValue;
        }
    }
    @Override
    public synchronized DataWrapper getData (String messageID)
            throws CommException
    {
        return getData(messageID, null);
    }

    @Override
        public synchronized DataWrapper getData (String messageID, String callbackParameters)
            throws CommException
    {
        DataWrapper data = super.getData(messageID, callbackParameters);
        if (data != null) {
            byte [] payload = serviceABEDecrypt(data._data);
            if(payload == null){
                return null;
            }
            return new DataWrapper(payload, data._hasMoreChunks, data._queryId);
        }
        return null;
    }

    //rpc--------------
    
    private int serviceABELogin() {
        LoginRequest loginRequest = LoginRequest.newBuilder().build();
        LoginResponse loginResponse = null;
        try {        
            loginResponse = _blockingStub.login(loginRequest);
            _session = loginResponse.getSession();            
            if (loginResponse.getSession().getKey().size() == 0){
                _logger.error("serviceABELogin:Session uninitialized");
                return -1;
            }
        }
        catch (StatusRuntimeException e){
            _logger.error("serviceABELogin:RPC Login failed: {}", e.getStatus());
            return -1;
        }
        return 0;        
    }
    private int serviceADDAttributes(Iterable <AttributeJava> attributeIterable) {
        KnownAttributesRequest.Builder addAttributesRequestBuilder = KnownAttributesRequest
                .newBuilder()
                .setSession(_session)
                .setAction(Action.ADD);
        Iterator <AttributeJava> iterAttr = attributeIterable.iterator();
        while (iterAttr.hasNext()) {
            Iterator <AttributeVersionJava> iterVer = iterAttr.next().versions.iterator();
            while (iterVer.hasNext()) {
                addAttributesRequestBuilder.addAttributes(iterVer.next().toProto());
            }
        }
        KnownAttributesRequest addAttributesRequest = addAttributesRequestBuilder.build();
        KnownAttributesResponse addAttributesResponse = null;
        try {
            addAttributesResponse = _blockingStub.knownAttributes(addAttributesRequest);
        } catch (StatusRuntimeException e){
            if (e.getStatus().getCode() == Status.Code.UNAUTHENTICATED) {
                if(reconnectToService() == 0) {
                    _logger.info("serviceADDAttributes: Service probably restarted, reconnection and reinitialization performed");
                    return serviceADDAttributes(attributeIterable);
                }
                _logger.error("serviceADDAttributes: reconnect to service failed");
                return -1;
            }
            _logger.error("serviceADDAttributes: failure {}", e.getStatus());
            return -1;
        }
        return 0;
    }


    private byte [] serviceABEDecrypt (byte [] data) {
        if (data == null) {
            _logger.error ("serviceABEDecrypt: data is null");
        }
        ABEMessage abeMessage;
        try{
            abeMessage = ABEMessage.parseFrom(data);
        }
        catch(InvalidProtocolBufferException e){
            _logger.info("serviceABEDecrypt: invalid protobuf message received {}",e.getMessage());
            return null;
        }
         if(!abeMessage.hasDataMsg()) {
            _logger.info("serviceABEDecrypt: not supported abeMessage type");
            return null;
        }
        else {
            return decryptABEDataMessage(abeMessage.getDataMsg());
        }
    } 
    
    private byte [] decryptABEDataMessage(ABEDataMessage dataMessage) {
        DecryptResponse response = decrypt(dataMessage.getEquation(), dataMessage.getCyphertext().toByteArray(), dataMessage.getIv().toByteArray());
        if(response == null){
            return null;
        }
        return response.getPlainMessage().toByteArray();
    }
    private DecryptResponse decrypt(DisjunctionEquation ABEPolicyEquation, byte[] cyphertext, byte [] iv){
        if((ABEPolicyEquation == null) || (cyphertext == null) || (iv == null)) {
            _logger.warn("decrypt: error parameters null");
            return null;
        }
        DecryptRequest decryptRequest = DecryptRequest
                .newBuilder()
                .setSession(_session)
                .setEquation(ABEPolicyEquation)
                .setCryptoMessage(ByteString.copyFrom(cyphertext))
                .setIv(ByteString.copyFrom(iv))
                .build();
        DecryptResponse decryptResponse;
        try {
            decryptResponse = _blockingStub.decrypt(decryptRequest);
        }
        catch (StatusRuntimeException e){
            if (e.getStatus().getCode() == Status.Code.UNAUTHENTICATED) {
                if(reconnectToService() == 0) {
                    _logger.info("decrypt: Service probably restarted, reconnection and reinitialization performed");
                    return decrypt(ABEPolicyEquation, cyphertext, iv);
                }
            }
            _logger.error("decrypt: RPC Decrypt failed: {}", e.getStatus());
            return null;
        }
        if (decryptResponse.getPlainMessage().isEmpty()) {
        	_logger.info("decrypt: Received undecryptable message");
            return null;
        }
        return decryptResponse;
    }

    //refactoring needed
    private String decryptMetadata (String xmlMetadata) {

        JsonObject metadata = new JsonParser().parse(xmlMetadata).getAsJsonObject();
        String abeEncryptedMetadataVal = metadata.get(ABE_ENCRYPTED_METADATA).getAsString();
        if (abeEncryptedMetadataVal == null) {
            //not an ABE message should be impossible to reach this statement
            return xmlMetadata;
        }
        if (abeEncryptedMetadataVal.equals("NONE")) {
            //metadata are not encrytped
            try{
                DisjunctionEquation ABEPolicyEquation = DisjunctionEquation.parseFrom(b64Decode(metadata.get(ABE_EQUATION).getAsString()));
                metadata.remove(ABE_EQUATION);
                if(isDecryptable(ABEPolicyEquation)) {
                    return xmlMetadata;    
                }
                _logger.info ("decryptMetadata: metadata are plain but the client does not match the policies");
                return null;
            }catch (InvalidProtocolBufferException e){
                _logger.error("decryptMetadata: unable to correctly parse ABE equation");
                return null;
            }  
            
        }       
        String res;
        String [] metadataEncryptedFields = abeEncryptedMetadataVal.split(";");
        try{
            DisjunctionEquation ABEPolicyEquation = DisjunctionEquation.parseFrom(b64Decode(metadata.get(ABE_EQUATION).getAsString()));
            byte [] iv = b64Decode(metadata.get(ABE_IV).getAsString());
            for (String metadataEncryptedField : metadataEncryptedFields) {
                if(metadataEncryptedField.equals("")) {
                    continue;
                }
                else if (metadata.has(metadataEncryptedField)){
                    DecryptResponse response = decrypt(ABEPolicyEquation, b64Decode(metadata.get(metadataEncryptedField).getAsString()), iv);
                    if(response == null) {
                        //metadata.remove(metadataEncryptedField);
                        return null;
                    }
                    metadata.remove(metadataEncryptedField);
                    metadata.addProperty(metadataEncryptedField, response.getPlainMessage().toStringUtf8());
                }
            }
            metadata.remove(ABE_IV);
            metadata.remove(ABE_EQUATION);
            metadata.remove(ABE_ENCRYPTED_METADATA);
            return metadata.toString();
        }catch (InvalidProtocolBufferException e){
            _logger.error("decryptMetadata: unable to correctly parse ABE equation");
            return null;
        }        
    }
    private EncryptResponse encrypt (ByteString key, ByteString iv, byte [] data) {
        if (key == null || iv == null || data == null) {
            _logger.error ("encrypt: key or iv or data is null");
            return null;
        }
        EncryptRequest encryptRequest = EncryptRequest
                .newBuilder()
                .setSession(_session)
                .setSymmetricKey(key)
                .setIv(iv)
                .setPlainMessage(ByteString.copyFrom(data))
                .build();
        EncryptResponse encryptResponse = null;
        try {
            encryptResponse = _blockingStub.encrypt(encryptRequest);
        } catch (StatusRuntimeException e){
            if (e.getStatus().getCode() == Status.Code.UNAUTHENTICATED) {
                if(reconnectToService() == 0) {
                    _logger.info("encrypt: Service probably restarted, reconnection and reinitialization performed");
                    return encrypt(key,iv, data);
                }
            } 
            _logger.error("encrypt: RPC Encrypt failed: {}", e.getStatus());
            return null;
        }
        if (encryptResponse.getCryptoMessage().isEmpty()) {
            _logger.info("encrypt: encryption payload null");
            return null;
        }
        return encryptResponse;
    }

    //standard encryption 
    private EncryptResponse encrypt (DisjunctionEquation ABEPolicyEquation, byte [] data){
        if (data == null) {
            _logger.error ("encrypt: data is null");
            return null;
        }
        if (ABEPolicyEquation == null) {
            ABEPolicyEquation = DisjunctionEquation.newBuilder().build();
        }
        EncryptRequest encryptRequest = EncryptRequest
                .newBuilder()
                .setSession(_session)
                .setEquation(ABEPolicyEquation)
                .setPlainMessage(ByteString.copyFrom(data))
                .build();
        EncryptResponse encryptResponse = null;
        try {
            encryptResponse = _blockingStub.encrypt(encryptRequest);
        } catch (StatusRuntimeException e){
            if (e.getStatus().getCode() == Status.Code.UNAUTHENTICATED) {
                if(reconnectToService() == 0) {
                    _logger.info("encrypt: Service probably restarted, reconnection and reinitialization performed");
                    return encrypt(ABEPolicyEquation, data);
                }
            } 
            _logger.error("encrypt: RPC Encrypt failed: {}", e.getStatus());
            return null;
        }
        if (encryptResponse.getMissingAttributesList().size() > 0) {
            _logger.info ("encrypt: missing some attributes for the specified equation");
            return null;
        }
        if (encryptResponse.getCryptoMessage().isEmpty()) {
        	_logger.info ("encrypt: encryption payload null");
            return null;
        }
        return encryptResponse;
    }

    private byte [] encryptData (DisjunctionEquation ABEPolicyEquation, byte [] data) {
        byte [] encryptedPayload = null;
        EncryptResponse response = encrypt (ABEPolicyEquation, data);
        if(response == null) {
            _logger.error ("encryptData: encrypt response is null. encrypt failed");
            return null;
        }
        ABEDataMessage encryptedDataMessage = ABEDataMessage
                .newBuilder()
                .setEquation(response.getEquation())
                .setIv(response.getIv())
                .setCyphertext(response.getCryptoMessage())
                .build();
        ABEMessage abeMessage = ABEMessage
                .newBuilder()
                .setDataMsg(encryptedDataMessage)
                .build();
        
        return abeMessage.toByteArray();
    }

    private int encryptMetadata (DisjunctionEquation ABEPolicyEquation, Map<Object, Object> metadata, ArrayList<String> metadataFields) {
        if (metadata == null) {
            _logger.error ("encryptMetadata: metadata is null");
            return -1;
        }
        String encodeRes =  null;
        if (metadataFields == null) {
            metadataFields =  new ArrayList<String>();
        }
        ByteString key = null;
        ByteString iv = null;
        //Base64.Encoder encoder = Base64.getEncoder();
        String encryptedABEMetadataField = "";
        for (String metadataField : metadataFields) {
            if(!metadata.containsKey (metadataField)) {
                continue;
            }
            encryptedABEMetadataField += metadataField + ";";
            if (key == null) {
                EncryptResponse response = encrypt (ABEPolicyEquation, ByteString.copyFromUtf8((String) metadata.get (metadataField)).toByteArray());
                if(response == null) {
                    return -1;
                }
                key = response.getSymmetricKey();
                iv = response.getIv();


                encodeRes = b64Encode(response.getEquation().toByteArray());
                if (encodeRes == null) {
                    _logger.error ("encryptMetadata: failed to encode equation to base64");
                    return -1;
                }
                metadata.put(ABE_EQUATION, encodeRes);
                encodeRes = b64Encode(response.getIv().toByteArray());
                if (encodeRes == null) {
                    _logger.error ("encryptMetadata: failed to encode IV to base64");
                    return -1;
                }
                metadata.put(ABE_IV, encodeRes);
                encodeRes = b64Encode(response.getCryptoMessage().toByteArray());
                if (encodeRes == null) {
                    _logger.error ("encryptMetadata: failed to encode ciphertext to base64");
                    return -1;
                }
                metadata.replace(metadataField, encodeRes);
            }
            else {
                EncryptResponse response = encrypt (key, iv, ByteString.copyFromUtf8((String) metadata.get (metadataField)).toByteArray());
                if (response == null) {
                    return -1;
                }
                encodeRes = b64Encode(response.getCryptoMessage().toByteArray());
                if (encodeRes == null) {
                    _logger.error ("encryptMetadata: failed to encode ciphertext to base64");
                    return -1;
                }
                metadata.replace(metadataField, encodeRes);
            }
        }
        if (!metadata.containsKey(ABE_EQUATION)) {
            //bummy equation to block the callback even if metadata are plain but the dataMessage won't be
            encodeRes = b64Encode(ABEPolicyEquation.toByteArray());
            if(encodeRes == null) {
                _logger.error ("encryptMetadata: failed to encode equation to base64");
                return -1;
            }
            metadata.put(ABE_EQUATION, encodeRes);
        }
        if(encryptedABEMetadataField.equals("")) {
            encryptedABEMetadataField = "NONE";
        }
        metadata.put(ABE_ENCRYPTED_METADATA,encryptedABEMetadataField);
        return 0;

    }

    private Map<Object, Object> standardizeMetadata(Map <Object,Object> metadata){
        if (metadata == null) {
            return null;
        }
        Map<Object, Object> res = new HashMap<Object, Object>();
        Iterator it = metadata.entrySet().iterator();
        while(it.hasNext()){
            Map.Entry entry =(Map.Entry) it.next();
            res.put(entry.getKey(), entry.getValue());
        }
        return res;
    }

    private Map<Object, Object> standardizeMetadata(String jsonMetadata){
        try{
            JsonObject metadata = new JsonParser().parse(jsonMetadata).getAsJsonObject();
            Map<Object, Object> res = new HashMap<>();
            for (Map.Entry<String,JsonElement> entry : metadata.entrySet()) {
                res.put(entry.getKey(), entry.getValue().getAsString());
            }
            return res;
        }catch (Exception ex){
            _logger.error ("standardizeMetadata: " + ex.getMessage());
            return null;
        }
    }
    private Map<Object, Object> standardizeMetadata(String [] metadataFields, String [] metadataVals){
        if (metadataFields == null || metadataVals == null) {
            return null;
        }
        if(metadataFields.length != metadataVals.length) {
            return null;
        }
        Map<Object, Object> res = new HashMap<Object, Object>();
        for(int i = 0; i < metadataFields.length; i++) {
            res.put (metadataFields[i], metadataVals[i]);
        }
        return res;
    }
    
    
    private int staticInitialization(String file) throws Exception {
        File cfgFile = new File(file);
        if(!cfgFile.exists()){
            _logger.error("cfg file not found");
            return -1;
        }
        BufferedReader br = new BufferedReader(new FileReader(cfgFile));
        HashMap<String, String> cfg = new HashMap<String, String>();
        String line = null;
        while ((line = br.readLine()) != null) {
            String [] splits = line.split("=");
            if (splits.length != 2)
                continue;
            cfg.put(splits[0].replaceAll("\\s+",""),splits[1]);
        }
        br.close();
        //init attributes
        int i = 0;
        while (true) {
            AttributeJava a = new AttributeJava();
            if (cfg.containsKey("attribute."+i+".id")){
                a.id = Integer.parseInt(cfg.get("attribute."+i+".id"));
            }
            else{
                a = null;
                break;
            }
            if (cfg.containsKey("attribute."+i+".name")){
                a.name = cfg.get("attribute."+i+".name");
            }
            else{
                a = null;
                break;
            }
            int j = 0;
            AttributeVersionJava attVer;
            while(true){
                attVer =  new AttributeVersionJava();
                attVer.id = a.id;
                if (cfg.containsKey("attribute."+i+".version."+j+".id")){
                    attVer.version = Integer.parseInt(cfg.get("attribute."+i+".version."+j+".id"));
                }
                else{
                    attVer = null;
                    break;
                }
                if (cfg.containsKey("attribute."+i+".version."+j+".keyfile")){
                    try {
                        File attributeFile = new File(cfg.get("attribute."+i+".version."+j+".keyfile"));
                        byte[] key = new byte[(int)attributeFile.length()];
                        FileInputStream fis = new FileInputStream(attributeFile);
                        fis.read(key);
                        fis.close();
                        attVer.key = key;
                    } catch (Exception e) {
                        attVer = null;
                        break;
                    }
                }
                else{
                    attVer = null;
                    break;
                }
                a.versions.add(attVer);
                j++;
            }
            if(a.versions.size() > 0)
                _attributeMap.put(a.id,a);
            i++;
        }
        //loading attribute in the service
        if(_attributeMap.values().size() != 0){
            if(serviceADDAttributes(_attributeMap.values()) < 0) {
                _logger.error("staticInitialization: gRPC ABE Service add attributes failure");
                return -1;
            }
        }
        //init default equation
        if(cfg.containsKey("defaultEquation")) {
            _debugEquation = cfg.get("defaultEquation");
        }
        return 0;
    }
    
    private DisjunctionEquation stringToEquation(String equation){
        DisjunctionEquation.Builder eb = DisjunctionEquation.newBuilder();
        if((equation == null) || (equation.equals(""))){
            return eb.build();
        }
        String [] splits = equation.split(" OR ");
        for (String spl : splits) {
            String [] attrIds = spl.split(" AND ");            
            ConjunctionEquation.Builder cb = ConjunctionEquation.newBuilder();
            for (String id : attrIds) {
                cb.addAttributes(_attributeMap.get(Integer.parseInt(id)).toProto());
            }
            eb.addConjunctions(cb.build());
        }
        return eb.build();
    }

    public String printEquation() {
        return _debugEquation.toString();
    }
    public void setDefaultEquation(String equation) {
        _debugEquation = equation;
    }
    /*public String [] getAttributes() {
        ArrayList<String> res = new ArrayList<String>();
        for(AttributeJava a : _attributeMap.values()) {
            if ((a.name == null) || a.name.equals("")) {
                a.name = "attribute_id " + a.id;
            }
            res.add(a.name);
        }
        return res.toArray();
    }*/
    private boolean isDecryptable(DisjunctionEquation equation) {
        boolean decryptable = false;
        for(ConjunctionEquation ce : equation.getConjunctionsList()) {
            boolean conjDecrypFlag = true;
            for(Attribute a : ce.getAttributesList()) {
                if(!_attributeMap.containsKey(a.getId())) {
                    conjDecrypFlag = false;
                    break;
                } else if (!(_attributeMap.get(a.getId()).hasVersion(a.getVersion()))) {
                    conjDecrypFlag = false;
                    break;
                }
            }
            if(conjDecrypFlag) {
                decryptable = true;
                break;
            }
        }
        return decryptable;
    }

    private String b64Encode(byte [] input) {
        try {
            ByteArrayOutputStream outStream =  new ByteArrayOutputStream();
            Base64Encoder encoder = new Base64Encoder (new ByteArrayInputStream(input), outStream);
            encoder.processWithoutNewlines();
            return (outStream.toString("ISO-8859-1"));
        }catch (IOException ioe) {
            _logger.error ("b64Encode: "+ioe.getMessage());
            return null;
        }
    }
    private byte [] b64Decode(String input) {
        try {
            Base64Decoder decoder = new Base64Decoder (input);
            decoder.process();
            return ((ByteArrayOutputStream)decoder.out).toByteArray();
        }catch (Base64FormatException b64fe) {
            _logger.error ("b64Decode: "+b64fe.getMessage());
            return null;
        }catch (IOException ioe) {
            _logger.error ("b64Decode: "+ioe.getMessage());
            return null;
        }
    }

    //this method returns the hasmap representing the attributes
    public HashMap<Integer, AttributeJava> getAttributes() {
        return _attributeMap;
    }

    //this method is made for android devices in case the kernel stop and restart the service
    public synchronized int reconnectToService() {
        try {
            _channel.shutdownNow();
            if(! _channel.awaitTermination(10, TimeUnit.SECONDS)) {
                _logger.error("reconnectToService: channel termination failure. no safe rollback");
                return -1;
            }
        }
        catch (InterruptedException ex) {
            if(! _channel.isTerminated()) {
                _logger.error("reconnectToService: channel termination failed. no safe rollback");
                return -1;
            }
        }
        if (initBlockingStub() < 0){
            _logger.error("reconnectToService: gRPC ABE Service init blocking stub");
            return -1;
        }
        if (serviceABELogin() < 0){
            _logger.error("reconnectToService: gRPC ABE Service Login failure");
            return -1;
        }
        if (serviceADDAttributes(_attributeMap.values()) < 0) {
            _logger.error("reconnectToService: gRPC ABE Service add attributes failure");
            return -1;
        }
        return 0;        
        
    }


    
    // internal classes
    public class AttributeVersionJava {
        public int id;
        public int version;
        public byte[] key;
        public AttributeVersionJava(){
            key = null;
        }
        public boolean equals(AttributeVersionJava a) {
            return (this.version == a.version);
        }
        public AttributeVersionJava (Attribute attProto){
            id = attProto.getId();
            version = attProto.getVersion();
            key = attProto.getKey().toByteArray();
        }
        public Attribute toProto(){
            return Attribute
                    .newBuilder()
                    .setId(id)
                    .setVersion(version)
                    .setKey(ByteString.copyFrom(key))
                    .build();
       }
    }

    public class AttributeJava {
        public AttributeJava() {
            versions = new ArrayList<AttributeVersionJava>();
        }
        public AttributeJava(int id, String name) {
            this.id = id;
            this.name = name;
            versions = new ArrayList<AttributeVersionJava>();
        }
        public Attribute toProto() {
            return getLastVersion().toProto();
        }
        public Attribute toProto(int index) {
            if (index > (versions.size() - 1))
                return null;
            return versions.get (index).toProto();
        }
        public void updateVersion (AttributeVersionJava attVer) {
            for(AttributeVersionJava a : versions) {
                if(a.version == attVer.version) {
                    a.key = attVer.key;
                }
            }
        }
        public boolean hasVersion(int version) {
            for(AttributeVersionJava a : versions) {
                if(a.version == version) {
                    return true;
                }
            }
            return false;
        }
        public boolean hasVersion(AttributeVersionJava attVer) {
            for(AttributeVersionJava a : versions) {
                if(a.version == attVer.version) {
                    return true;
                }
            }
            return false;
        }
        public AttributeVersionJava getLastVersion() {
            //wrong
            return versions.get(versions.size() - 1);
        }
        public boolean equals(AttributeJava a) {
            return (this.id == a.id);
        }
        public int id;
        public String name;
        public ArrayList<AttributeVersionJava> versions; 
    }
}
