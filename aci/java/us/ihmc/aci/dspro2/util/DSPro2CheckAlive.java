package us.ihmc.aci.dspro2.util;

import us.ihmc.aci.dspro2.DSProProxy;
import us.ihmc.aci.dspro2.DSProProxyListener;
import us.ihmc.comm.CommException;

import java.util.ArrayList;
import java.util.List;
import us.ihmc.aci.dspro2.SearchListener;

/**
 * Class <code>DSPro2CheckAlive</code> periodically checks the status of the connection between DSProProxy and DSPro.
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class DSPro2CheckAlive implements Runnable
{
    public DSPro2CheckAlive (final DSProProxy dsPro, final String host, final int port, DSProProxyListener dsProListener,
                             SearchListener searchListener, ConnectionEventsListener connectionEventsListener,
                             boolean registerUserId, String userId)
    {
        _dspro = dsPro;
        _dsProHost = host;
        _dsProPort = port;
        _dsProListener = dsProListener;
        _searchListener = searchListener;
        _registerUserId = registerUserId;
        _userId = userId;

        _connectionEventsListeners = new ArrayList<ConnectionEventsListener>();
        if (connectionEventsListener != null) {
            _connectionEventsListeners.add (connectionEventsListener);
        }

        _isDsProRegistered = false;
        _isSearchListenerRegistered = false;
        _connAttempts = 0;

    }

    /**
     * Starts the main thread
     */
    public void start ()
    {
//        (new Thread (this)).start();
        Thread thread = new Thread (this);
        thread.setName (THREAD_NAME);
        thread.start();
    }

    /**
     * Tries the connection with DSPro2
     * @return true if the connection is established
     */
    private synchronized boolean connect()
    {
        int rc = _dspro.init();
        log.debug ("DSPro init rc: " + rc);
        if (rc == 0) {
            log.info (String.format ("Connection successful to DSPro on %s:%d", _dsProHost, _dsProPort));
            for (ConnectionEventsListener listener : _connectionEventsListeners) {
                listener.notifyConnection();
            }
            return true;
        }
        else {
            if (_connAttempts < 1) {
                log.warn (String.format ("Unable to initialize DSPro on %s:%d", _dsProHost, _dsProPort));
            }
            _connAttempts++;
            return false;
        }
    }

    /**
     * Registers the <code>DSProProxyListener</code> with dspro2
     */
    private void registerDSProProxyListener()
    {
        if (_dsProListener == null || _isDsProRegistered)
            return;

        try {
            int rc = _dspro.registerDSProProxyListener (_dsProListener);
            if (rc >=0) {
                log.info (String.format ("%s registered successfully to %s", DSProProxy.class.getSimpleName(),
                        DSProProxyListener.class.getSimpleName()));
            }
            else {
                log.info ("Return value for registerDSProProxyListener: " + rc);
                return;
            }
        }
        catch (CommException e) {
            log.warn (String.format ("Unable to register %s to %s", DSProProxy.class.getSimpleName(),
                    DSProProxyListener.class.getSimpleName()));
            return;
        }

        _isDsProRegistered = true;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            listener.notifyListenerRegistration (DSProProxyListener.class.getSimpleName());
        }
    }

    /**
     * Deregistering the <code>DSProProxyListener</code>
     */
    public synchronized void deregisterDSProProxyListener()
    {
        if (_dsProListener == null) {
            _isDsProRegistered = false;
            return;
        }

        try {
            _dspro.deregisterDSProProxyListener(_dsProListener);
            log.info (String.format ("%s deregistered successfully to %s", DSProProxy.class.getSimpleName(),
                    DSProProxyListener.class.getSimpleName()));
        }
        catch (CommException e) {
            log.warn (String.format ("The connection with dspro is down, removed the %s form the %s",
                    DSProProxyListener.class.getSimpleName(), DSProProxy.class.getSimpleName()));
        }

        _isDsProRegistered = false;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            listener.notifyListenerUnregistration (DSProProxyListener.class.getSimpleName());
        }
    }

    /**
     * Registers the <code>SearchListener</code> with dspro2
     */
    private void registerSearchListener()
    {
        if (_searchListener == null || _isSearchListenerRegistered)
            return;

        try {
            int rc = _dspro.registerSearchListener (_searchListener);
            if (rc >=0) {
                log.info (String.format ("%s registered successfully to %s", DSProProxy.class.getSimpleName(),
                        SearchListener.class.getSimpleName()));
            }
            else {
                log.info ("Return value for registerSearchListener: " + rc);
                return;
            }
        }
        catch (CommException e) {
            log.warn (String.format ("Unable to register %s to %s", DSProProxy.class.getSimpleName(),
                    SearchListener.class.getSimpleName()));
            return;
        }

        _isSearchListenerRegistered = true;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            listener.notifyListenerRegistration (SearchListener.class.getSimpleName());
        }
    }

    /**
     * Deregisters the <code>SearchListener</code>
     */
    public synchronized void deregisterSearchListener()
    {
        if (_searchListener == null) {
            _isSearchListenerRegistered = false;
            return;
        }

        try {
            _dspro.deregisterSearchListener(_searchListener);
            log.info (String.format ("%s deregistered successfully to %s", DSProProxy.class.getSimpleName(),
                    SearchListener.class.getSimpleName()));
        }
        catch (CommException e) {
            log.warn (String.format ("The connection with dspro is down, removed the %s form the %s",
                    SearchListener.class.getSimpleName(), DSProProxy.class.getSimpleName()));
        }

        _isSearchListenerRegistered = false;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            listener.notifyListenerUnregistration (SearchListener.class.getSimpleName());
        }
    }

    /**
     * Registers the user id with DSPro
     */
    private void registerUserId()
    {
        if (_userId != null) {
            try {
                _dspro.addUserId (_userId);
                for (ConnectionEventsListener listener : _connectionEventsListeners) {
                    listener.notifyUserIdRegistration (_userId);
                }
            }
            catch (CommException e) {
                log.error ("Problem in registering the user id", e);
            }
        }
    }

    @Override
    public void run ()
    {
        while (!_terminate) {
            try {
                Thread.sleep (TIMEOUT); //check if connection with DSPro is alive every TIMEOUT seconds
            }
            catch (InterruptedException e) {}

            try {
                if (!_dspro.isInitialized()) {

                    if (connect()) {
                        if (_registerUserId) {
                            registerUserId();
                        }
                        registerDSProProxyListener();   //optional listeners
                        registerSearchListener();
                    }
                    else {
                        log.debug(DSPro2CheckAlive.class.getSimpleName() + ": DSPro is not connected");
                        if (_isDsProRegistered) {
                            for (ConnectionEventsListener listener : _connectionEventsListeners) {
                                listener.notifyDisconnection();
                            }
                            deregisterDSProProxyListener();
                        }
                        if (_isSearchListenerRegistered) {
                            deregisterSearchListener();
                        }
                    }
                }
            }
            catch (Exception e) {
                log.error ("Error during reconnection:" + e.getMessage());
            }
        }

        log.debug("The thread " + THREAD_NAME + " is not running anymore");
    }

    /**
     * Terminates the thread
     */
    public void terminate()
    {
        log.debug ("Stopping thread: " + THREAD_NAME);
        deregisterDSProProxyListener();
        deregisterSearchListener();
        _terminate = true;
    }

    /**
     * Interface to notify application about the DSPro connection and listeners registration
     */
    public interface ConnectionEventsListener
    {
        public void notifyUserIdRegistration (String userId);

        public void notifyConnection();

        public void notifyDisconnection();

        public void notifyListenerRegistration (String listenerName);

        public void notifyListenerUnregistration (String listenerName);
    }

    private final DSProProxy _dspro;
    private final DSProProxyListener _dsProListener;
    private final SearchListener _searchListener;
    private final List<ConnectionEventsListener> _connectionEventsListeners;
    private long _connAttempts;
    private final String _dsProHost;
    private final int _dsProPort;
    private final boolean _registerUserId;
    private final String _userId;
    private boolean _isDsProRegistered;
    private boolean _isSearchListenerRegistered;
    private boolean _terminate = false;
    private static final String THREAD_NAME = "DSProCheckAliveThread";

    private static final int TIMEOUT = 2000;
    private static final LoggerInterface log = LoggerWrapper.getLogger (DSPro2CheckAlive.class);
}
