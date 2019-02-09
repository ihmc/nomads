package us.ihmc.cm;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.Serializable;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Vector;

public abstract class RovingMind implements Runnable
{
    public abstract void run();
    
    public abstract void init();
    
    public RovingMind ()
    {
    }
    
    public final void initProviders (String mobProvider, String platProvider)
    {
        msg ("Initializing providers:");
        try {
            msg ("Mobility provider: "+mobProvider);
            Class mobProvClass = Class.forName (mobProvider);
            _mobilityProvider = (MobilityProvider) mobProvClass.newInstance ();
            _mobilityProvider.setRovingMind (this);
            _token = _mobilityProvider.getSynchronizationToken ();
        }
        catch (Exception xcp) {
            xcp.printStackTrace ();
            msg ("Could not instantiate ["+mobProvider+"]");
        }
        
        try {
            msg ("Platform provider: "+platProvider);
            Class platProvClass = Class.forName (platProvider);
            _platformProvider = (PlatformProvider) platProvClass.newInstance ();
            _platformProvider.setRovingMind (this);
        }
        catch (Exception xcp) {
            xcp.printStackTrace ();
            msg ("Could not instantiate ["+platProvider+"]");
        }
    }
    
    public final void startMobilityProvider ()
    {
        msg ("Starting mobility provider");
        if (_mobilityProvider != null) {
            _mobilityProvider.start ();
        }
        else {
            msg ("Mob.provider is null ???!");
        }
    }
    
    public final void addBotPart (BotPart bp)
    {
        if (bp == null) throw new IllegalArgumentException ("Null botpart object");
        String bpuri = bp.getURI();
        if (bpuri == null) throw new IllegalArgumentException ("Null botpart URI");
        bp.setRovingMindRef (this);
        _botPartMap.put (bpuri, bp);
        if (_platformProvider != null) {
            _platformProvider.addBotPart (bp);
        }
        if (_mobilityProvider != null) {
            _mobilityProvider.addBotPart (bp);
        }
    }

    public final void removeBotPart (String bpuri)
    {
        if (bpuri == null) throw new IllegalArgumentException ("Null botpart URI");
        _botPartMap.remove (bpuri);
        if (_platformProvider != null) {
            _platformProvider.removeBotPart (bpuri);
        }
        if (_mobilityProvider != null) {
            _mobilityProvider.removeBotPart (bpuri);
        }
    }

    // Get BotPart values using platform provider
    public final Serializable getBotPartValue (String botPartURI, String attr)
    {
        Serializable result = null;
        synchronized (_token) {
            waitToReachBotPart (botPartURI);
            if (_platformProvider != null) {
                result = _platformProvider.getBotPartValue (botPartURI, attr); // Should not need to pass in botPartURI
            }
            _token.notifyAll();
            return result;
        }
    }

    // Set BotPart values using platform provider
    public final void setBotPartValue (String botPartURI, String attr, Serializable value)
    {
        synchronized (_token) {
            waitToReachBotPart (botPartURI);
            if (_platformProvider != null) {
                _platformProvider.setBotPartValue (botPartURI, attr, value);  // Should not need to pass in botPartURI
            }
            _token.notifyAll ();
            return;
        }
    }

    // Execute a runnable when getting to a specific botPart
    public final void executeOnBotPartSync (String botPartURI, Runnable runnable)
    {
        if (runnable == null) {
            throw new IllegalArgumentException ("Null runnable");
        }
        synchronized (_token) {
            waitToReachBotPart (botPartURI);
            runnable.run ();
            _token.notifyAll ();
        }
    }

    private void waitToReachBotPart (String uri)
    {
        if ((uri == null) || (_botPartMap.get (uri) == null)) {
            throw new IllegalArgumentException ("Invalid Bot part uri: "+uri);
        }
        synchronized (_token) {
            while (!_mobilityProvider.getCurrentBotPart().equals (uri)) {
                //*!!*// Notice this optimization is ok if the roving mind
                // runs with just one thread, but might cause some potential problem
                // if it runs with 2 or more threads
                if (FastForwardEnabled) {
                    _mobilityProvider.moveRovMindAsap ();
                }
                try {
                    _token.wait ();
                }
                catch (Exception e) {}
            }
        }
    }
    
    public final void setTimeslice (long ms)
    {
        if (_mobilityProvider != null) {
            _mobilityProvider.setTimeSlice (ms);
        }
    }

    public final MobilityProvider getMobilityProvider ()
    {
        return _mobilityProvider;
    }
    
    public final PlatformProvider getPlatformProvider ()
    {
        return _platformProvider;
    }
    
    private static void msg (String s)
    {
        System.out.println ("RovingMind: "+s);
    }
    
    // Member variables of RovingMind:
    public static boolean FastForwardEnabled = true;
                                                     
    private MobilityProvider _mobilityProvider = null;
    private PlatformProvider _platformProvider = null;
    private Hashtable _botPartMap = new Hashtable();     // Key is a URI, value is a BotPart instance
    //private Properties _properties;
    private Object _token;
}
