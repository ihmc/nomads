package us.ihmc.mockets;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketAddress;

import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.util.ByteConverter;

/**
 * The StreamServerMocket class represents an endpoint for mockets that is capable
 * of receiving incoming connections. This class is designed to be a drop-in
 * replacement for the Socket class in the Java Platform API.
 *
 * @author Niranjan Suri
 * @author Maggie Breedy
 * @author Marco Arguedas
 * @version $Revision: 1.1 $
 */
public class StreamServerMocket
{
    /**
     * Construct a new server mocket that listens for incoming connections on
     * the specified SocketAddress.
     *
     * @param port      the port number to use.
     *
     * @exception IOException     if the specified port is in use.
     */
    public StreamServerMocket (SocketAddress sockAddress)
        throws IOException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        try {
            _serverSocket = new DatagramSocketWrapper (sockAddress);
            _port = _serverSocket.getLocalPort();
        }
        catch (IOException ex) {
            throw ex;
        }

        this.initSynHistoryArray();
    }

    /**
     * Construct a new server mocket that listens for incoming connections on
     * the specified port.
     *
     * @param port      the port number to use.
     *
     * @exception IOException     if the specified port is in use.
     */
    public StreamServerMocket (int port)
        throws IOException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        try {
            _serverSocket = new DatagramSocketWrapper (port);

            // read the port number out of the Socket once it has been initialized
            // because if the port given by whoever call the constructor was 0,
            // the Socket chose a random port number.
            _port = _serverSocket.getLocalPort();
        }
        catch (IOException ex) {
            throw ex;
        }

        this.initSynHistoryArray();
    }

    /**
     * Accept a new connection and return an instance of StreamMocket that represents
     * the local endpoint for the new connection. The method will block until a
     * new connection is received.
     *
     * @return a new StreamMocket instance that represents the new connection
     *
     * @exception IOException     if there was a problem in accepting the connection
     */
    public StreamMocket accept()
        throws IOException
    {
        while (true) {
            int rc;
            InetAddress remoteAddr;
            DatagramPacket dpIn = new DatagramPacket (new byte [StreamPacket.getHeaderSize()],
                                                      StreamPacket.getHeaderSize());
            _serverSocket.receive(dpIn);
            rc = dpIn.getLength();
            remoteAddr = dpIn.getAddress();
            int remotePort = dpIn.getPort();

            if (rc <= 0) {
                _logger.log(Level.WARNING, "StreamServerMocket::accept >> failed to receive a packet; "
                           + "rc = " + rc + "\n");
                // Since this should not have failed - return a failure instead of trying again
                return null;
            }
            else if (rc != StreamPacket.getHeaderSize()) {
                _logger.log(Level.WARNING, "StreamServerMocket::accept >> received a short packet; "
                           + "rc = " + rc + "; PACKET_HEADER_SIZE = " + StreamPacket.getHeaderSize() + "\n");
                continue;
            }
            _logger.log(Level.INFO, "StreamServerMocket::accept >> get a packet\n");

            StreamPacket incomingPacket = new StreamPacket(dpIn.getData());

            if (incomingPacket.getSYNBit()) {
                _logger.log(Level.FINE, "StreamServerMocket::accept >> got a SYN packet\n");
                // Check the SYN History
                SynRec synRec = this.getSynRec (remoteAddr, remotePort);
                if (synRec == null) {
                    // There is no room to cache information about this new incoming connection
                    // Ignore
                    _logger.log(Level.WARNING, "StreamServerMocket::accept >> no room in the syn history "
                               + "buffer for incomming connection from " + remoteAddr + ":" + remotePort);

                    continue;
                }
                if (synRec._count == 0) {
                    // This is the first SYN packet received from the remote endpoint
                    // Create the necessary local endpoint
                    DatagramSocketWrapper dgSocket = new DatagramSocketWrapper(0);
                    int localPort = dgSocket.getLocalPort();
                    if (localPort == 0) {
                        _logger.log(Level.WARNING, "StreamServerMocket::accept>> could not get local port\n");
                        continue;
                    }
                    _logger.log(Level.FINE, "StreamServerMocket::accept >> local port = "
                               + localPort + "\n");

                    byte[] localPortData = new byte [2];
                    ByteConverter.fromUnsignedShortIntTo2Bytes (localPort, localPortData, 0);

                    StreamPacket replyPacket = new StreamPacket();
                    replyPacket.setSYNAckBit();
                    replyPacket.setPayload(localPortData, 0, localPortData.length);

                    DatagramPacket dpOut = new DatagramPacket (replyPacket.getPacket(), replyPacket.getPacket().length,
                                                               remoteAddr, remotePort);
                    _serverSocket.send (dpOut);
                    rc = dpOut.getLength();
                    if (rc <= 0) {
                        _logger.log(Level.WARNING, "StreamServerMocket::accept>> failed to send reply "
                                   + "packet; rc = " + rc + "\n");
                        // Since this should not have failed - return a failure instead of trying again
                        return null;
                    }
                    else if (rc != replyPacket.getPacketSize()) {
                        _logger.log(Level.WARNING, "StreamServerMocket::accept>> failed to send full "
                                   + "packet; rc = " + rc + "; size = " + replyPacket.getPacketSize() + "\n");
                        // Since this should not have failed - return a failure instead of trying again
                        return null;
                    }

                    synRec._lastSynTime = System.currentTimeMillis();
                    synRec._count++;
                    synRec._localPort = localPort;

                    StreamMocket mocket = new StreamMocket (dgSocket, remoteAddr, remotePort);
                    return mocket;
                }
                else {
                    // We have already received a packet from this remote endpoint
                    // Send a reply packet with the port that was already allocated
                    if (synRec._count > 10) {
                        _logger.log(Level.WARNING, "StreamServerMocket::accept>> received "
                                   + synRec._count + " SYN packets from " + remoteAddr
                                   + ":" + remotePort);
                    }
                    else if (synRec._count > 3) {
                        _logger.log(Level.FINE, "StreamServerMocket::accept>> received "
                                   + synRec._count + " SYN packets from " + remoteAddr
                                   + ":" + remotePort);
                    }

                    byte[] localPortData = new byte [2];
                    ByteConverter.fromUnsignedShortIntTo2Bytes (synRec._localPort, localPortData, 0);

                    StreamPacket replyPacket = new StreamPacket();
                    replyPacket.setSYNAckBit();
                    replyPacket.setPayload(localPortData, 0, localPortData.length);

                    DatagramPacket dpOut = new DatagramPacket (replyPacket.getPacket(), replyPacket.getPacket().length,
                                                               remoteAddr, remotePort);
                    _serverSocket.send (dpOut);
                    rc = dpOut.getLength();
                    if (rc <= 0) {
                        _logger.log(Level.WARNING, "StreamServerMocket::accept>>  failed "
                                   + "to send reply packet; rc = "+ rc + "\n");
                        // Since this should not have failed - return a failure instead of trying again
                        return null;
                    }
                    else if (rc != replyPacket.getPacketSize()) {
                        _logger.log(Level.WARNING, "StreamServerMocket::accept>> failed to send full "
                                   + "packet; rc = " + rc + "; size = " + replyPacket.getPacketSize() + "\n");
                        return null;
                    }
                    synRec._lastSynTime = System.currentTimeMillis();
                    synRec._count++;
                }
            }
        }
    }

    /**
     * Returns the port on which this socket is listening.
     */
    public int getLocalPort()
    {
        return _port;
    }

    /**
     *  Close the server mocket and no longer accept new incoming connections.
     */
    public void close()
    {
        _serverSocket.close();
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private SynRec getSynRec (InetAddress addr, int port)
    {
        // See if there is an active record for the specified remote address
        long currTime = System.currentTimeMillis();
        for (int i = 0; i < StreamMocket.SYN_HISTORY_SIZE; i++) {
            SynRec sr = _synHistory[i];
            if (sr._inUse) {
                if ((currTime - sr._lastSynTime) > StreamMocket.SYN_VALIDITY_WINDOW) {
                    // This SYN record is too old - delete
                    sr._inUse = false;
                    sr._lastSynTime = 0;
                    sr._count = 0;
                    sr._localPort = 0;
                }
                else if ((sr._remoteAddr != null) && (sr._remoteAddr.equals(addr)) &&
                         (sr._remotePort == port)) {
                    return sr;
                }
            }
        }

        // There is no active record - return a new blank one
        for (int i = 0; i < StreamMocket.SYN_HISTORY_SIZE; i++) {
            SynRec sr = _synHistory[i];
            if (!sr._inUse) {
                // Found an empty record
                sr._inUse = true;
                sr._lastSynTime = 0;
                sr._remoteAddr = addr;
                sr._remotePort = port;
                sr._count = 0;
                sr._localPort = 0;

                return sr;
            }
        }

        // There is no room for another record
        return null;
    }

    /**
     *
     */
    private void initSynHistoryArray()
    {
        _synHistory = new SynRec[StreamMocket.SYN_HISTORY_SIZE];

        for (int i = 0; i < _synHistory.length; i++) {
            SynRec sr = new SynRec();
            _synHistory[i] = sr;
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    private class SynRec
    {
        SynRec()
        {
            _inUse = false;
            _lastSynTime = 0;
            _count = 0;
            _localPort = 0;
            _remoteAddr = null;
            _remotePort = 0;
        }

        boolean _inUse;
        InetAddress _remoteAddr;
        int _remotePort;
        long _lastSynTime;
        int _count;
        int _localPort;
    }

    // /////////////////////////////////////////////////////////////////////////
    protected DatagramSocketWrapper _serverSocket;
    protected int _port;

    private SynRec[] _synHistory;
    private Logger _logger;
}
