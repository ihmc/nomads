package us.ihmc.aci.util.dspro.test;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.ObjectOutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.disServiceProxy.Utils;
import us.ihmc.aci.dspro2.DSProProxy;
import us.ihmc.aci.dspro2.DSProProxyListener;
import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.aci.util.dspro.NodePath;
import us.ihmc.aci.util.dspro.XMLMetadataParser;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DSProTCPBridge extends DSProProxy implements Runnable
{
    private final String _bridgeNodeId;
    private final String _bridgeHost;
    private final int _bridgePort;
    private final short _bridgeApplicationId;
    private ObjectOutputStream _out;
    private final AtomicInteger _seqId  = new AtomicInteger(0);
    private final LinkedBlockingDeque<Message> _enqueuedMessages  = new LinkedBlockingDeque<Message>();
    private static final String SIG_ACT_GROUP = "sigactgrp";
    private static final String SIG_ACT_OBJECT_ID = "sigActObjId";
    private static org.apache.log4j.Logger log = Logger.getLogger ("debugLogger");

    public DSProTCPBridge()
    {
        this ((short) 3, "127.0.0.1", 7777);
    }

    public DSProTCPBridge (short applicationId)
    {
        this (applicationId, "127.0.0.1", 7777);
    }

    /**
     * @param applicationId
     * @param reinitializationAttemptInterval: the time to wait before trying to
     *                                         reconnect to the DSPro service upon
     *                                         disconnection.
     */
    public DSProTCPBridge (short applicationId, long reinitializationAttemptInterval)
    {
        this (applicationId, "127.0.0.1", 7777);
    }

    /**
     * @param applicationId
     * @param host: the IP of the disservicepro proxy server in dot-decimal
     *              notation
     * @param iPort
     */
    public DSProTCPBridge (short applicationId, String host, int iPort)
    {
        this (applicationId, host, iPort, 0);
    }

    /**
     * 
     * @param applicationId
     * @param host: the IP of the disservicepro proxy server in dot-decimal
     *              notation
     * @param port
     * @param reinitializationAttemptInterval: the time to wait before trying to
     *                                         reconnect to the DSPro service upon
     *                                         disconnection.
     */
    public DSProTCPBridge (short applicationId, String host, int port, long reinitializationAttemptInterval)
    {
        _bridgeApplicationId = applicationId;
        _bridgeHost = host;
        _bridgePort = port;
        _bridgeNodeId = "DSProTCPBridge" + "-" + _bridgeApplicationId;
    }

    private static Properties getLog4jProperties (String configFilePath)
    {
        Properties log4jProperties = new Properties();
        try {
            log4jProperties.load (new FileInputStream (configFilePath));
            String logFileName = log4jProperties.getProperty ("log4j.appender.rollingFile.File");
            Date day = new Date();
            String formattedDate = new SimpleDateFormat ("yyyyMMddhhmm").format (day);
            log4jProperties.setProperty ("log4j.appender.rollingFile.File", String.format ("../logs/%s-%s",
                    formattedDate, logFileName));
        }
        catch (FileNotFoundException e) {
            log.error ("Unable to load log4j configuration, file not found", e);
        }
        catch (IOException e) {
            log.error ("Unable to load log4j configuration, error while I/O on disk", e);
        }

        return log4jProperties;
    }

    public void configLogger (String loggerConfig)
    {
        Properties log4jProperties = getLog4jProperties (loggerConfig);
        PropertyConfigurator.configure (log4jProperties);
    }

    public void run()
    {
        while (true) {
            Message msg = null;
            try {
                msg = _enqueuedMessages.pollFirst (Long.MAX_VALUE, TimeUnit.DAYS);
                if (msg != null) {
                    _out.writeObject (msg);
                    if (msg._groupName.equals (SIG_ACT_GROUP)) {
                        logMsg (msg);
                    }
                }
            }
            catch (InterruptedException ex) {}
            catch (IOException ex) {
                _isInitialized.set (false);
                _enqueuedMessages.addFirst(msg);
                log.warn (StringUtil.getStackTraceAsString(ex));
                while (!_isInitialized.get()) {
                    init();
                    try { Thread.sleep (500); }
                    catch (InterruptedException ex1) {}
                }
            }
        }
    }

    @Override
    public String getSessionId() throws CommException
    {
        return null; 
    }

    @Override
    public int init()
    {
        try {
            Socket clientSocket = new Socket (_bridgeHost, _bridgePort);
            _out = new ObjectOutputStream (clientSocket.getOutputStream());
            _out.flush();
            _isInitialized.set (true);
            return 0;
        }
        catch (UnknownHostException ex) {
            log.warn (StringUtil.getStackTraceAsString(ex));
        }
        catch (IOException ex) {
            log.warn (StringUtil.getStackTraceAsString(ex));
        }
        return -1;
    }

    @Override
    public boolean isInitialized()
    {
        return _isInitialized.get();
    }

    private void logMsg (Message msg)
    {
        StringBuilder sb = new StringBuilder (Long.toString(System.currentTimeMillis()))
            .append(", ").append (msg._id)
            .append(", ").append (msg._objectId)
            .append(", ").append (msg._instanceId);
        log.info (sb);
    }

    @Override
    public synchronized String addAnnotation (String groupName, String objectId, String instanceId,
                                              String xmlMedatada, String referredObject,
                                              long expirationTime)
            throws CommException
    {
        return null;
    }

    @Override
    public synchronized String addAnnotation (String groupName, String objectId, String instanceId,
                                              String xmlMetadata, long expirationTime)
            throws CommException
    {
        return null;
    }

    public synchronized String addMessage (Message msg)
    {
        _enqueuedMessages.addFirst (msg);
        return msg._id;
    }

    @Override
    public synchronized String addMessage (String groupName, String objectId, String instanceId,
                                           String xmlMedatada, byte[] data, long expirationTime)
            throws CommException
    {
        checkConcurrentModification("addMessage");

        String id;
        try {
            id = Utils.getMessageID (_bridgeNodeId, groupName, _seqId.getAndAdd(1));
            return addMessage (new Message (id, groupName, objectId, instanceId, xmlMedatada, data, expirationTime));
        }
        catch (Exception ex) {
            log.warn (StringUtil.getStackTraceAsString(ex));
            log.warn (ex);
        }
        return null;
    }

    @Override
    public String addMessage (String groupName, String objectId, String instanceId,
                              Map<Object, Object> metadata, byte[] data, long expirationTime)
            throws CommException
    {
        return addMessage (groupName, objectId, instanceId,
                           us.ihmc.aci.util.dspro.XMLMetadataParser.toXML(metadata),
                           data, expirationTime);
    }

    @Override
    public String addMessage (String groupName, String objectId, String instanceId,
                              String[] metaDataAttributes, String[] metaDataValues,
                               byte[] data, long expirationTime)
            throws CommException
    {
        return addMessage (groupName, objectId, instanceId,
                           us.ihmc.aci.util.dspro.XMLMetadataParser.toXML(
                                   Arrays.asList(metaDataAttributes),
                                   Arrays.asList(metaDataValues)),
                           data, expirationTime);
    }

    @Override
    public String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                      String xmlMedatada, byte[] data, String dataMimeType,
                                      long expirationTime) throws CommException
    {
        return addMessage(groupName, objectId, instanceId, xmlMedatada, data, expirationTime);
    }

    @Override
    public String chunkAndAddMessage (String groupName, String objectId, String instanceId,
                                      String[] metaDataAttributes, String[] metaDataValues,
                                      byte[] data, String dataMimeType,
                                      long expirationTime) throws CommException
    {
        return addMessage(groupName, objectId, instanceId, metaDataAttributes, metaDataValues, data, expirationTime);
    }

    @Override
    public synchronized boolean addPeer (short adaptorType, String networkInterface, String remoteAddress,
                                         short port)
            throws CommException
    {
        return false;
    }

    @Override
    public synchronized void cancel (String id)
            throws CommException, ProtocolException
    {
    }

    @Override
    public synchronized boolean configureProperties (Properties properties)
        throws CommException
    {
        return false;
    }

    @Override
    public synchronized boolean setRankingWeights (float coordRankWeight, float timeRankWeight,
                                                   float expirationRankWeight, float impRankWeight,
                                                   float predRankWeight, float targetWeight,
                                                   boolean strictTarget) throws CommException
    {
        return false;
    }

    @Override
    public synchronized boolean registerPath (NodePath path)
            throws CommException
    {
        return true;
    }

    @Override
    public synchronized int registerDSProProxyListener (DSProProxyListener listener)
            throws CommException
    {
        return 0;
    }
    
    @Override
    public synchronized int registerMatchmakingLogListener (MatchmakingLogListener listener)
            throws CommException
    {
        return 0;
    }

    public static void main (String args[]) throws IOException
    {
        int port = 56487;
        String forwardingHost = null;
        int forwardingPort = port;
        boolean bSendMessage = false;
        for (int i = 0; i < args.length; i++) {
            if (args[i].compareTo("-port") == 0) {
                port = Integer.parseInt(args[++i]);
            }
            if (args[i].compareTo("-forwardTo") == 0) {
                String[] fwdSock = args[++i].split(":");
                forwardingHost = fwdSock[0];
                if (fwdSock.length > 1) {
                    forwardingPort = Integer.parseInt(fwdSock[1]);
                }
            }
            if (args[i].compareTo("-sendMessage") == 0) {
                bSendMessage = true;
            }
        }
        final DSProTCPBridge bridge = new DSProTCPBridge ((short)3, forwardingHost, forwardingPort);
        bridge.init();
        new Thread (bridge).start();

        if (bSendMessage) {
            new Thread () {
                @Override
                public void run()
                {
                    int i = 0;
                    while (true) {
                        String id = Integer.toString(i++);
                        String groupName = "grp";
                        String objectId = "objId";
                        String instanceId = "instId";
                        Map metadata = new HashMap();
                        metadata.put(MetadataElement.Message_ID.toString(), id);
                        String xmlMedatada = us.ihmc.aci.util.dspro.XMLMetadataParser.toXML(metadata);
                        byte[] data = null;
                        long expirationTime = 0;
                        bridge.addMessage (new Message (id, groupName, objectId, instanceId, xmlMedatada, data, expirationTime));

                        try {
                            Thread.sleep (1000);
                        }
                        catch (InterruptedException ex) {
                            log.warn (StringUtil.getStackTraceAsString(ex));
                        }
                    }
                }
            }.start();
        }

        final BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        int i = 0;
        for (String input; (input = br.readLine()) != null;) {
            try {
                final String[] inputTokens = input.split ("\\s+");
                final String cmd = inputTokens[0];
                if (("quit".compareToIgnoreCase (cmd) == 0) || ("exit".compareToIgnoreCase (cmd) == 0)) {
                    break;
                }
                else if ("add".compareToIgnoreCase (cmd) == 0) {
                    final String pathToData = inputTokens[1];
                    final String pathToMetadata = pathToData + ".dpmd";

                    final String id = Integer.toString(i++);
                    final String groupName = SIG_ACT_GROUP;
                    final String objectId = SIG_ACT_OBJECT_ID;
                    final String instanceId = "instId";
                    final HashMap metadata = TCPProxyServer.parse (new FileReader (pathToMetadata));
                    final String xmlMedatada = XMLMetadataParser.toXML(metadata);
                    final byte[] data = Files.readAllBytes(Paths.get(pathToData));
                    final long expirationTime = 0;
                    bridge.addMessage (new Message (id, groupName, objectId, instanceId, xmlMedatada, data, expirationTime));
                }
            }
            catch(IOException io){
                log.warn (StringUtil.getStackTraceAsString(io));
            }
	}
    }
}
