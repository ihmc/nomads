package us.ihmc.aci.nodemon.proxy;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Node;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.serialization.SerializationException;

import java.io.IOException;
import java.net.Socket;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * BaseNodeMonProxy.java
 * <p/>
 * Class <code>BaseNodeMonProxy</code> contains the public API to connect to a local or remote <code>NodeMon</code>
 * instance through a TCP connection.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class BaseNodeMonProxy implements NodeMonProxy, NodeMonProxyListener
{

    public BaseNodeMonProxy (final short clientId, final String clientName, final String host, final int port)
    {
        _clientId = Objects.requireNonNull(clientId, "ClientId can't be null");
        _clientName = Objects.requireNonNull(clientName, "ClientName can't be null");
        _host = Objects.requireNonNull(host, "Host can't be null");
        _port = port;
        _isConnected = new AtomicBoolean(false);
        _listeners = new ArrayList<>();
    }

    /**
     * Connects this <code>BaseNodeMonProxy</code> to the <code>NodeMon</code> instance running at
     * <code>host</code> and <code>port</code> specified in the constructor.
     *
     * @return <code>true</code> if the <code>connect()</code> was successful, <code>false</code> otherwise.
     */
    @Override
    public synchronized boolean connect ()
    {
        int rc;
        log.trace("Before connection to: " + _host + ":" + _port);
        CommHelper ch = connectToServer(_host, _port);
        log.trace("After connection to: " + _host + ":" + _port);
        if (ch == null) {
            return false;
        }

        log.trace("Before callback connection to: " + _host + ":" + _port);
        CommHelper chCallback = connectToServer(_host, _port);
        log.trace("After callback connection to: " + _host + ":" + _port);
        if (chCallback == null) {
            return false;
        }
        log.trace("Before registering proxy with " + _clientId + " client name " + _clientName);
        rc = registerProxy(ch, chCallback, _clientId, _clientName);
        log.trace("After registering proxy with " + _clientId + " client name " + _clientName);
        if (rc >= 0) {
            _clientId = (short) rc; //the server may have assigned a different id than requested
            _commHelper = ch;
            try {
                _callbackHandler = new NodeMonProxyCallbackHandler(this, chCallback);
            }
            catch (SerializationException e) {
                log.error(NodeMonProxyErrorHandler.getSerializationError(_clientId), e);
                return false;
            }
            (new Thread(_callbackHandler)).start();
            _isConnected.set(true);
            return true;
        }

        return false;
    }

    /**
     * Creates an instance of <code>CommHelper</code> which opens a socket connection with the <code>host</code> and
     * <code>port</code> specified
     *
     * @param host hostname
     * @param port binding port
     * @return the <code>CommHelper</code> instance
     */
    private CommHelper connectToServer (final String host, final int port)
    {
        try {
            log.trace("Crated CommHelper");
            CommHelper commHelper = new CommHelper();
            log.trace("Crated TCP Socket");
            Socket socket = new Socket(host, port);
            socket.setTcpNoDelay(true);
            log.trace("Initializing CommHelper with TCP Socket");
            commHelper.init(socket);
            log.trace("Initialized CommHelper!");
            return commHelper;
        }
        catch (IOException e) {
            log.debug(NodeMonProxyErrorHandler.getConnectionError(_host, _port));
            return null;
        }
    }

    /**
     * Registers the proxy and the callback with the proxy server
     *
     * @param ch                <code>CommHelper</code> instance representing the proxy
     * @param chCallback        <code>CommHelper</code> instance representing the proxy callback
     * @param desiredClientId   client id to be assigned to the proxy (if possible)
     * @param desiredClientName client name to be assigned to the proxy
     * @return the actual id assigned to the client
     */
    private int registerProxy (final CommHelper ch, final CommHelper chCallback, final int desiredClientId,
                               final String desiredClientName)
    {
        try {
            log.trace("Before sending line : " + Request.registerProxy.toString());
            // Registering the proxy using the desired app id
            ch.sendLine(String.format("%s %d %s", Request.registerProxy.toString(), desiredClientId,
                    desiredClientName));
            log.trace("After sending line : " + Request.registerProxy.toString());
            // The proxy returns the assigned app id
            log.trace("Before receiving confirm : " + Request.confirm.toString());
            String[] strArray = ch.receiveRemainingParsed(Request.confirm.toString());
            log.trace("After receiving confirm : " + Request.confirm.toString());
            int clientId = Short.parseShort(strArray[0]);

            // Registering the callback using the assigned app id
            log.trace("Before sending line : " + Request.registerProxyCallback.toString());
            chCallback.sendLine(String.format("%s %d %s", Request.registerProxyCallback.toString(), clientId, desiredClientName));
            log.trace("After sending line : " + Request.registerProxyCallback.toString());
            log.trace("Before receiving confirm : " + Request.confirm.toString());
            chCallback.receiveMatch(Request.confirm.toString());
            log.trace("After receiving confirm : " + Request.confirm.toString());
            return clientId;
        }
        catch (CommException e) {
            _isConnected.set(false);
            log.debug(NodeMonProxyErrorHandler.getRegistrationError(_host, _port), e);
            return -1;
        }
        catch (ProtocolException e) {
            log.debug(NodeMonProxyErrorHandler.getProtocolError(_host, _port), e);
            return -2;
        }
    }

    @Override
    public boolean isConnected ()
    {
        return _isConnected.get();
    }


    void notifyCallbackHandlerStop ()
    {
        _isConnected.set(false);
        synchronized (_listeners) {
            for (NodeMonProxyListener listener : _listeners) {
                listener.connectionClosed();
            }
        }
    }

    @Override
    public void registerNodeMonProxyListener (NodeMonProxyListener listener)
    {
        synchronized (_listeners) {
            _listeners.add(listener);
            log.debug("Registered listener of type NodeMonProxyListener");
        }

    }

    @Override
    public boolean getWorldState ()
    {
        if (!isConnected()) {
            log.debug("Proxy not connected");
            return false;
        }

        try {
            _commHelper.sendLine(Request.worldState.toString());
            _commHelper.receiveMatch(Request.confirm.toString());
            log.debug("Sent " + Request.worldState + " request to NodeMon");
            return true;
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getConnectionError(_host, _port), e);
            _isConnected.set(false);
            return false;
        }
        catch (ProtocolException e) {
            log.error(NodeMonProxyErrorHandler.getProtocolError(_host, _port), e);
            return false;
        }
    }

    @Override
    public void worldStateUpdate (Collection<Node> worldState)
    {

        if (_listeners.isEmpty()) {
            log.error(NodeMonProxyErrorHandler.getNoListenersError());
            return;
        }

        synchronized (_listeners) {
            for (NodeMonProxyListener listener : _listeners) {
                listener.worldStateUpdate(worldState);
            }
        }
    }

    @Override
    public void updateData (String nodeId, byte[] data)
    {

        if (_listeners.isEmpty()) {
            log.error(NodeMonProxyErrorHandler.getNoListenersError());
            return;
        }

        synchronized (_listeners) {
            for (NodeMonProxyListener listener : _listeners) {
                listener.updateData(nodeId, data);
            }
        }
    }

    @Override
    public void connectionClosed ()
    {
        if (_listeners.isEmpty()) {
            log.error (NodeMonProxyErrorHandler.getNoListenersError());
            return;
        }

        synchronized (_listeners) {
            for (NodeMonProxyListener listener : _listeners) {
                listener.connectionClosed();
            }
        }
    }


    private short _clientId;
    private final String _clientName;
    private final String _host;
    private final int _port;
    private final AtomicBoolean _isConnected;

    private final List<NodeMonProxyListener> _listeners;

    private CommHelper _commHelper;
    private NodeMonProxyCallbackHandler _callbackHandler;

    private static final Logger log = Logger.getLogger(BaseNodeMonProxy.class);


}
