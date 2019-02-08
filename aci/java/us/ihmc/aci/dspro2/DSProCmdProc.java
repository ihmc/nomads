package us.ihmc.aci.dspro2;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import us.ihmc.chunking.AnnotationWrapper;
import us.ihmc.chunking.ChunkWrapper;
import us.ihmc.chunking.Fragmenter;
import us.ihmc.chunking.Reassembler;

import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.aci.util.dspro.Waypoint;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DSProCmdProc implements DSProProxyListener, SearchListener, Fragmenter, Reassembler, Runnable {

    private static final String TEST_CHUNKING_MIME_TYPE = "application/chunkingplugintest";
    private final DSProProxyInterface _proxy;
    private final int _iPathNumber;

    DSProCmdProc (DSProProxyInterface proxy, String localUser, String recepient)
    {
        _proxy = proxy;
        _iPathNumber = 0;
    }

    @Override
    public void run()
    {
        Console console = System.console();
        for (String input; (input = console.readLine("DSPro>")) != null;) {
            String command = null;
            try {
                String[] tokens = input.split("\\s+");
                command = tokens[0];
                if (0 == tokens[0].compareToIgnoreCase("quit")) {
                    break;
                }
                if ((0 == tokens[0].compareToIgnoreCase("getNodeContext")) || (0 == tokens[0].compareToIgnoreCase("getNodeCtxt"))) {
                    String nodeId = tokens.length > 1 ? tokens[1] : null;
                    String ctxt = _proxy.getNodeContext(nodeId);
                    System.out.println("Node Context:\n" + ctxt + "\n");
                }
                if (0 == tokens[0].compareToIgnoreCase("getNodeId")) {
                    String nodeId = _proxy.getNodeId();
                    System.out.println("Node id " + nodeId);
                }
                if (0 == tokens[0].compareToIgnoreCase("setMissionId")) {
                    String missionId = tokens[1];
                    _proxy.setMissionId(missionId);
                    System.out.println("Added mission id " + missionId);
                }
                else if (0 == tokens[0].compareToIgnoreCase("setRole")) {
                    String role = tokens[1];
                    _proxy.setRole(role);
                    System.out.println("Added role " + role);
                }
                else if (0 == tokens[0].compareToIgnoreCase("setNodeType")) {
                    String type = tokens[1];
                    _proxy.setNodeType(type);
                    System.out.println("Added type " + type);
                }
                else if (0 == tokens[0].compareToIgnoreCase("setUsefulDistance")) {
                    if (tokens[1].matches("^-?\\d+$")) {
                        // default useful distance
                        _proxy.setDefaultUsefulDistance(Integer.parseInt(tokens[1]));
                        System.out.println("set default useful distance " + tokens[1]);
                    }
                    else {
                        _proxy.setUsefulDistance(tokens[1], Integer.parseInt(tokens[2]));
                        System.out.println("set useful distance for " + tokens[1] + ": " + tokens[2]);
                    }
                }
                else if (0 == tokens[0].compareToIgnoreCase("setRangeOfInfluence")) {
                    _proxy.setRangeOfInfluence (tokens[1], Integer.parseInt(tokens[2]));
                    System.out.println("set range of influence for " + tokens[1] + ": " + tokens[2]);
                }
                else if ((0 == tokens[0].compareToIgnoreCase("addMessage")) || (0 == tokens[0].compareToIgnoreCase("chunkMessage"))) {
                    String groupName = "test";
                    String objectId = tokens[1].trim();
                    String instanceId = tokens[2].trim(); 
                    byte[] data = new byte[Integer.parseInt(tokens[3].trim())];
                    for (int i = 0; i < data.length; i++) {
                        data[i] = (byte) (i % 10);
                    }
                    long expirationTime = 0;
                    Map<Object, Object> metadata = new HashMap<Object, Object>();
                    for (int i = 4; i < (tokens.length - 1); i++) {
                        String attributeName = tokens[i];
                        String attributeValue = tokens[++i];
                        metadata.put (attributeName, attributeValue);
                    }

                    String dsproId;
                    if (0 == tokens[0].compareToIgnoreCase("chunkMessage")) {
                        if (!metadata.containsKey(MetadataElement.dataFormat.toString())) {
                            metadata.put(MetadataElement.dataFormat.toString(), TEST_CHUNKING_MIME_TYPE);
                        }
                        dsproId = _proxy.chunkAndAddMessage (groupName, objectId, instanceId, 
                                                             metadata, data, TEST_CHUNKING_MIME_TYPE,
                                                             expirationTime);
                    }
                    else {
                        dsproId = _proxy.addMessage (groupName, objectId, instanceId, 
                                                     metadata, data, expirationTime);
                    }

                    System.out.println("Added message with id " + dsproId);
                }
                else if (0 == tokens[0].compareToIgnoreCase("addChunk")) {
                    String groupName = "test";
                    String objectId = tokens[1].trim();
                    String instanceId = tokens[2].trim();
                    byte[] data = new byte[Integer.parseInt(tokens[3].trim())];
                    for (int i = 0; i < data.length; i++) {
                        data[i] = (byte) (i % 10);
                    }
                    long expirationTime = 0;
                    Map<Object, Object> metadata = new HashMap<Object, Object>();
                    for (int i = 4; i < (tokens.length - 1); i++) {
                        String attributeName = tokens[i];
                        String attributeValue = tokens[++i];
                        metadata.put (attributeName, attributeValue);
                    }
                    ChunkWrapper wr = new ChunkWrapper(data, (byte) 1, (byte) 4, TEST_CHUNKING_MIME_TYPE);
                    String dsproId = _proxy.addChunk(groupName, objectId, instanceId, metadata, wr, 0);

                    System.out.println("Added chunk message with id " + dsproId);
                }
                else if (0 == tokens[0].compareToIgnoreCase("addAdditionalChunk")) {
                    String id = tokens[1].trim();
                    byte[] data = new byte[Integer.parseInt(tokens[2].trim())];
                    for (int i = 0; i < data.length; i++) {
                        data[i] = (byte) (i % 10);
                    }
                    byte chunkId = Byte.parseByte(tokens[3].trim());
                    ChunkWrapper wr = new ChunkWrapper(data, chunkId, (byte) 4, TEST_CHUNKING_MIME_TYPE);
                    _proxy.addAdditionalChunk(id, wr);

                    System.out.println("Added additional chunk with id " + id);
                }
                else if ((0 == tokens[0].compareToIgnoreCase("disseminateMessage"))
                        || (0 == tokens[0].compareToIgnoreCase("dissMessage"))
                        || (0 == tokens[0].compareToIgnoreCase("dissMsg"))){
                    String groupName = "test";
                    String objectId = tokens[1].trim();
                    String instanceId = tokens[2].trim(); 
                    Message msg = getData(tokens[3].trim());
                    long expirationTime = 0;
                    String dsproId = _proxy.disseminateMessage (groupName, objectId, instanceId, 
                                                                msg.data, expirationTime);
                    System.out.println("Disseminated message with id " + dsproId);
                }
                else if ((0 == tokens[0].compareToIgnoreCase("disseminateMessageMetadata"))
                        || (0 == tokens[0].compareToIgnoreCase("dissMessageMeta"))
                        || (0 == tokens[0].compareToIgnoreCase("dissMsgMeta"))){
                    String groupName = "test";
                    String objectId = tokens[1].trim();
                    String instanceId = tokens[2].trim();
                    Message msg = getData(tokens[3]);
                    long expirationTime = 0;
                    String dsproId = _proxy.disseminateMessageMetadata (groupName, objectId, instanceId,
                            msg.metadata, msg.data, "", expirationTime);
                    System.out.println("Disseminated message with id " + dsproId);
                }
                else if (0 == tokens[0].compareToIgnoreCase("changekey")){
                    KeyGenerator keyGen;
                    try {
                      keyGen = KeyGenerator.getInstance("AES");
                      keyGen.init(128);
                      SecretKey key = keyGen.generateKey();
                      String encodedKey = (new us.ihmc.util.Base64Encoder (new String(key.getEncoded()))).processString();
                      _proxy.changeEncryptionKey(encodedKey.getBytes());
                      System.out.println("Changed encryption key to " + encodedKey);
                    }
                    catch (Exception e) {
                      e.printStackTrace();
                    }
                }
                else if (0 == tokens[0].compareToIgnoreCase ("addMessageFromFile")) {
                    Properties fileMetadata = new Properties();
                    InputStream is = null;
                    try {
                        is = new FileInputStream (tokens[1].trim());
                        fileMetadata.load (is);
                    }
                    finally {
                        is.close();
                    }
                    String groupName = "test";
                    String objectId = UUID.randomUUID().toString();
                    String instanceId = UUID.randomUUID().toString();
                    long expirationTime = 0;
                    Map<Object, Object> metadata = new HashMap<Object, Object>();
                    for (String attributeName : fileMetadata.stringPropertyNames()) {
                        String attributeValue = fileMetadata.getProperty (attributeName);
                        if (attributeName.equals (MetadataElement.referredDataObjectId.name())) {
                            objectId = attributeValue;
                        }
                        else if (attributeName.equals (MetadataElement.referredDataInstanceId.name())) {
                            instanceId = attributeValue;
                        }
                        else if (attributeName.equals (MetadataElement.expirationTime.name())) {
                            try {
                                expirationTime = Long.parseLong (attributeValue);
                            }
                            catch (NumberFormatException e) {
                                System.out.println ("Impossible to read the expiration time from the file");
                                e.printStackTrace();
                            }
                        }
                        metadata.put (attributeName, attributeValue);
                    }
                    String dsproId = _proxy.addMessage (groupName, objectId, instanceId,
                            metadata, null, expirationTime);
                    System.out.println ("Added message from file with id " + dsproId);
                }
                else if (0 == tokens[0].compareToIgnoreCase("search")) {
                    String groupName = "test";
                    String queryType = tokens[1];
                    String queryQualifiers = tokens[2];
                    String query = "";
                    for (int i = 3; i < tokens.length; i++) {
                        query += tokens[i] + " ";
                    }
                    query = query.trim();
                    String searchId = _proxy.search (groupName, queryType, queryQualifiers,
                                                     query.getBytes());
                    System.out.println("Searched message. Search Id: " + searchId);
                }
                else if ((0 == tokens[0].compareToIgnoreCase("replyToSearch")) || (0 == tokens[0].compareToIgnoreCase("replyToQuery"))) {
                    String searchId = tokens[1];
                    Collection<String> ids = new LinkedList<String>();
                    for (int i = 2; i < tokens.length; i++) {
                        ids.add(tokens[i]);
                    }
                    _proxy.replyToSearch (searchId, ids);
                    System.out.println("Replied to search with id: " + searchId);
                }
                else if ((0 == tokens[0].compareToIgnoreCase("vReplyToSearch")) || (0 == tokens[0].compareToIgnoreCase("vReplyToQuery"))) {
                    String searchId = tokens[1];
                    _proxy.replyToQuery (searchId, tokens[2].getBytes(StandardCharsets.UTF_8));
                    System.out.println("Replied to search with id: " + searchId);
                }
                else if (0 == tokens[0].compareToIgnoreCase("getData")) {
                    String dsproId = tokens[1];
                    String callbackParameters = null;
                    if (tokens.length > 2) {
                        callbackParameters = tokens[2];
                    }
                    DSProProxy.DataWrapper wrapper = _proxy.getData (dsproId, callbackParameters);
                    System.out.println ("data " + dsproId + "requested with query id: <" + callbackParameters + ">");
                    if (wrapper != null && wrapper._data != null) {
                        System.out.println ("data " + dsproId + "received");
                    }
                }
                else if (0 == tokens[0].compareToIgnoreCase("requestchunks")) {
                    for (int i = 0; i < tokens.length; i++) {
                        System.out.println("[" + i + "] = " + tokens[i]);
                    }
                    long timeout = 0;
                    String chunkedMsgId = tokens[1];
                    if (tokens.length > 2) {
                        String mimeType = tokens[2];
                        long ui32StartXPixel, ui32EndXPixel, ui32StartYPixel, ui32EndYPixel, ui32StartTPixel, ui32EndTPixel;
                        ui32StartXPixel = ui32EndXPixel = ui32StartYPixel = ui32EndYPixel = ui32StartTPixel = ui32EndTPixel = 0;
                        for (int i = 3; i < tokens.length; i += 1) {
                            System.out.println("AAA therre are " + tokens.length  + " tokens. Reading: " + i);
                            switch (us.ihmc.aci.util.chunking.Dimension.valueOf(tokens[i].toUpperCase())) {
                                case X:
                                    ui32StartXPixel = Long.parseLong(tokens[++i]);
                                    ui32EndXPixel = Long.parseLong(tokens[++i]);
                                    break;
                                case Y:
                                    System.out.println("y: " + tokens[i+1] + " " + tokens[i+2]);
                                    ui32StartYPixel = Long.parseLong(tokens[++i]);
                                    ui32EndYPixel = Long.parseLong(tokens[++i]);
                                    break;
                                case T:
                                    ui32StartTPixel = Long.parseLong(tokens[++i]);
                                    ui32EndTPixel = Long.parseLong(tokens[++i]);
                                    break;
                            }
                        }
                        _proxy.requestCustomAreaChunk(chunkedMsgId, mimeType, ui32StartXPixel, ui32EndXPixel, ui32StartYPixel, ui32EndYPixel, (byte)100, timeout);
                    }
                    else {
                        String callbackParameters = "cbackparameters";
                        _proxy.requestMoreChunks(chunkedMsgId, callbackParameters);
                    }
                    System.out.println ("requested chunk");
                }
                else if (0 == tokens[0].compareToIgnoreCase("submitpath")) {
                    final String pathName = "TestPath-" + _iPathNumber;
                    NodePath path = new NodePath(pathName, (short)1, 1.0f);
                    path.appendWayPoint(1, 2, 0, System.currentTimeMillis(), null, null);
                    path.appendWayPoint(1, 2, 0, System.currentTimeMillis() + 100 , null, null);
                    path.appendWayPoint(1, 2, 0, System.currentTimeMillis() + 200 , null, null);
                    //_iPathNumber++;
                    _proxy.registerPath(path);
                    _proxy.setCurrentPath(pathName);
                    System.out.println ("submitted path " + pathName);
                }
                else if (0 == tokens[0].compareToIgnoreCase("cancelobj")) {
                    String objectId = tokens[1];
                    String instanceId = null;
                    if (tokens.length > 2) {
                        instanceId = tokens[2];
                    }
                    _proxy.cancel (objectId, instanceId);
                    System.out.println ("Cancelled " + objectId + " " + (instanceId == null ? "" : instanceId));
                }
                else {
                    System.out.println("wieds");
                }
            }
            catch (Exception e) {
                String msg = e.getMessage();
                if (msg == null) {
                    msg = ".";
                }
                System.err.println("Could not execute command: <" + command + "> (" + input + ")"+ msg);
                e.printStackTrace();
            }
        }
        System.out.println("Quitting");
    }

    @Override
    public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission)
    {
        System.out.println ("Path arrived for node " + nodeId + ":");
        System.out.println ("PathId: " + path.getPathId());
        System.out.println ("Path Prob: " + path.getPathProbability());
        for (Waypoint w : path.getWaypoints()) {
            System.out.println (w.getLat() + " "+ w.getLon());
        }
        return true;
    }

    @Override
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        return true;
    }

    @Override
    public void newNeighbor (String peerID)
    {
        System.out.println ("newNeighbor: " + peerID);
    }

    @Override
    public void deadNeighbor (String peerID)
    {
        System.out.println ("deadNeighbor: " + peerID);
    }

    @Override
    public boolean dataArrived (String id, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId,
                                String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String queryId)
    {
        System.out.println (getLogMsg (objectId, instanceId, id, queryId, data));

        return true;
    }

    @Override
    public boolean metadataArrived (String id, String groupName, String referredDataObjectId,
                                    String referredDataInstanceId, String jsonMetadata,
                                    String referredDataId, String queryId)
    {
        StringBuilder msg = new StringBuilder ("metadataArrived: ")
                .append("groupName ").append(groupName).append(" ")
                .append("referredDataObjectId ").append(referredDataObjectId).append(" ")
                .append("referredDataInstanceId ").append(referredDataInstanceId).append(" ")
                .append("referredDataId ").append(referredDataId).append(" ")
                .append("queryId ").append(queryId);

        System.out.println (msg);

        return true;
    }

    @Override
    public boolean dataAvailable (String id, String groupName, String objectId, String instanceId, String referredDataId,
                                  String mimeType, byte[] metadata, String queryId)
    {
        StringBuilder msg = new StringBuilder ("dataAvailable: ")
                .append("groupName ").append(groupName).append(" ")
                .append("referredDataObjectId ").append(objectId).append(" ")
                .append("referredDataInstanceId ").append(instanceId).append(" ")
                .append("referredDataId ").append(referredDataId).append(" ")
                .append("queryId ").append(queryId);

        System.out.println (msg);

        return true;
    }

    @Override
    public void searchArrived (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query) {
        StringBuilder msg = new StringBuilder ("searchArrived: ")
                .append("query Id ").append(queryId).append(" ")
                .append("groupName ").append(groupName).append(" ")
                .append("querier ").append(querier).append(" ")
                .append("queryType ").append(queryType).append(" ")
                .append("queryQualifiers ").append(queryQualifiers);

        System.out.println (msg);
    }

    @Override
    public void searchReplyArrived (String queryId, Collection<String> matchingMessageIds, String responderNodeId)
    {
        StringBuilder msg = new StringBuilder ("searchReplyArrived: ")
                .append("query Id ").append(queryId).append(" ")
                .append("responding node id ").append(responderNodeId).append(" ")
                .append("matching id: ");
        for (String id : matchingMessageIds) {
            msg.append(id).append(" ");
        }

        System.out.println (msg);
    }

    @Override
    public void searchReplyArrived (String queryId, byte[] reply, String responderNodeId)
    {
        StringBuilder msg = new StringBuilder ("searchReplyArrived: ")
                .append("query Id ").append(queryId).append(" ")
                .append("responding node id ").append(responderNodeId).append(" ")
                .append("reply id: ").append(new String(reply, StandardCharsets.UTF_8));

        System.out.println (msg);
    }

    // Chunking plugins

    @Override
    public List<ChunkWrapper> fragment(byte[] data, String inputMimeType, byte nChunks, byte compressionQuality)
    {
        if (TEST_CHUNKING_MIME_TYPE.compareToIgnoreCase(inputMimeType) != 0) {
            return null;
        }
        List<ChunkWrapper> chunks = new LinkedList<>();
        char ch = 'a';
        for (byte chunkId = 1; chunkId <= nChunks; chunkId++) {
            char[] chars = { ch, ch, ch, ch, ch};
            ByteBuffer bb = StandardCharsets.UTF_8.encode(CharBuffer.wrap(chars));
            byte[] subdata = new byte[bb.remaining()];
            bb.get(subdata);
            chunks.add(new ChunkWrapper(subdata, chunkId, nChunks, TEST_CHUNKING_MIME_TYPE));
            ch++;
        }
        return chunks;
    }

    @Override
    public byte[] extract(byte[] data, String inputMimeType, byte nChunks, byte compressionQuality, Collection<us.ihmc.chunking.Interval> intervals)
    {
        if (TEST_CHUNKING_MIME_TYPE.compareToIgnoreCase(inputMimeType) != 0) {
            return null;
        }
        return "ZZZZZ".getBytes(StandardCharsets.UTF_8);
    }

    @Override
    public byte[] reassemble(Collection<ChunkWrapper> chunks, Collection<AnnotationWrapper> annotations, String mimeType, byte totalNumberOfChunks, byte compressionQuality)
    {
        StringBuilder sb = new StringBuilder();
        for (ChunkWrapper chunk : chunks) {
            if (sb.length() > 0) {
                sb.append(System.lineSeparator());
            }
            sb.append(new String(chunk.getData(), StandardCharsets.UTF_8)).append(sb);
        }
        for (AnnotationWrapper annotation : annotations) {
            sb.append(System.lineSeparator())
                    .append(new String(annotation.getData(), StandardCharsets.UTF_8));
        }
        return sb.toString().getBytes(StandardCharsets.UTF_8);
    }

    private String getLogMsg (String id, String objectId, String instanceId, String queryId, byte[] data)
    {
        return getLogMsg (id, objectId, instanceId, queryId, (data == null ? "" : new String (Arrays.copyOfRange(data, 0, Math.min(data.length, 100)))));
    }

    private String getLogMsg (String id, String objectId, String instanceId, String queryId, String msg)
    {
        return new StringBuilder (id).append(" ").append (objectId)
                .append(" ").append (instanceId)
                .append(" query id: <").append (queryId)
                .append("> ").append (msg).toString();
    }

    static private class Message
    {
        final byte[] metadata;
        final byte data[];

        Message(byte[] metadata, byte[] data)
        {
            this.metadata = metadata;
            this.data = data;
        }
    }

    static Message getData(String token) throws IOException {

        final byte[] metadata = token.getBytes(StandardCharsets.UTF_8);
        Scanner sc = new Scanner(token.trim());
        if(sc.hasNextInt()) {
            // Generate next data
            int len = sc.nextInt();
            byte[] data = new byte[len];
            for (int i = 0; i < data.length; i++) {
                data[i] = (byte) (i % 10);
            }
            return new Message(metadata, data);
        }
        // Assume it's a path to a file
        return new Message(metadata, Files.readAllBytes(new File(token).toPath()));
    }

    @SuppressWarnings("empty-statement")
    public static void main (String[] args) throws IOException, CommException, ProtocolException
    {
        // Read the name of the user connecting to the local instance of DSPro
        String username = null;
        String recepient = null;
        String host = "127.0.0.1:56487";
        for (int i = 0; i < args.length; i++) {
            if ((args[i].compareTo ("-host") == 0) && ((i + 1) < args.length))  {
                host = args[++i];
            }
        }
        String[] hostTokens = host.split(":");

        // Instantiate DSPro Proxy
        System.out.println ("Instantiating AsyncDSProProxy to connect to " + hostTokens[0] + " on port " + hostTokens[1]);
        DSProProxy innerProxy = new DSProProxyBuilder((short)5)
                .setHost (hostTokens[0])
                .setPort (Integer.parseInt(hostTokens[1]))
                .build();

        final AsyncDSProProxy proxy = new AsyncDSProProxy (new QueuedDSProProxy (innerProxy));
        int rc = proxy.init();
        if (rc != 0) {
            System.err.println ("Problem in initializing the dspro2 proxy. Return init value = " + rc);
            System.exit (-1);
        }

        // Register DSPro listeners
        DSProCmdProc chat = new DSProCmdProc (proxy, username, recepient);
        proxy.registerDSProProxyListener (chat);
        proxy.registerSearchListener (chat);
        proxy.registerChunkFragmenter(TEST_CHUNKING_MIME_TYPE, chat);
        proxy.registerChunkReassembler(TEST_CHUNKING_MIME_TYPE, chat);
        new Thread (proxy).start();

        // Register the local user with DSPro
        proxy.addUserId (username);

        new Thread (chat).start();
    }
}

