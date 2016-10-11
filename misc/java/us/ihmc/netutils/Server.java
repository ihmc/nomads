package us.ihmc.netutils;

import org.apache.log4j.Logger;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.ServerMocket;
import us.ihmc.mockets.StreamMocket;
import us.ihmc.mockets.StreamServerMocket;
import us.ihmc.netutils.protocol.Protocol;

import java.io.IOException;
import java.net.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Server.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Server implements Runnable, MessageListener
{


    public Server (final String listenAddr, final int port, final Protocol protocol, final String configFile)
    {
        _listenAddr = listenAddr;
        _port = port;
        _protocol = protocol;
        _children = new ConcurrentHashMap<Short, ServerChild>();
        _messageListeners = new ArrayList<MessageListener>();
        _messages = new LinkedBlockingQueue<byte[]>();
        _configFile = configFile;
    }


    public Server (final int port, final Protocol protocol, final String configFile)
    {
        _listenAddr = "127.0.0.1";
        _port = port;
        _protocol = protocol;
        _children = new ConcurrentHashMap<Short, ServerChild>();
        _messageListeners = new ArrayList<MessageListener>();
        _messages = new LinkedBlockingQueue<byte[]>();
        _configFile = configFile;
    }

    public Server (final String listenAddr, final int port, final Protocol protocol, final String configFile,
                   final int tcpControlPort)
    {
        _listenAddr = listenAddr;
        _port = port;
        _protocol = protocol;
        _children = new ConcurrentHashMap<Short, ServerChild>();
        _messageListeners = new ArrayList<MessageListener>();
        _messages = new LinkedBlockingQueue<byte[]>();
        _configFile = configFile;
        DEFAULT_CTRL_PORT = tcpControlPort;
    }

    public Server (final int port, final Protocol protocol, final String configFile, final int tcpControlPort)
    {
        _listenAddr = "127.0.0.1";
        _port = port;
        _protocol = protocol;
        _children = new ConcurrentHashMap<Short, ServerChild>();
        _messageListeners = new ArrayList<MessageListener>();
        _messages = new LinkedBlockingQueue<byte[]>();
        _configFile = configFile;
        DEFAULT_CTRL_PORT = tcpControlPort;
    }

    public Protocol getProtocol ()
    {
        return _protocol;
    }

    public Server getControlServer ()
    {
        return _controlServer;
    }

    public boolean init ()
    {
        try {
            LOG.info("Initializing " + _protocol + " server on port: " + _port);
            switch (_protocol.type) {
                case TCP:
                    _serverSocket = new ServerSocket(_port, 10, InetAddress.getByName(_listenAddr));
                    break;
                case Mockets:
                    //check if mocket or stream mocket
                    switch (_protocol.socket) {
                        case Mocket:
                            _serverMocket = new ServerMocket(_port, _listenAddr, _configFile);
                            //_serverMocket = new ServerMocket(_port, "127.0.0.1", _configFile);
                            LOG.info("Initializing ServerMocket on port " + _port + " with config file: " +
                                    _configFile);
                            break;
                        case StreamMocket:
                            _streamServerMocket = new StreamServerMocket(new InetSocketAddress(
                                    InetAddress.getByName(_listenAddr), _port));

                            LOG.info("Initializing StreamServerMocket on port " + _port + " with config file: " +
                                    _configFile);
                            break;
                        default:
                            throw new IllegalArgumentException("Protocol not supported: " + _protocol);

                    }
                    break;
                case UDP:
                    //check if multicast
                    switch (_protocol.socket) {
                        case MulticastSocket:
                            if (!Inet4Address.getByName(_listenAddr).isMulticastAddress()) {
                                throw new IllegalArgumentException("Listen address is not multicast address");
                            }
                            _serverDSocket = new MulticastSocket(_port);
                            ((MulticastSocket) _serverDSocket).joinGroup(InetAddress.getByName(_listenAddr));
                            break;
                        case DatagramSocket:
                            _serverDSocket = new DatagramSocket(_port, InetAddress.getByName(_listenAddr));
                            break;
                    }
                    break;
                default:
                    throw new IllegalArgumentException("Protocol not supported: " + _protocol);
            }

            return true;
        }
        catch (IOException e) {
            LOG.error(ConnErrorHandler.getBoundError(_port), e);
            onBoundError(_protocol, _port);
            return false;
        }
    }

    public void start ()
    {
        if (_protocol.type.equals(Protocol.Type.UDP))
            (new Thread(new ControlRunnable(this), "UDPControlThread")).start();
        else
            (new Thread(this, "ServerThread")).start();
    }

    CommHelperAdapter accept ()
    {
        CommHelperAdapter commHelper;
        LOG.info("Waiting for incoming " + _protocol + " connections on port: " + _port);
        try {
            switch (_protocol.type) {
                case TCP:
                    Socket socket = _serverSocket.accept();
                    commHelper = CommHelperAdapter.newInstance(_protocol);
                    commHelper.init(socket);
                    break;
                case Mockets:
                    if (_protocol.socket.equals(Protocol.Socket.Mocket)) {
                        Mocket mocket = _serverMocket.accept();
                        commHelper = CommHelperAdapter.newInstance(_protocol);
                        commHelper.init(mocket);
                    }
                    //TODO this is for testing purposes
                    else if (_protocol.socket.equals(Protocol.Socket.StreamMocket)) {
                        StreamMocket streamMocket = _streamServerMocket.accept();
                        commHelper = CommHelperAdapter.newInstance(_protocol);
                        commHelper.init(streamMocket.getInputStream(), streamMocket.getOutputStream());
                    }
                    else {
                        throw new IllegalArgumentException("Protocol not supported: " + _protocol);
                    }
                    break;
                case UDP:
                    commHelper = CommHelperAdapter.newInstance(_protocol);
                    commHelper.init(_serverDSocket);
                    break;
                default:
                    throw new IllegalArgumentException("Protocol not supported: " + _protocol);
            }

            if (!_protocol.type.equals(Protocol.Type.UDP)) LOG.info(_protocol + " connection accepted by client");
            return commHelper;
        }
        catch (IOException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_port), e);
            return null;
        }
    }

    Map<Short, ServerChild> getChildren ()
    {
        return _children;
    }

    int getPort ()
    {
        return _port;
    }

    @Override
    public void run ()
    {
        LOG.debug(_protocol + "Server thread started");

        while (!Thread.currentThread().isInterrupted()) {
            CommHelperAdapter commHelper = accept();
            ServerConnHandler connHandler = new ServerConnHandler(this, commHelper);
            connHandler.start();
        }
    }

    /**
     * ControlRunnable
     * <p/>
     * Class <code>ControlRunnable</code> is used by a UDP server to have an additional TCP (thus reliable) connection
     * through which communicate with the client for handshake and statistics purposed. The number of packets sent
     * through this connection is very limited for performance reasons. Mockets and TCP do not need this additional
     * connection since they can use their own main connection to exchange stas.
     */
    class ControlRunnable implements Runnable, MessageListener
    {
        public ControlRunnable (Server server)
        {
            _server = server;
        }

        @Override
        public void run ()
        {
            _controlServer = new Server(DEFAULT_CTRL_PORT, Protocol.TCP, null);
            LOG.debug("Starting TCP ControlServer");
            _controlServer.addMessageListener(this);
            if (_controlServer.init()) {
                _controlServer.start();
            }
        }

        //////////////////////
        /* Notify listeners */
        //////////////////////

        @Override
        public void onMessage (short clientId, byte[] message)
        {
            handleUDP(clientId, message);
        }

        @Override
        public void onProgress (int bytesSent, int total)
        {

        }

        @Override
        public void onTestStream (short clientId, int streamSize, Stats stats)
        {
            _server.onTestStream(clientId, streamSize, stats);
        }

        @Override
        public void onStats (short clientId, Stats stats)
        {
            _server.onStats(clientId, stats);
        }

        @Override
        public void onBoundError (Protocol protocol, int port)
        {
            _server.onBoundError(protocol, port);
        }

        private void handleUDP (short clientId, byte[] message)
        {
            if (_protocol.type.equals(Protocol.Type.UDP)) {
                LOG.debug("Received onMessage from: " + clientId);
                String messageStr = new String(message);
                RequestType requestType = RequestType.valueOf(messageStr);

                switch (requestType) {
                    case udpRegister:
                        LOG.debug("Received " + RequestType.udpRegister + " request, sending handshake to client");
                        _controlServer.getChildren().get(clientId).sendMessage(message);
                        //handling separate from ServerConnHandler for UDP
                        if (init()) {
                            CommHelperAdapter commHelper = accept();
                            ServerChild serverChild = new ServerChild(clientId, "UDPClient", commHelper, _server);
                            getChildren().put(clientId, serverChild);
                            serverChild.start();
                        }

                        break;
                    case udpDisconnect:
                        LOG.debug("Received " + RequestType.udpDisconnect + " request, disconnecting UDP child");
                        if (getChildren().get(clientId) != null) getChildren().get(clientId).disconnectChild();
                        getChildren().remove(clientId);
                        _controlServer.getChildren().clear();
                        break;
                }
            }
        }

        private final Server _server;
        private final Logger LOG = Logger.getLogger(ControlRunnable.class);
    }

    /**
     * Get current queued message;
     *
     * @return a <code>Queue</code> with the messages waiting to be processed.
     */
    public LinkedBlockingQueue<byte[]> getMessages ()
    {
        return _messages;
    }

    //////////////////////
    /* Notify listeners */
    //////////////////////

    @Override
    public void onMessage (short clientId, byte[] message)
    {
        if (_messageListeners.isEmpty()) {
            return;
        }

        for (MessageListener listener : _messageListeners) {
            listener.onMessage(clientId, message);
        }
    }

    @Override
    public void onProgress (int bytesSent, int total)
    {

    }

    @Override
    public void onTestStream (short clientId, int streamSize, Stats stats)
    {
        if (_messageListeners.isEmpty()) {
            return;
        }

        for (MessageListener listener : _messageListeners) {
            listener.onTestStream(clientId, streamSize, stats);
        }
    }

    @Override
    public void onStats (short clientId, Stats stats)
    {
        onStatsReply(clientId, stats);

        if (_messageListeners.isEmpty()) {
            return;
        }

        for (MessageListener listener : _messageListeners) {
            listener.onStats(clientId, stats);
        }

    }

    @Override
    public void onBoundError (Protocol protocol, int port)
    {
        if (_messageListeners.isEmpty()) {
            return;
        }

        for (MessageListener listener : _messageListeners) {
            listener.onBoundError(protocol, port);
        }
    }

    private void onStatsReply (short clientId, final Stats stats)
    {
        if (_port == DEFAULT_CTRL_PORT) return;
        //TCP control doesn't not have to reply through this instance

        LOG.debug(_protocol + " server running on port " + _port);
        LOG.debug("Received onStats from client: " + clientId + " (client sent: " + stats.msgSent + " and received: "
                + stats.msgReceived + ")");
        //send (client) sent messages and (server) received message
        if (getChildren().get(clientId) == null) {
            LOG.error("Server child with id " + clientId + " not found.");
            return;
        }
        int msgReceivedChild = getChildren().get(clientId).getStats().msgReceived.intValue();
        stats.msgReceived.set(msgReceivedChild);

        //if received > sent, correction
        if (msgReceivedChild > stats.msgSent.get())
            stats.msgSent.set(msgReceivedChild);

        if (_protocol.type.equals(Protocol.Type.UDP)) {

            if (_controlServer.getChildren().get(clientId) == null) {
                LOG.error("Server child with id " + clientId + " not found.");
                return;
            }
            _controlServer.getChildren().get(clientId).sendStats(stats);
            LOG.debug("TCP control server reply. Sent msgs: " + stats.msgSent.get() + ". Received msgs: " + stats
                    .msgReceived.get());
        }
        else {
            getChildren().get(clientId).sendStats(stats);
            LOG.debug(_protocol + " server reply. Sent msgs: " + stats.msgSent.get() + ". Received msgs: " + stats
                    .msgReceived.get());
        }
    }


    public void addMessageListener (MessageListener listener)
    {
        _messageListeners.add(listener);
        LOG.info("Added MessageListener");
    }

    public void removeMessageListener (MessageListener listener)
    {
        if (!_messageListeners.isEmpty()) {
            _messageListeners.remove(listener);
            LOG.info("Removed MessageListener");
        }
    }

    private final Map<Short, ServerChild> _children;
    private ServerMocket _serverMocket;
    private StreamServerMocket _streamServerMocket;
    private ServerSocket _serverSocket;
    private DatagramSocket _serverDSocket;
    private final String _listenAddr;
    private final int _port;
    private final Protocol _protocol;
    private final List<MessageListener> _messageListeners;
    private final String _configFile;
    private Server _controlServer;
    private final LinkedBlockingQueue<byte[]> _messages;
    private final static Logger LOG = Logger.getLogger(Server.class);

    //public final static char ACK = '.';
    public final static String ACK = ".";
    public static int MAX_MSG_SIZE = 1024;
    public static volatile int DEFAULT_CTRL_PORT = 9001;
}