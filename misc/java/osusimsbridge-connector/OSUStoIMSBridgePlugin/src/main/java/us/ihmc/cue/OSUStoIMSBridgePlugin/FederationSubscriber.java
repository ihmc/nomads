package us.ihmc.cue.OSUStoIMSBridgePlugin;

import mil.dod.th.core.log.Logging;
import org.osgi.service.log.LogService;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * The FederationSubscriber class takes care of opening a <code>FederationWebSocket</code> to receive data from
 * the Federation endpoint.
 */
public class FederationSubscriber
{
    /**
     * Constructor
     * @param nameMsg Name message to send to the Federation instance
     * @param subsMsg Subscription message to send the Federation instance
     * @param handler Handler to call when a message is received
     */
    public FederationSubscriber (String nameMsg, String subsMsg, StringHandler handler) {
        _nameMessage = nameMsg;
        _subsMessage = subsMsg;
        _stringHandler = handler;
    }

    /**
     * Default main for testing the subscriber
     * @param args Command line arguments
     */
    public static void main (String[] args) {
        FederationSubscriber subscriber = new FederationSubscriber("{\n\"name\": \"OSUStoIMSBridge\"}", "{\n" +
                " \"add\": [" +
                "\"trackInfo\", \"image\", \"K05196017b\", \"acknowledgement\", \"request\", \"osus\"" +
                "]\n" +
                "}", System.out::println);
        try {
            subscriber.connect("10.100.0.228", 4567);
            new Thread(() -> {
                try {
                    Thread.sleep(10000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }).start();
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    /**
     * Called with host and port arguments for the location of the Federation instance to connect to
     * @param host IP address of the Federation instance
     * @param port Port of the Federation instance
     */
    public void connect (String host, int port) {
        setHost(host);
        setPort(port);
        updateSubString();

        // Close the session before opening a new web socket
        endConnection();

        URI uri = null;
        try {
            uri = new URI(_subURLString);
        } catch (URISyntaxException e) {
            e.printStackTrace();
        }
        if (uri == null){
            Logging.log(LogService.LOG_ERROR, "URI could not be created???");
            return;
        }

        connectAsync(uri);

    }

    public boolean isConnected(){
        if (_socketClient == null){
            return false;
        }

        return _socketClient.isOpen();
    }

    /**
     * Connect to the URL on a separate thread.
     * In case the URL is not responsive it won't hold up the rest of the program
     */
    public void connectAsync (final URI uri) {
        Logging.log(LogService.LOG_INFO, "Connect async called");
        _connectionThread = new Thread(() -> {
            boolean connected = false;

            while (!connected && !_terminate) {
                Logging.log(LogService.LOG_DEBUG,"Trying to connect to websocket: " + _subURLString);
                try {
                    _socketClient = new FederationWebSocket(uri, _nameMessage, _subsMessage, _stringHandler, 5000);

                    connected = _socketClient.connectBlocking();
                    if (connected) {
                        Logging.log(LogService.LOG_INFO,"Connected to Federation");
                    }

                } catch (Exception e) {
                    if (e.getMessage() != null) {
                        Logging.log(LogService.LOG_WARNING, e.getMessage());
                    }
                    connected = false;
                }

                if (!connected){
                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException e1) {
                        Logging.log(LogService.LOG_DEBUG, "Sleep interrupted");
                    }
                }
            }
        }, "Federation subscriber connecting thread");
        _connectionThread.start();
    }

    public String getHost () {
        return _host;
    }

    public void setHost (String host) {
        _host = host;
    }

    public int getPort () {
        return _port;
    }

    public void setPort (int port) {
        _port = port;
    }

    /**
     * Get the subscription URL
     * @return Subscription URL
     */
    public String getSubURLString () {
        return _subURLString;
    }

    /**
     * Set the URL to use to connect to the Federation instance
     * @param subURLString
     */
    public void setSubURLString (String subURLString) {
        _subURLString = subURLString;
    }

    public void endConnection () {
        _terminate = true;
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

    private void updateSubString () {
        setSubURLString("ws://" + _host + ":" + _port + "/subscribe");
    }

    private String _host;
    private int _port;
    private String _subURLString;
    private Thread _connectionThread;
    private StringHandler _stringHandler;

    private FederationWebSocket _socketClient;
    private String _nameMessage;
    private String _subsMessage;
    private boolean _terminate = false;
}
