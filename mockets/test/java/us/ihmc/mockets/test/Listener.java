package us.ihmc.mockets.test;
/*
 * Listener.java
 *
 * Created on October 4, 2006, 4:07 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
import us.ihmc.mockets.*;
/**
 *
 * @author Bindo
 */
public class Listener  implements MocketStatusListener{
    
    /** Creates a new instance of Listener */
    public Listener() {
    }
    
    public boolean peerUnreachableWarning (long x)
    {
        _closeConn = (x > 30 * 1000);
        System.out.println("Listener :: peerUnreachableWarning :: " + x);
        if (_closeConn) {
            System.out.println ("Listener :: MSL >> connection inactive for 30 seconds, will disconnect.");
        }

        return _closeConn;
    }

    public boolean peerReachable (long x)
    {
        System.out.println("Listener :: The peer is reachable again :: "+x);

        return true;
    }

    public boolean suspendReceived (long x)
    {
        System.out.println("Listener :: a suspension has been called from the peer. Communication suspended.");

        return false;
    }
    
    private boolean _closeConn;
    
}
