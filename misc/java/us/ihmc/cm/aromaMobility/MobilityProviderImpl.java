package us.ihmc.cm.aromaMobility;

import java.io.Serializable;
import java.util.Hashtable;
import java.util.Vector;

import us.ihmc.cm.*;
import us.ihmc.cm.example.*;
import us.ihmc.nomads.Agent;
import us.ihmc.nomads.StrongMobilityService;
import us.ihmc.util.URI;

public class MobilityProviderImpl implements MobilityProvider
{
    public MobilityProviderImpl ()
    {
        _moverThread = new Mover (this);
    }

    public MobilityProviderImpl (RovingMind rovMind)
    {
        _rovMind = rovMind;
        _moverThread = new Mover (this);
    }

    
    // ======= These are the Mobility provider interface methods
    public RovingMind getRovingMind ()
    {
        return _rovMind;
    }
    
    public void setRovingMind (RovingMind rv)
    {
        _rovMind = rv;
    }
    
    public void addBotPart (BotPart bp)
    {
        if (bp == null) throw new IllegalArgumentException ("Null botpart");
        String bpuri = bp.getURI();
        if (bpuri == null) throw new IllegalArgumentException ("Null botpart URI");
        _moverThread.addBotPart (bpuri);
    }

    public void removeBotPart (String bpuri)
    {
        if (bpuri == null) throw new IllegalArgumentException ("Null botpart URI");
        _moverThread.removeBotPart (bpuri);
    }
    
    public String getCurrentBotPart ()
    {
        return _moverThread._currentBotPart;
    }

    public void setTimeSlice (long t)
    {
        _moverThread._timeslice = t;
    }
    
    public long getTimeSlice ()
    {
        return _moverThread._timeslice;
    }
    
    public Object getSynchronizationToken ()
    {
        return _token;
    }
    
    public void start ()
    {
        _moverThread.start ();
        msg ("Mover thread has been started");
    }
    
    //*!!*// Take into consideration this optimization is ok if the roving mind
    // runs with just one thread, but might cause some potential problem
    // if it runs with 2 or more threads
    public void moveRovMindAsap ()
    {
        /*
        synchronized (_token2) {
            _token2.notifyAll ();
        }
        */
        // Using just one token again
        synchronized (_token) {
            _token.notifyAll ();
        }
    }

    // ==============================================================

    private class Mover extends Thread
    {
        public Mover (MobilityProviderImpl mpi)
        {
            _mpi = mpi;
            _timeslice = 50;
            _botParts = new Vector();
            _currentBotPart = "";
            _stop = false;
        }

        public void addBotPart (String uri)
        {
            _botParts.addElement (uri);
        }
        
        public void removeBotPart (String uri)
        {
            _botParts.removeElement (uri);
        }
        
        public void setTimeslice (long ms)
        {
            _timeslice = ms;
        }
        
        public void terminate()
        {
            _stop = true;
        }
        
        public void run()
        {
            int i = 0;
            int iterationCount = 0;
            long iterationStartTime = System.currentTimeMillis();
            String nextBotPart = null;
            while (!_stop) {
                try {
                    //Thread.sleep (_timeslice);
                    //synchronized (_token2) {
                    //    _token2.wait (_timeslice);
                    //}
                    synchronized (_token) {
                        _token.wait (_timeslice);
                    }
                    nextBotPart = null;
                    if (i < _botParts.size()) {
                        nextBotPart = (String) _botParts.elementAt (i);
                    }
                    else {
                        iterationCount++;
                        long currentTime = System.currentTimeMillis();
                        msg ("\n\nIteration Time = " + (currentTime - iterationStartTime)+"\n\n");
                        iterationStartTime = currentTime;
                        i = 0;
                        if (_botParts.size() == 0) {
                            break;
                        }
                        nextBotPart = (String) _botParts.elementAt (i);
                        msg ("About to move to "+nextBotPart);
                    }
                    synchronized (_token) {
                        URI uri = new URI (nextBotPart);
                        // Call go in mobility service of the movility provider
                        Launcher._launcher.getStrongMobilityService().go (uri.getHost(), uri.getPort());
                        _currentBotPart = nextBotPart;
                        msg ("Iteration " + iterationCount);
                        _token.notifyAll ();
                    }
                    i++;
                }
                catch (Exception e) {
                    e.printStackTrace();
                    msg ("At BotPart #"+i+"("+nextBotPart+" of BotParts: "+_botParts);
                }
            }
        }
        
        // Member variables of inner class Mover
        MobilityProviderImpl _mpi;
        long    _timeslice;
        Vector  _botParts;
        String  _currentBotPart;
        boolean _stop;
    }
    
    
    public static void msg (String msg)
    {
        System.out.println ("MobProvImpl: "+msg);
    }

    
    private   RovingMind _rovMind;
    private   Mover      _moverThread;
    private   String     _token = "token";
    //protected String     _token2 = "token2";
}
