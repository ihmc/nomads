package us.ihmc.aci.disServiceProProxy.util;

import us.ihmc.aci.disServiceProProxy.DisServiceProProxy;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyInterface;
import us.ihmc.aci.disServiceProProxy.DisServiceProProxyListener;
import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyListener;
import us.ihmc.comm.CommException;

import java.util.logging.Logger;


/**
 * DSProCheckAlive.java
 * <p/>
 * Class <code>DSProCheckAlive</code> periodically checks the status of the connection between DSProProxy and DSPro.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DSProCheckAlive implements Runnable
{
    public DSProCheckAlive (final DisServiceProProxyInterface dsPro, final String host, final int port,
                            DisServiceProProxyListener dsProListener, final DisseminationServiceProxyListener
            dsListener)
    {
        _dsPro = dsPro;
        _dsProHost = host;
        _dsProPort = port;
        _dsProListener = dsProListener;
        _dsListener = dsListener;
        _isDsProRegistered = false;
        _isDsRegistered = false;
        _connAttempts = 0;

    }

    public void start ()
    {
        (new Thread(this)).start();
    }

    public void registerDisServiceProProxyListener ()
    {
        if (_dsProListener == null || _isDsProRegistered)
            return;

        try {
            _dsPro.registerDisServiceProProxyListener(_dsProListener);
            log.info(String.format("%s registered successfully to %s", DisServiceProProxy.class.getSimpleName()
                    , DisServiceProProxyListener.class.getSimpleName()));
        }
        catch (CommException e) {
            log.warning(String.format("Unable to register %s to %s", DisServiceProProxy.class.getSimpleName(),
                    DisServiceProProxyListener.class.getSimpleName()));
            return;
        }

        _isDsProRegistered = true;
    }

    public void registerDisseminationServiceProxyListener ()
    {
        if (_dsListener == null || _isDsRegistered)
            return;

        try {
            _dsPro.registerDisseminationServiceProxyListener(_dsListener);
            log.info(String.format("%s registered successfully to %s", DisServiceProProxy.class.getSimpleName()
                    , DisseminationServiceProxyListener.class.getSimpleName()));
        }
        catch (CommException e) {
            log.warning(String.format("Unable to register %s to %s", DisServiceProProxy.class.getSimpleName(),
                    DisseminationServiceProxyListener.class.getSimpleName()));
            return;
        }

        _isDsRegistered = true;
    }

    private boolean connect ()
    {
        try {
            _dsPro.init();
            log.info(String.format("Connection successful to DSPro on %s:%d", _dsProHost, _dsProPort));
            return true;
        }
        catch (Exception e) {
            if (_connAttempts < 1) {
                log.warning(String.format("Unable to initialize DSPro on %s:%d", _dsProHost, _dsProPort));
            }
            _connAttempts++;
            return false;
        }
    }


    @Override
    public void run ()
    {
        do {
            try {
                Thread.sleep(TIMEOUT); //check if connection with DSPro is alive every TIMEOUT seconds
            }
            catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (!_dsPro.isInitialized()) {

                if (connect()) {
                    registerDisseminationServiceProxyListener(); //optional listeners
                    registerDisServiceProProxyListener();   //optional listeners
                }
            }
        }
        while (true);
    }

    private final DisServiceProProxyInterface _dsPro;
    private final DisseminationServiceProxyListener _dsListener;
    private final DisServiceProProxyListener _dsProListener;
    private long _connAttempts;
    private final String _dsProHost;
    private final int _dsProPort;
    private boolean _isDsRegistered;
    private boolean _isDsProRegistered;

    private static final int TIMEOUT = 2000;
    private static final Logger log = Logger.getLogger(DSProCheckAlive.class.toString());
}
