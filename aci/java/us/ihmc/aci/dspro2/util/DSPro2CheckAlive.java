package us.ihmc.aci.dspro2.util;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.disServiceProProxy.MatchmakingLogListener;
import us.ihmc.aci.dspro2.*;
import us.ihmc.comm.CommException;

import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Class <code>DSPro2CheckAlive</code> periodically checks the status of the connection between DSProProxy and DSPro
 */
public class DSPro2CheckAlive implements Runnable
{
    private DSPro2CheckAlive (final DSProProxyInterface dsPro, final String host, final int port, DSProProxyListener dsProListener,
                              SearchListener searchListener, UndecryptableMessageListener undecryptableMessageListener, 
                              MatchmakingLogListener matchmakingLogListener, ConnectionEventsListener connectionEventsListener, 
                              boolean registerUserId, String userId)
    {
        _dspro = dsPro;
        _dsProHost = host;
        _dsProPort = port;
        _dsProListener = dsProListener;
        _searchListener = searchListener;
        _undecryptableMessageListener = undecryptableMessageListener;
        _matchmakingLogListener = matchmakingLogListener;
        _registerUserId = registerUserId;
        _userId = userId;

        _connectionEventsListeners = new CopyOnWriteArrayList<ConnectionEventsListener>();
        if (connectionEventsListener != null) {
            _connectionEventsListeners.add (connectionEventsListener);
        }

        _isDsProRegistered = false;
        _isSearchListenerRegistered = false;
        _isMatchmakingLogListenerRegistered = false;
        _isUndListenerRegistered = false;
        _connAttempts = 0;  
    }
    public DSPro2CheckAlive (final DSProProxyInterface dsPro, final String host, final int port, DSProProxyListener dsProListener,
                             SearchListener searchListener, MatchmakingLogListener matchmakingLogListener,
                             ConnectionEventsListener connectionEventsListener, boolean registerUserId, String userId)
    {
        this (dsPro, host, port, dsProListener, searchListener, null, matchmakingLogListener, connectionEventsListener,
                registerUserId, userId);  
    }

    public DSPro2CheckAlive (final DSProProxyInterface dsPro, final String host, final int port, DSProProxyListener dsProListener,
                             SearchListener searchListener, ConnectionEventsListener connectionEventsListener,
                             boolean registerUserId, String userId)
    {
        this (dsPro, host, port, dsProListener, searchListener, null, null, connectionEventsListener,
                registerUserId, userId);
    }
    
    public DSPro2CheckAlive (final DSProProxyInterface dsPro, final String host, final int port, DSProProxyListener dsProListener,
                             SearchListener searchListener, UndecryptableMessageListener undecryptableMessageListener, 
                             ConnectionEventsListener connectionEventsListener, boolean registerUserId, String userId)
    {
        this (dsPro, host, port, dsProListener, searchListener, undecryptableMessageListener, null, connectionEventsListener,
                registerUserId, userId);
    }

    /**
     * Starts the main thread
     */
    public void start()
    {
        Thread thread = new Thread (this);
        thread.setName (THREAD_NAME);
        thread.setDaemon (true);
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
                try {
                    listener.notifyConnection();
                }
                catch (Exception e) {
                    log.error ("Problem notifying the connection event listener");
                }
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
     * Registers the new <code>DSProProxyListener</code> with dspro2
     */
    public void registerDSProProxyListener(DSProProxyListener newDSProProxyListener)
    {
        deregisterDSProProxyListener();

        _isDsProRegistered = false;
        _dsProListener = newDSProProxyListener;

        registerDSProProxyListener();
    }

    /**
     * Registers the <code>DSProProxyListener</code> with dspro2
     */
    private void registerDSProProxyListener()
    {
        if (_dsProListener == null || _isDsProRegistered)
            return;

        if (! _dspro.isInitialized())
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
            try {
                listener.notifyListenerRegistration (DSProProxyListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
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
            try {
                listener.notifyListenerUnregistration (DSProProxyListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
        }
    }

    /**
     * Registers the new <code>SearchListener</code> with dspro2
     */
    public void registerSearchListener(SearchListener newSearchListener)
    {
        deregisterSearchListener();

        _isSearchListenerRegistered = false;
        _searchListener = newSearchListener;

        registerSearchListener();
    }

    /**
     * Registers the <code>SearchListener</code> with dspro2
     */
    private void registerSearchListener()
    {
        if (_searchListener == null || _isSearchListenerRegistered)
            return;

        if (! _dspro.isInitialized())
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
            try {
                listener.notifyListenerRegistration (SearchListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
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
            try {
                listener.notifyListenerUnregistration (SearchListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
        }
    }

    private void registerMatchMakingLogListener(MatchmakingLogListener newMatchmakingLogListener)
    {
        deregisterMatchMakingLogListener();

        _isMatchmakingLogListenerRegistered = false;
        _matchmakingLogListener = newMatchmakingLogListener;

        registerMatchMakingLogListener();
    }

    private void registerMatchMakingLogListener()
    {
        if (_matchmakingLogListener == null || _isMatchmakingLogListenerRegistered)
            return;

        try {
            int rc = _dspro.registerMatchmakingLogListener (_matchmakingLogListener);
            if (rc >=0) {
                log.info (String.format ("%s registered successfully to %s", DSProProxy.class.getSimpleName(),
                        MatchmakingLogListener.class.getSimpleName()));
            }
            else {
                log.info ("Return value for registerMatchMakingLogListener: " + rc);
                return;
            }
        }
        catch (CommException e) {
            log.warn (String.format ("Unable to register %s to %s", DSProProxy.class.getSimpleName(),
                    MatchmakingLogListener.class.getSimpleName()));
            return;
        }

        _isMatchmakingLogListenerRegistered = true;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            try{
                listener.notifyListenerRegistration (MatchmakingLogListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
        }
    }

    /**
     * Deregisters the <code>SearchListener</code>
     */
    public synchronized void deregisterMatchMakingLogListener()
    {
        if (_matchmakingLogListener == null) {
            _isMatchmakingLogListenerRegistered = false;
            return;
        }

        try {
            _dspro.deregisterMatchmakingLogListener(0, _matchmakingLogListener);
            log.info (String.format ("%s deregistered successfully to %s", DSProProxy.class.getSimpleName(),
                    MatchmakingLogListener.class.getSimpleName()));
        }
        catch (CommException e) {
            log.warn (String.format ("The connection with dspro is down, removed the %s form the %s",
                    MatchmakingLogListener.class.getSimpleName(), DSProProxy.class.getSimpleName()));
        }

        _isMatchmakingLogListenerRegistered = false;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            listener.notifyListenerUnregistration (MatchmakingLogListener.class.getSimpleName());
        }
    }

    /**
     * Registers the new <code>UndecryptableMessageListener</code> with dspro2
     * @param listener new listener instance
     */
    public void registerUndecryptableMessageListener (UndecryptableMessageListener listener)
    {
        deregisterUndecryptableMessageListener();

        _isUndListenerRegistered = false;
        _undecryptableMessageListener = listener;

        registerUndecryptableMessageListener();
    }

    /**
     * Registers the <code>UndecryptableMessageListener</code> with dspro2
     */
    private void registerUndecryptableMessageListener()
    {
        if (_undecryptableMessageListener == null || _isUndListenerRegistered)
            return;

        if (! _dspro.isInitialized())
            return;

        if (!(_dspro instanceof ABEDSProProxy)) {
            log.info ("Not registering the " + UndecryptableMessageListener.class.getSimpleName() + " because the " +
                    "proxy is not an instance of " + ABEDSProProxy.class.getSimpleName());
            return;
        }

        try {
            int rc = ((ABEDSProProxy) _dspro).registerUndecryptableMessageListener (_undecryptableMessageListener);
            if (rc >=0) {
                log.info (String.format ("%s registered successfully to %s", DSProProxy.class.getSimpleName(),
                        UndecryptableMessageListener.class.getSimpleName()));
            }
            else {
                log.info ("Return value for registerUndecryptableMessageListener: " + rc);
                return;
            }
        }
        catch (Exception e) {
            log.warn (String.format ("Unable to register %s to %s", DSProProxy.class.getSimpleName(),
                    UndecryptableMessageListener.class.getSimpleName()));
            return;
        }

        _isUndListenerRegistered = true;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            try {
                listener.notifyListenerRegistration (UndecryptableMessageListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
        }
    }

    /**
     * Deregistering the <code>UndecryptableMessageListener</code>
     */
    public synchronized void deregisterUndecryptableMessageListener()
    {
        if (_undecryptableMessageListener == null) {
            _isUndListenerRegistered = false;
            return;
        }

        if (!(_dspro instanceof ABEDSProProxy)) {
            log.info ("Not deregistering the " + UndecryptableMessageListener.class.getSimpleName() + " because the " +
                    "proxy is not an instance of " + ABEDSProProxy.class.getSimpleName());
            return;
        }

        try {
            ((ABEDSProProxy) _dspro).deregisterUndecryptableMessageListener (_undecryptableMessageListener);
            log.info (String.format ("%s deregistered successfully to %s", DSProProxy.class.getSimpleName(),
                    UndecryptableMessageListener.class.getSimpleName()));
        }
        catch (CommException e) {
            log.warn (String.format ("The connection with dspro is down, removed the %s form the %s",
                    UndecryptableMessageListener.class.getSimpleName(), DSProProxy.class.getSimpleName()));
        }

        _isUndListenerRegistered = false;
        for (ConnectionEventsListener listener : _connectionEventsListeners) {
            try {
                listener.notifyListenerUnregistration (UndecryptableMessageListener.class.getSimpleName());
            }
            catch (Exception e) {
                log.error ("Problem notifying the connection event listener");
            }
        }
    }

    /**
     * Registers the user id with DSPro
     */
    public void registerUserId()
    {
        if (_userId != null) {
            try {
                _dspro.addUserId (_userId);
                for (ConnectionEventsListener listener : _connectionEventsListeners) {
                    try {
                        listener.notifyUserIdRegistration (_userId);
                    }
                    catch (Exception e) {
                        log.error ("Problem notifying the connection event listener");
                    }
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
        boolean wasInitialized = _dspro.isInitialized();
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
                        registerMatchMakingLogListener();
                        registerUndecryptableMessageListener();
                        wasInitialized = true;
                    }
                    else {
                        if (wasInitialized) {
                            for (ConnectionEventsListener listener : _connectionEventsListeners) {
                                try {
                                    listener.notifyDisconnection();
                                }
                                catch (Exception e) {
                                    log.error ("Problem notifying the connection event listener");
                                }
                            }
                        }
                        wasInitialized = false;
                        log.debug(DSPro2CheckAlive.class.getSimpleName() + ": DSPro is not connected");
                        if (_isDsProRegistered) {
                            deregisterDSProProxyListener();
                        }
                        if (_isSearchListenerRegistered) {
                            deregisterSearchListener();
                        }
                        if (_isMatchmakingLogListenerRegistered) {
                            deregisterMatchMakingLogListener();
                        }
                        if (_isUndListenerRegistered) {
                            deregisterUndecryptableMessageListener();
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
        deregisterMatchMakingLogListener();
        deregisterUndecryptableMessageListener();
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

    //private final DSProProxy _dspro;
    private final DSProProxyInterface _dspro;
    private DSProProxyListener _dsProListener;
    private SearchListener _searchListener;
    private UndecryptableMessageListener _undecryptableMessageListener;
    private MatchmakingLogListener _matchmakingLogListener;
    private final CopyOnWriteArrayList<ConnectionEventsListener> _connectionEventsListeners;
    private long _connAttempts;
    private final String _dsProHost;
    private final int _dsProPort;
    private final boolean _registerUserId;
    private final String _userId;
    private boolean _isDsProRegistered;
    private boolean _isSearchListenerRegistered;
    private boolean _isMatchmakingLogListenerRegistered;
    private boolean _isUndListenerRegistered;
    private boolean _terminate = false;
    private static final String THREAD_NAME = "DSProCheckAliveThread";

    private static final int TIMEOUT = 2000;

    private static final Logger log = LoggerFactory.getLogger (DSPro2CheckAlive.class);
}
