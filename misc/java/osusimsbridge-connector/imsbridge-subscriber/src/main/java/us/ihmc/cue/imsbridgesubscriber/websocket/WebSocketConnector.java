package us.ihmc.cue.imsbridgesubscriber.websocket;


import mil.dod.th.core.log.Logging;
import org.java_websocket.drafts.Draft_6455;
import org.osgi.service.log.LogService;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * The {@link WebSocketConnector} class takes care of opening a <code>WebSocketImpl</code> to receive data from
 * an endpoint.
 */
public class WebSocketConnector
{
    /**
     * Constructor
     */
    public WebSocketConnector (Endpoint endpoint) {
        _endpoint = endpoint;
    }

    public WebSocketImpl getSocket () {
        return _socketClient;
    }

    public void updateHostPort (String host, int port) {
        setHost(host);
        setPort(port);

        updateSubString();
    }

    public boolean isConnected () {
        if (_socketClient != null) {
            return _socketClient.isOpen();
        }

        return false;
    }

    /**
     * Called with host and port arguments for the location of the Federation instance to connect to
     */
    public void connect () {

        if (_socketClient.isOpen()) {
            // Close the session before opening a new web socket
            endConnection();
        }
        connectAsync(false);
    }

    public void reconnect(){
        connectAsync(true);
    }

    public void createSocket () {
        try {
            _socketClient = new WebSocketImpl(new URI(_subURLString), new Draft_6455());
        } catch (URISyntaxException e) {
            e.printStackTrace();
        }
    }

    /**
     * Gets the host
     *
     * @return host
     */
    public String getHost () {
        return _host;
    }

    /**
     * Sets the host
     *
     * @param host host IP
     */
    public void setHost (String host) {
        _host = host;
    }

    /**
     * Gets the port
     *
     * @return the port
     */
    public int getPort () {
        return _port;
    }

    /**
     * Sets the port
     *
     * @param port the port
     */
    public void setPort (int port) {
        _port = port;
    }

    /**
     * Get the subscription URL
     *
     * @return Subscription URL
     */
    public String getSubURLString () {
        return _subURLString;
    }

    /**
     * Set the URL to use to connect to the Federation instance
     *
     * @param subURLString
     */
    public void setSubURLString (String subURLString) {
        _subURLString = subURLString;
    }

    /**
     * Closes the web socket. If the connection thread is running, it interrupts and calls <code>join()</code> to wait
     * for the thread to finish processing. The websocket is then closed if it's open.
     */
    public void endConnection () {
        _terminate = true;
        // Stop the connect thread
        if (_connectionThread != null) {
            _connectionThread.interrupt();
            try {
                _connectionThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        _terminate = false;

        // Close the session before opening a new web socket
        if (_socketClient != null && _socketClient.isOpen()) {
            try {
                _socketClient.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void setEndpoint (Endpoint endpoint) {
        _endpoint = endpoint;
    }

    /**
     * Connect to the URL on a separate thread.
     * In case the URL is not responsive it won't hold up the rest of the program
     */
    private void connectAsync (boolean reconnect) {
        _connectionThread = new Thread(() -> {
            boolean connected = false;
            while (!connected && !_terminate) {
                if (_socketClient == null) {
                    break;
                }

                try {
                    if (reconnect) {
                        connected = reconnectSocket();
                    }
                    else {
                        connected = connectSocket();
                    }

                    if (connected) {
                        Logging.log(LogService.LOG_INFO, "Connection made");
                    }

                } catch (Exception e) {
                    if (e.getMessage() != null) {
                        Logging.log(LogService.LOG_ERROR, e.getMessage());
                    }
                    connected = false;
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e1) {
                        e1.printStackTrace();
                    }
                }
            }
        }, "Federation subscriber connecting thread");
        _connectionThread.start();
    }

    private boolean reconnectSocket() throws InterruptedException{
        Logging.log(LogService.LOG_INFO, "Trying to reconnect to websocket: " + _subURLString);
        return _socketClient.reconnectBlocking();
    }

    private boolean connectSocket() throws InterruptedException {
        Logging.log(LogService.LOG_INFO, "Trying to connect to websocket: " + _subURLString);
        return _socketClient.connectBlocking();
    }

    /**
     * Updates the subscription string. Internal call that prepends the websocket protocol, inserts a colon between the
     * host and port, and appends the endpoint for the websocket.
     */
    private void updateSubString () {
        setSubURLString("ws://" + _host + ":" + _port + _endpoint.toString());
    }

    private String _host;
    private int _port;
    private String _subURLString;
    private Thread _connectionThread;
    private Endpoint _endpoint;
    private WebSocketImpl _socketClient;
    private boolean _terminate = false;
}
