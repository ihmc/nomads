package us.ihmc.mockets;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketAddress;
import java.net.SocketException;

import java.util.logging.Logger;

import us.ihmc.manet.MANETDatagramSocket;

/**
 * DatagramSocketWrapper wraps the DatagramSocket and the MANETDatagramSocket
 * classes so that the Mockets API can use either implementation.
 *
 * If the system property use.manet is defined, then the wrapper will resort to
 * using the MANETDatagramSocket class
 *
 */
public class DatagramSocketWrapper
{
    public DatagramSocketWrapper()
        throws SocketException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        if (_useMANET) {
            _manetDGSocket = new MANETDatagramSocket();
            _logger.info ("Using MANET");
        }
        else {
            _dgSocket = new DatagramSocket();
            _logger.info ("Using UDP");
        }
    }

    public DatagramSocketWrapper (int port)
        throws SocketException
    {
        if (_useMANET) {
            _manetDGSocket = new MANETDatagramSocket (port);
        }
        else {
            _dgSocket = new DatagramSocket (port);
        }
    }

    public DatagramSocketWrapper (SocketAddress addr)
        throws SocketException
    {
        if (_useMANET) {
            _manetDGSocket = new MANETDatagramSocket (addr);
        }
        else {
            _dgSocket = new DatagramSocket (addr);
        }
    }

    public void bind (SocketAddress addr)
        throws SocketException
    {
        if (_dgSocket != null) {
            _dgSocket.bind (addr);
        }
        else {
            //_manetDGSocket.bind (addr);
        }
    }

    public void close()
    {
        if (_dgSocket != null) {
            _dgSocket.close();
        }
        else {
            _manetDGSocket.close();
        }
    }
    
    public void receive (DatagramPacket p)
        throws IOException
    {
        if (_dgSocket != null) {
            _dgSocket.receive (p);
        }
        else {
            _manetDGSocket.receive (p);
        }
    }

    public void send (DatagramPacket p)
        throws IOException
    {
        if (_dgSocket != null) {
            _dgSocket.send (p);
        }
        else {
            _manetDGSocket.send (p);
        }
    }

    public void setReceiveBufferSize (int size)
        throws SocketException
    {
        if (_dgSocket != null) {
            _dgSocket.setReceiveBufferSize (size);
        }
        else {
            //_manetDGSocket.setReceiveBufferSize (size);
        }
    }

    public int getReceiveBufferSize()
        throws SocketException
    {
        if (_dgSocket != null) {
            return _dgSocket.getReceiveBufferSize();
        }
        else {
            //return _manetDGSocket.getReceiveBufferSize();
            return 0;
        }        
    }

    public void setSendBufferSize (int size)
        throws SocketException
    {
        if (_dgSocket != null) {
            _dgSocket.setSendBufferSize (size);
        }
        else {
            //_manetDGSocket.setSendBufferSize (size);
        }
    }

    public int getSendBufferSize()
        throws SocketException
    {
        if (_dgSocket != null) {
            return _dgSocket.getSendBufferSize();
        }
        else {
            //return _manetDGSocket.getSendBufferSize();
            return 0;
        }        
    }

    public InetAddress getLocalAddress()
    {
        if (_dgSocket != null) {
            return _dgSocket.getLocalAddress();
        }
        else {
            //return _manetDGSocket.getLocalAddress();
            return null;
        }
    }

    public int getLocalPort()
    {
        if (_dgSocket != null) {
            return _dgSocket.getLocalPort();
        }
        else {
            return _manetDGSocket.getLocalPort();
        }
    }

    public SocketAddress getLocalSocketAddress()
    {
        if (_dgSocket != null) {
            return _dgSocket.getLocalSocketAddress();
        }
        else {
            //return _manetDGSocket.getLocalSocketAddress();
            return null;
        }
    }

    public int getSoTimeout ()
        throws SocketException
    {
        if (_dgSocket != null) {
            return _dgSocket.getSoTimeout();
        }
        else {
            return _manetDGSocket.getSoTimeout();
        }
    }

    public void setSoTimeout (int timeout)
        throws SocketException
    {
        if (_dgSocket != null) {
            _dgSocket.setSoTimeout (timeout);
        }
        else {
            _manetDGSocket.setSoTimeout (timeout);
        }
    }

    private DatagramSocket _dgSocket;
    private MANETDatagramSocket _manetDGSocket;
    private Logger _logger;

    private static boolean _useMANET = false;

    static {
        if (System.getProperty ("use.manet") != null) {
            _useMANET = true;
        }
    }
}
