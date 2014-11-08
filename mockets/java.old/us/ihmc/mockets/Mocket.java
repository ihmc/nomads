package us.ihmc.mockets;

import java.util.Enumeration;
import java.util.Vector;
import java.io.IOException;
import java.net.SocketAddress;
import java.net.SocketException;

public abstract class Mocket
{
    Mocket() 
    {
        _stats = new Statistics();
        _statusListeners = new Vector();
        _uid = UIDGenerator.getUID();
    }
    
    Enumeration getStatusListeners ()
    {
        return _statusListeners.elements();
    }

    /**
     * Adds msl to the internal list of registered status listeners.
     */
    public void addStatusListener (MocketStatusListener msl)
        throws SocketException
    {
        if (msl != null) {
            _statusListeners.add (msl);
        }
    }

    /**
     * Returns the bytes available in the receive buffer.
     */
    public abstract int bytesAvailable()
        throws SocketException;
   
    /**
     * Binds the socket to a local address.
     */   
    public abstract void bind(SocketAddress ma) 
        throws SocketException;
    
    /**
     * Closes this mocket.
     */ 
    public abstract void close()
        throws SocketException;

    /**
     * Connects the mocket to the specified communication endpoint.
     */
    public abstract void connect(SocketAddress ma)
        throws SocketException;
    
    /**
     * Connects the mocket to the specified communication endpoint. Returns an
     * error if the connection is not established after the specified timeout
     * (in milliseconds).
     */
    public abstract void connect(SocketAddress ma, long timeout)
        throws SocketException;
   
    /**
     * Returns the closed state of the Mocket.
     */
    public abstract boolean isClosed()
        throws SocketException;
    
    /**
     * Returns true if this mocket is connected.
     */
    public abstract boolean isConnected()
        throws SocketException;
    
    /**
     * This function sets the default input timeout (in milliseconds)
     * for this mocket.
     */
    public abstract void setReceiveTimeout(long timeout)
        throws SocketException;
    
    /**
     * Returns the statistics associated with this Mocket.
     *
     * @return the statistics
     **/
    public Statistics getStatistics()
    {
        return _stats;
    }

    long getUID() 
    {
        return _uid;
    }

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     * The Statistics class is used to retrieve statistics about the
     * current mocket connection
     */
    public class Statistics
    {
        public int getRetransmittedPacketCount()
        {
            return _retransmittedPackets;
        }

        public int getSentPacketCount()
        {
            return _sentPackets;
        }

        public int getSentByteCount()
        {
            return _sentBytes;
        }

        public int getReceivedPacketCount()
        {
            return _receivedPackets;
        }

        public int getReceivedByteCount()
        {
            return _receivedBytes;
        }

        public int getDiscardedPacketCount()
        {
            return _discardedPackets_Invalid +
                   _discardedPackets_NoRoom +
                   _discardedPackets_Duplicates;
        }

        // TODO: Add methods to fetch transmission quality and round trip times
        
        int _retransmittedPackets;
        int _sentPackets;
        int _sentBytes;
        int _receivedPackets;
        int _receivedBytes;
        int _discardedPackets_Invalid;
        int _discardedPackets_NoRoom;
        int _discardedPackets_Duplicates;
    }
    
    private Vector _statusListeners;    // Vector of MocketStatusListener instances
    private long _uid;                  // The Mocket Unique ID
    protected Statistics _stats;        // Statistics information
}
/*
 * vim: et ts=4 sw=4
 */

