package us.ihmc.aci.dspro2;

import java.io.Console;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.comm.CommException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DSProCmdProc implements DSProProxyListener, SearchListener, Runnable {

    enum Dimension
    {
        x,
        y,
        t
    }

    class Interval
    {
        public long start;
        public long end;
    }

    private final DSProProxy _proxy;

    DSProCmdProc (DSProProxy proxy, String localUser, String recepient)
    {
        _proxy = proxy;
    }

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
                else if (0 == tokens[0].compareToIgnoreCase("addMessage")) {
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
                    String dsproId = _proxy.addMessage (groupName, objectId, instanceId, 
                                                        metadata, data, expirationTime);
                    System.out.println("Added message with id " + dsproId);
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
                            switch (Dimension.valueOf(tokens[i])) {
                                case x:
                                    ui32StartXPixel = Long.parseLong(tokens[++i]);
                                    ui32EndXPixel = Long.parseLong(tokens[++i]);
                                    break;
                                case y:
                                    System.out.println("y: " + tokens[i+1] + " " + tokens[i+2]);
                                    ui32StartYPixel = Long.parseLong(tokens[++i]);
                                    ui32EndYPixel = Long.parseLong(tokens[++i]);
                                    break;
                                case t:
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

    public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission)
    {
        return true;
    }

    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId)
    {
        return true;
    }

    public void newNeighbor (String peerID)
    {
	    System.out.println ("newNeighbor: " + peerID);
    }

    public void deadNeighbor (String peerID)
    {
	    System.out.println ("deadNeighbor: " + peerID);
    }

    public boolean dataArrived (String id, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId,
                                String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String queryId)
    {
        System.out.println (getLogMsg (objectId, instanceId, id, queryId, data));

        return true;
    }

    public boolean metadataArrived (String id, String groupName, String referredDataObjectId,
                                    String referredDataInstanceId, String XMLMetadata,
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

    public void searchArrived (String queryId, String groupName, String querier, String queryType, String queryQualifiers, byte[] query)
    {
        StringBuilder msg = new StringBuilder ("searchArrived: ")
                .append("query Id ").append(queryId).append(" ")
                .append("groupName ").append(groupName).append(" ")
                .append("querier ").append(querier).append(" ")
                .append("queryType ").append(queryType).append(" ")
                .append("queryQualifiers ").append(queryQualifiers);

        System.out.println (msg);
    }

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

    public void searchReplyArrived (String queryId, byte[] reply, String responderNodeId)
    {
        StringBuilder msg = new StringBuilder ("searchReplyArrived: ")
                .append("query Id ").append(queryId).append(" ")
                .append("responding node id ").append(responderNodeId).append(" ")
                .append("reply id: ").append(new String(reply, StandardCharsets.UTF_8));

        System.out.println (msg);
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

    @SuppressWarnings("empty-statement")
    public static void main (String[] args) throws IOException, CommException
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
        final AsyncDSProProxy proxy = new AsyncDSProProxy ((short)5, hostTokens[0], Integer.parseInt(hostTokens[1]));
        int rc = proxy.init();
        if (rc != 0) {
            System.err.println ("Problem in initializing the dspro2 proxy. Return init value = " + rc);
            System.exit (-1);
        }

        // Register DSPro listeners
        DSProCmdProc chat = new DSProCmdProc (proxy, username, recepient);
        proxy.registerDSProProxyListener (chat);
        proxy.registerSearchListener (chat);
        new Thread (proxy).start();

        // Register the local user with DSPro
        proxy.addUserId (username);

        new Thread (chat).start();
    }
}

