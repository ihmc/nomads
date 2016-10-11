package us.ihmc.netutils;

import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.netutils.protocol.Protocol;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * ServerChild.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ServerChild implements Runnable
{
    ServerChild (final short clientId, final String clientName, CommHelperAdapter ch, Server server)
    {
        _clientId = clientId;
        _clientName = clientName;
        _commHelper = ch;
        _callbackCommHelper = null;
        _stats = new Stats(Stats.Type.Server);
        _server = server;
        _isStream = new AtomicBoolean(false);
        _streamSize = new AtomicInteger(0);
        STOP = new AtomicBoolean(false);
    }

    ServerChild (final short clientId, final String clientName, int streamSize, CommHelperAdapter ch, Server server)
    {
        _clientId = clientId;
        _clientName = clientName;
        _commHelper = ch;
        _callbackCommHelper = null;
        _stats = new Stats(Stats.Type.Server);
        _server = server;
        _isStream = new AtomicBoolean(true);
        _streamSize = new AtomicInteger(streamSize);
        STOP = new AtomicBoolean(false);
    }

    void start ()
    {
        (new Thread(this, "ServerChildThread-" + _clientId)).start();
    }

    Stats getStats ()
    {
        return _stats;
    }

    synchronized void setCallbackCommHelper (CommHelperAdapter ch)
    {
        _callbackCommHelper = ch;
    }

    @Override
    public void run ()
    {
        LOG.info(String.format("Started ServerChildThread-" + _clientId));

        String request = null;
        while (!STOP.get()) {
            try {
                boolean success;

                if (_isStream.get()) {
                    doOnTestStream();
                }

                //UDP
                if (_server.getProtocol().type.equals(Protocol.Type.UDP)) {
                    LOG.trace("Received UDP packet");
                    success = doOnMessage();

                }//TCP, Mockets RS
                else {
                    //LOG.debug("Waiting for " + _commHelper.getProtocol() + " request");
                    request = _commHelper.receiveLine();
                    RequestType requestType = RequestType.valueOf(request);
                    LOG.trace("Received " + _commHelper.getProtocol() + " " + requestType + " request");
                    switch (requestType) {
                        case onMessage:
                            success = doOnMessage();
                            break;
                        case onStats:
                            success = doOnStats();
                            break;
                        default:
                            LOG.error(ConnErrorHandler.getProtocolError(request));
                            success = false;
                    }

                }

                if (!success) {
                    LOG.error("Failed to receive message from client id:" + _clientId + " name: " + _clientName);
                    //disconnectChild();
                    //break;
                }
            }
            catch (CommException e) {
                LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
                disconnectChild();
                break;
            }
            catch (IllegalArgumentException e) {
                LOG.error(ConnErrorHandler.getProtocolError(request), e);
                sendErrorToClient();
            }
            catch (ProtocolException e) {
                LOG.error(ConnErrorHandler.getProtocolError(), e);
            }
        }
    }

    private void sendErrorToClient ()
    {
        try {
            _commHelper.sendLine(RequestType.error.toString()); //send an error to proxy client
        }
        catch (CommException e) {
            e.printStackTrace();
        }
        catch (ProtocolException e) {
            e.printStackTrace();
        }
    }

    public void sendMessage (final byte[] message)
    {
        if (_callbackCommHelper == null) {
            LOG.error("Callback channel not registered");
            return;
        }

        try {
            LOG.trace("Sending " + RequestType.onMessage + " request");
            _callbackCommHelper.sendLine(RequestType.onMessage.toString());
            _callbackCommHelper.write32(message.length);
            _callbackCommHelper.sendBlob(message);
            //LOG.info(String.format("Waiting confirm from the client"));
            //_callbackCommHelper.receiveMatch(Request.confirm.toString(), TIMEOUT);
            LOG.trace(String.format("%s successful", RequestType.onMessage.toString()));
            _stats.msgSent.incrementAndGet();
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            //disconnectChild();
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
        }
    }

    public void sendStats (final Stats stats)
    {
        if (_callbackCommHelper == null) {
            LOG.error("Callback channel not registered");
            return;
        }

        try {
            LOG.trace("Sending " + RequestType.onStats + " request");
            _callbackCommHelper.sendLine(RequestType.onStats.toString());
            _callbackCommHelper.write32(stats.msgSent.get());
            _callbackCommHelper.write32(stats.msgReceived.get());
            _callbackCommHelper.write32(stats.throughputReceived.get());
            //LOG.info(String.format("Waiting confirm from the client"));
            //_callbackCommHelper.receiveMatch(Request.confirm.toString(), TIMEOUT);
            LOG.trace(RequestType.onMessage + " successful");
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            //disconnectChild();
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
        }
    }

    private boolean doOnTestStream ()
    {
        byte[] buf;
        try {
            LOG.debug("Acknowledged to receive a stream of size " + _streamSize.get() + " from client.");
            LOG.debug("Sending initial ACK symbol '" + Server.ACK + "' to client " + _clientId);
            _commHelper.sendLine(Server.ACK);

            int bytesRead = 0, bytesToReceive;
            long startTime, time;
            double throughput, timeInSeconds;

            startTime = System.currentTimeMillis();
            while (bytesRead < _streamSize.get()) {
                bytesToReceive = Math.min(Server.MAX_MSG_SIZE, (_streamSize.get() - bytesRead));
                //LOG.debug("Waiting for a buf of size " + bytesToReceive);
                buf = _commHelper.receiveBlob(bytesToReceive);
                bytesRead += buf.length;
                //LOG.debug("Bytes read so far: " + bytesRead);
                _stats.msgReceived.incrementAndGet();
            }
            time = System.currentTimeMillis() - startTime;
            LOG.debug("Sending final ACK symbol '" + Server.ACK + "' to client " + _clientId);
            _commHelper.sendLine(Server.ACK);

            //timeInSeconds = (TimeUnit.MILLISECONDS.convert(time, TimeUnit.NANOSECONDS) / 1000);
            throughput = (_streamSize.get() / time) * 1000;  // byte/sec
            LOG.debug("Calculating throughput (received). Stream size: " + _streamSize.get() + ". Msec: " + time);
            _stats.throughputReceived.set(Double.valueOf(throughput).intValue());

            _server.onTestStream(_clientId, _streamSize.get(), _stats);
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            return false;
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
            return false;
        }
        catch (NullPointerException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            return false;
        }

        return true;
    }

    private boolean doOnMessage ()
    {
        byte[] data;
        try {
            if (_server.getProtocol().type.equals(Protocol.Type.UDP)) {
                data = _commHelper.receiveBlob(Server.MAX_MSG_SIZE + UDPCommHelper.MAX_HEADER_SIZE);
            }
            else {
                int dataLength = _commHelper.read32(); //read data length
                data = _commHelper.receiveBlob(dataLength);
            }
            _server.onMessage(_clientId, data);
            _stats.msgReceived.incrementAndGet();

            // _server.getMessages().add(data);
            //_commHelper.sendLine(Request.confirm.toString()); //confirm that data was received successfully
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            return false;
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
            return false;
        }
        catch (NullPointerException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            return false;
        }

        return true;
    }

    private boolean doOnStats ()
    {
        try {
            int msgSent = _commHelper.read32();
            int msgReceived = _commHelper.read32();
            int throughputSent = _commHelper.read32();
            int throughputReceived = _commHelper.read32();

            Stats stats = new Stats(Stats.Type.Client);
            stats.msgSent.set(msgSent);
            stats.msgReceived.set(msgReceived);
            stats.throughputSent.set(throughputSent);
            stats.throughputReceived.set(throughputReceived);

            _server.onStats(_clientId, stats);
            // _server.getMessages().add(data);
            //_commHelper.sendLine(Request.confirm.toString()); //confirm that data was received successfully
        }
        catch (CommException e) {
            LOG.error(ConnErrorHandler.getServerConnectionError(_clientId, _server.getPort()), e);
            return false;
        }
        catch (ProtocolException e) {
            LOG.error(ConnErrorHandler.getProtocolError(), e);
            return false;
        }

        return true;
    }

    /**
     * Disconnects this <code>ServerChild</code> instance
     */
    void disconnectChild ()
    {
        //if TCP control, disconnect any associate UDP connection
        if (_server.getProtocol().type.equals(Protocol.Type.TCP)) {
            LOG.debug("Sending " + RequestType.udpDisconnect + " request to main Server");
            _server.onMessage(_clientId, RequestType.udpDisconnect.toString().getBytes());
        }

        STOP.set(true);

        LOG.debug("Removing " + _server.getProtocol() + " child from Server map");
        _server.getChildren().remove(_clientId);

        if (_commHelper != null) {
            _commHelper.closeConnection();
            LOG.debug("Closed main " + _server.getProtocol() + " CommHelper connection.");
        }
        if (_callbackCommHelper != null) {
            _callbackCommHelper.closeConnection();
            LOG.debug("Closed callback " + _server.getProtocol() + " CommHelper connection.");
        }

        _commHelper = null;
        _callbackCommHelper = null;
    }

    private final Server _server;
    private final short _clientId;
    private final String _clientName;
    private final Stats _stats;
    private final AtomicBoolean _isStream;
    private final AtomicInteger _streamSize;
    private CommHelperAdapter _commHelper;
    private CommHelperAdapter _callbackCommHelper;
    private final static Logger LOG = Logger.getLogger(ServerChild.class);

    AtomicBoolean STOP;
}