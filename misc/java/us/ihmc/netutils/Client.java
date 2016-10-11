package us.ihmc.netutils;

import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.StreamMocket;
import us.ihmc.netutils.protocol.Protocol;

import java.io.IOException;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Client.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Client implements us.ihmc.netutils.MessageListener
{
    public Client (final String name, CommDetails commDetails)
    {
        this((commDetails.getProtocol().type.equals(Protocol.Type.UDP) ||
                        commDetails.getMode().equals(Mode.Stream)) ? 0 : (short) new Random().nextInt(Short.MAX_VALUE
                        + 1),
                name, commDetails);
    }

    public Client (final short id, final String name, CommDetails commDetails)
    {
        _id = ((commDetails.getProtocol().type.equals(Protocol.Type.UDP) ||
                commDetails.getMode().equals(Mode.Stream)) ? 0 : id); //only one connection
        _name = name;
        _commDetails = commDetails;
        _isConnected = new AtomicBoolean(false);
        _messageListeners = new ArrayList<MessageListener>();
        stats = new Stats(Stats.Type.Client);
    }

    public short getId ()
    {
        return _id;
    }

    public String getName ()
    {
        return _name;
    }

    public CommDetails getCommDetails ()
    {
        return _commDetails;
    }

    public boolean isConnected ()
    {
        return _isConnected.get();
    }

    public synchronized boolean connect ()
    {
        if (_isConnected.get()) {
            LOG.warn("Client is already connected to " + _commDetails);
            return true;
        }

        CommHelperAdapter ch = connectToServer(_commDetails.getRemoteHost(), _commDetails.getPort(),
                _commDetails.getProtocol());

        if (ch == null) {
            LOG.warn("Client unable to connect");
            return false;
        }

        //no need to register
        if (_commDetails.getMode().equals(Mode.Stream)) {
            _commHelper = ch;
            _isConnected.set(true);
            LOG.debug("Connected to " + _commDetails);
            return _isConnected.get();
        }

        if (_commDetails.getProtocol().type.equals(Protocol.Type.UDP)) {
            _commHelper = ch;
            LOG.debug("Starting additional TCP control connection for UDP");

            CommDetails commDetails = new CommDetails.Builder(_commDetails.getRemoteHost(),
                    Protocol.TCP,
                    _commDetails.getTcpControlPort())
                    .mode(Mode.Interval).build();

            _tcpControlClient = new Client(_id, "ControlClient", commDetails);
            _tcpControlClient.addMessageListener(this);
            _tcpControlClient.connect();

            _tcpControlClient.sendMessage(RequestType.udpRegister.toString().getBytes());
            return _isConnected.get();
        }

        CommHelperAdapter chCallback = connectToServer(_commDetails.getRemoteHost(), _commDetails.getPort(),
                _commDetails.getProtocol());
        if (ch == null || chCallback == null) return false;

        //only register for TCP and Mockets reliable/sequenced
        int rc = registerClient(ch, chCallback, _id, _name);
        if (rc >= 0) {
            _id = (short) rc; //the server may have assigned a different id than requested

            _commHelper = ch;
            _callbackHandler = new ClientCallbackHandler(this, chCallback);
            _callbackHandler.start();

            _isConnected.set(true);
            LOG.debug("Connected client " + _id + " " + _name + " through " + _commDetails);
            return _isConnected.get();
        }

        _isConnected.set(false);
        return _isConnected.get();
    }

    public synchronized void closeConnection ()
    {
        if (_commHelper != null) _commHelper.closeConnection();
        if (_callbackHandler != null) _callbackHandler.STOP.set(true);
        if (_callbackHandler != null) _callbackHandler.closeConnection();
        //if UDP, send udpDisconnect and close control client
        if (_commDetails.getProtocol().type.equals(Protocol.Type.UDP)) {
            _tcpControlClient.sendMessage(RequestType.udpDisconnect.toString().getBytes());
            _tcpControlClient.closeConnection();
        }
        _isConnected.set(false);
        _commHelper = null;
        _callbackHandler = null;
    }

    public synchronized Mocket.Statistics requestStats () throws IOException
    {
        if (_commDetails.getProtocol().type.equals(Protocol.Type.UDP) && !_tcpControlClient.isConnected()) {
            LOG.warn("Client is UDP but no TCP control connections established");
            return null;
        }

        switch (_commDetails.getProtocol().type) {
            case TCP:
            case Mockets:
                sendStats(stats);
                LOG.debug(_commDetails.getProtocol() + " stats request sent.");
                return _commHelper.getStatistics();
            case UDP:
                LOG.debug(_commDetails.getProtocol() + " stats request sent.");
                _tcpControlClient.sendStats(stats);
                return null;
            default:
                return null;
        }
    }


    //only TCP and Mockets
    public synchronized boolean sendTestStream (byte[] message, int streamSize)
    {
        if (_commDetails.getProtocol().type.equals(Protocol.Type.UDP)) {
            throw new IllegalArgumentException("Protocol " + _commDetails.getProtocol() + " not supported for streams");
        }

        if (!isConnected()) {
            LOG.warn("Client is not connected, unable to send stream");
            return false;
        }

        try {
            _commHelper.write32(streamSize);
            LOG.debug("Sending stream of size " + streamSize);
            _commHelper.receiveMatch(Server.ACK);//wait to receive '.' from server
            LOG.debug("Received initial ACK '.' from Server " + streamSize);
            int bytesSent = 0, bytesToSend;
            long startTime, time;
            double throughputSent, throughputReceived, timeInSeconds;
            ;

            startTime = System.currentTimeMillis();

            //call on progress 24 times
            int progressUnit = streamSize / DEFAULT_PROGRESS_UNITS;
            int progressCounter = 1;

            while (bytesSent < streamSize) {
                //LOG.debug("Before sending blob. Bytes sent: " + bytesSent + "/" + streamSize);
                bytesToSend = Math.min(message.length, (streamSize - bytesSent));
                _commHelper.sendBlob(message, 0, bytesToSend);
                //LOG.debug("After sending blob. Bytes sent: " + bytesSent + "/" + streamSize);
                stats.msgSent.incrementAndGet();
                bytesSent += bytesToSend;
                if (bytesSent >= progressUnit * progressCounter) {
                    onProgress(bytesSent, streamSize);
                    progressCounter++;
                }
            }
            time = System.currentTimeMillis() - startTime;
            //timeInSeconds = (TimeUnit.MILLISECONDS.convert(time, TimeUnit.NANOSECONDS) / 1000);
            throughputSent = (streamSize / time) * 1000;

            LOG.debug("Calculating throughput (sent). Stream size: " + streamSize + ". Msec: " + time);
            stats.throughputSent.set(Double.valueOf(throughputSent).intValue());
            LOG.debug("Waiting to receive final ACK from Server");
            _commHelper.receiveMatch(Server.ACK); //wait to receive '.' from server

            time = System.currentTimeMillis() - startTime;
            //timeInSeconds = (TimeUnit.MILLISECONDS.convert(time, TimeUnit.NANOSECONDS) / 1000);
            throughputReceived = (streamSize / time) * 1000;

            LOG.debug("Calculating throughput (received). Stream size: " + streamSize + ". Msec: " + time);
            stats.throughputReceived.set(Double.valueOf(throughputReceived).intValue());
            LOG.debug("Received final ACK '.' from Server " + streamSize);

            onTestStream(_id, streamSize, stats);
            //LOG.info(String.format("Waiting confirm from the server"));
            //_commHelper.receiveMatch(Request.confirm.toString(), TIMEOUT);
            //LOG.info(String.format("Waiting confirm from the server"));
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getConnectionError(_commDetails.getRemoteHost(), _commDetails.getPort()));
            _isConnected.set(false);
            return false;
            //disconnectChild();
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(_commDetails.getRemoteHost(), _commDetails.getPort()), e);
            return false;
        }

        return true;
    }

    public synchronized boolean sendMessage (byte[] message)
    {
        if (!isConnected()) {
            LOG.warn("Client is not connected, unable to send message");
            return false;
        }

        //UDP special case
        try {
            if (_commDetails.getProtocol().type.equals(Protocol.Type.UDP)) {
                LOG.trace("Sending " + _commDetails.getProtocol() + " message of size " + message.length);
                _commHelper.sendBlob(message);
            }
            else {
                LOG.trace("Sending " + _commDetails.getProtocol() + " " + RequestType.onMessage + " request");
                _commHelper.sendLine(RequestType.onMessage.toString());
                _commHelper.write32(message.length);
                _commHelper.sendBlob(message);
                //LOG.info(String.format("Waiting confirm from the server"));
                //_commHelper.receiveMatch(Request.confirm.toString(), TIMEOUT);
                //LOG.info(String.format("Waiting confirm from the server"));
                LOG.trace(RequestType.onMessage + " successful");
            }
            stats.msgSent.incrementAndGet();
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getConnectionError(_commDetails.getRemoteHost(), _commDetails.getPort()));
            _isConnected.set(false);
            return false;
            //disconnectChild();
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(_commDetails.getRemoteHost(), _commDetails.getPort()), e);
            return false;
        }

        return true;
    }

    boolean sendStats (final Stats stats)
    {
        try {
            LOG.trace("Sending " + RequestType.onStats.toString() + " request");
            _commHelper.sendLine(RequestType.onStats.toString());
            _commHelper.write32(stats.msgSent.get());
            _commHelper.write32(stats.msgReceived.get());
            _commHelper.write32(stats.throughputSent.get());
            _commHelper.write32(stats.throughputReceived.get());
            //LOG.info(String.format("Waiting confirm from the server"));
            //_commHelper.receiveMatch(Request.confirm.toString(), TIMEOUT);
            //LOG.info(String.format("Waiting confirm from the server"));
            LOG.trace(RequestType.onStats + " successful");
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getConnectionError(_commDetails.getRemoteHost(), _commDetails.getPort()));
            _isConnected.set(false);
            return false;
            //disconnectChild();
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(_commDetails.getRemoteHost(), _commDetails.getPort()), e);
            return false;
        }

        return true;
    }

    void setConnected (boolean isConnected)
    {
        _isConnected.set(isConnected);
    }

    private CommHelperAdapter connectToServer (final String host, final int port, final Protocol protocol)
    {
        try {
            LOG.debug("Initializing CommHelper");
            CommHelperAdapter commHelper = CommHelperAdapter.newInstance(protocol);
            switch (protocol.type) {
                case TCP:
                    Socket s = new Socket(host, port);
                    commHelper.init(s);
                    break;
                case Mockets:
                    if (_commDetails.getProtocol().socket.equals(Protocol.Socket.Mocket)) {
                        Mocket m = new Mocket(_commDetails.getConfigFile());
                        LOG.info("Initializing Mocket with config file: " + _commDetails.getConfigFile());
                        m.connect(host, port);
                        commHelper.init(m);
                    }
                    else if (_commDetails.getProtocol().socket.equals(Protocol.Socket.StreamMocket)) {
                        StreamMocket streamMocket = new StreamMocket();
                        streamMocket.connect(InetAddress.getByName(host), port);
                        commHelper.init(streamMocket.getInputStream(), streamMocket.getOutputStream());
                    }
                    else {
                        throw new IllegalArgumentException("Unsupported protocol " + protocol);
                    }
                    break;
                case UDP:
                    DatagramSocket ds = new DatagramSocket();
                    ds.connect(InetAddress.getByName(host), port);
                    commHelper.init(ds, host, port);
                    break;
                default:
                    throw new IllegalArgumentException("Unsupported protocol " + protocol);
            }
            LOG.debug(protocol + " CommHelper initialized and connected on " + host + ":" + port);
            return commHelper;
        }
        catch (IOException e) {
            LOG.error(ConnErrorHandler.getConnectionError(_commDetails.getRemoteHost(), _commDetails.getPort()));
            return null;
        }
        catch (IllegalArgumentException e) {
            LOG.error(ConnErrorHandler.getConnectionError(_commDetails.getRemoteHost(), _commDetails.getPort()));
            return null;
        }
    }

    private int registerClient (final CommHelperAdapter ch, final CommHelperAdapter chCallback,
                                final int requestedId, final String requestedName)
    {
        try {
            LOG.debug("Requesting register to Server for clientId: " + requestedId);
            ch.write32(RequestType.register.i32code());
            ch.receiveMatch(RequestType.confirm.toString());
            //sending real request
            StringBuilder sb = new StringBuilder();
            sb.append(RequestType.register.toString())
                    .append(Request.separator)
                    .append(requestedId)
                    .append(Request.separator)
                    .append(requestedName);
            ch.sendLine(sb.toString());
            String[] strArray = ch.receiveRemainingParsed(RequestType.confirm.toString()); //the proxy will
            // return the assigned app id
            int clientId = Short.parseShort(strArray[0]);

            LOG.debug("Confirm for register received with clientId: " + clientId);
            chCallback.write32(RequestType.registerCallback.i32code());
            chCallback.receiveMatch(RequestType.confirm.toString());
            //sending real request

            sb = new StringBuilder();
            sb.append(RequestType.registerCallback.toString())
                    .append(Request.separator)
                    .append(clientId)
                    .append(Request.separator)
                    .append(requestedName);
            chCallback.sendLine(sb.toString());
            LOG.debug("Waiting for registerCallback confirm from Server");
            chCallback.receiveMatch(RequestType.confirm.toString());
            LOG.debug("Confirm for registerCallback received");
            //assigned app id
            return clientId;
        }
        catch (CommException e) {
            _isConnected.set(false);
            LOG.error(ConnErrorHandler.getRegistrationError(_commDetails.getRemoteHost(), _commDetails.getPort()), e);
            return -1;
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(_commDetails.getRemoteHost(), _commDetails.getPort()), e);
            return -2;
        }
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

        if (_commDetails.getProtocol().type.equals(Protocol.Type.UDP)) {
            String request = new String(message);
            if (request.equals(RequestType.udpRegister.toString())) {
                LOG.debug("Received confirm of " + RequestType.udpRegister + " request, client connected");
                LOG.debug("Connected client " + _id + " with name " + _name + " through " + _commDetails);
                _isConnected.set(true);
            }
        }
    }

    @Override
    public void onProgress (int bytesSent, int total)
    {
        if (_messageListeners.isEmpty()) {
            return;
        }

        for (MessageListener listener : _messageListeners) {
            listener.onProgress(bytesSent, total);
        }
    }

    @Override
    public void onTestStream (short clientId, int streamSize, final Stats stats)
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

    private short _id;
    private final String _name;

    private CommDetails _commDetails;
    private CommHelperAdapter _commHelper;
    private ClientCallbackHandler _callbackHandler;
    private Client _tcpControlClient;
    private final ArrayList<MessageListener> _messageListeners;
    private final AtomicBoolean _isConnected;
    final Stats stats;

    private final static Logger LOG = Logger.getLogger(Client.class);

    public final static Mode DEFAULT_MODE = Mode.Interval;
    public final static int DEFAULT_MSG_SIZE_INTERVAL = 1024;
    public final static int MAX_MSG_SIZE_INTERVAL = 1024;
    public final static int DEFAULT_MSG_SIZE_STREAM = 312000;
    public final static int MAX_MSG_SIZE_STREAM = 10800000;
    public final static int DEFAULT_PROGRESS_UNITS = 24;
    public final static String DEFAULT_MOCKETS_CONFIG_FILE = "/tmp/mockets.conf";

    public final static int DEFAULT_INTERVAL = 1000;
}
