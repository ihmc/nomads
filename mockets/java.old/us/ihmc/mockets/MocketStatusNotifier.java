package us.ihmc.mockets;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

/**
 * Sends out statistics about open connections to the MocketStatusMonitor
 * @author  nsuri
 */
public class MocketStatusNotifier
{
    /**
     * Construct a mocket status notifier object
     *
     * @param mocket         the mocket represented by this notifier
     * @param clientSide     true if this is the client side (i.e., the mocket that originated the connection)
     *                       and false if this is the server side (i.e., the mocket that was created in response
     *                       to an incoming connection)
     * @param localPort      the local port number
     * @param remoteAddress  the remote IP address
     * @param remotePort     the remote port number
     */
    public MocketStatusNotifier (StreamMocket mocket)
        throws SocketException
    {
        _mocket = mocket;

        _statSocket = new DatagramSocket();
        try {
            _statPacket = new DatagramPacket (new byte[0], 0, 0, InetAddress.getByName("127.0.0.1"), StreamMocket.STATS_PORT);
        }
        catch (UnknownHostException e) {
            throw new RuntimeException ("failed to get InetAddress for localhost");
        }
        _statBuf = new AccessibleByteArrayOutputStream();
        _statWriter = new OutputStreamWriter (_statBuf);
    }
    
    public void sendConnectionFailed (InetAddress remoteAddress, int remotePort)
        throws IOException
    {
        _statBuf.reset();
        _statWriter.write ("ConnectionFailedInfo\r\n");
        _statWriter.write ("LocalIP=" + extractIPAddr (InetAddress.getLocalHost().getHostAddress()) + "\r\n");
        _statWriter.write ("RemoteIP=" + extractIPAddr (remoteAddress.getHostAddress() + "\r\n"));
        _statWriter.write ("RemotePort=" + remotePort + "\r\n");
        _statWriter.flush();
        _statPacket.setData (_statBuf.getBuffer(), 0, _statBuf.size());
        _statSocket.send (_statPacket);
    }

    public void connected (boolean clientSide, int localPort, InetAddress remoteAddress, int remotePort)
    {
        _clientSide = clientSide;
        try {
            _localEndPointInfo = "LocalIP=" + extractIPAddr (InetAddress.getLocalHost().getHostAddress());
        }
        catch (UnknownHostException e) {
            throw new RuntimeException ("failed to get local InetAddress");
        }
        _localEndPointInfo += "\r\n";
        _localEndPointInfo += "LocalPort=" + localPort;
        _localEndPointInfo += "\r\n";
        _remoteEndPointInfo = "RemoteIP=" + extractIPAddr (remoteAddress.getHostAddress());
        _remoteEndPointInfo += "\r\n";
        _remoteEndPointInfo += "RemotePort=" + remotePort;
        _remoteEndPointInfo += "\r\n";
    }

    public void setTimeSinceLastContact (long timeSinceLastContact)
    {
        _timeSinceLastContact = timeSinceLastContact;
    }

    public void sendStats()
        throws IOException
    {
        StreamMocket.Statistics stats = _mocket.getStatistics();
        _statBuf.reset();
        _statWriter.write ("OpenConnectionInfo\r\n");
        _statWriter.write (_localEndPointInfo);
        _statWriter.write (_remoteEndPointInfo);
        _statWriter.write ("TimeSinceLastContact=" + _timeSinceLastContact);
        _statWriter.write ("\r\n");
        _statWriter.write ("BytesSent=" + stats.getSentByteCount());
        _statWriter.write ("\r\n");
        _statWriter.write ("PacketsSent=" + stats.getSentPacketCount());
        _statWriter.write ("\r\n");
        _statWriter.write ("PacketsRetransmitted=" + stats.getRetransmitCount());
        _statWriter.write ("\r\n");
        _statWriter.write ("BytesReceived=" + stats.getReceivedByteCount());
        _statWriter.write ("\r\n");
        _statWriter.write ("PacketsReceived=" + stats.getReceivedPacketCount());
        _statWriter.write ("\r\n");
        _statWriter.write ("PacketsDiscarded=" + stats.getDiscardedPacketCount());
        _statWriter.write ("\r\n");
        _statWriter.flush();
        _statPacket.setData (_statBuf.getBuffer(), 0, _statBuf.size());
        _statSocket.send (_statPacket);
    }

    private String extractIPAddr (String ipAddr)
    {
        if (ipAddr.indexOf('/') >= 0) {
            return ipAddr.substring (ipAddr.indexOf('/') + 1);
        }
        else {
            return ipAddr;
        }
    }

    private static class AccessibleByteArrayOutputStream extends ByteArrayOutputStream
    {
        public byte[] getBuffer()
        {
            return buf;
        }
    }

    private StreamMocket _mocket;
    private boolean _clientSide;
    private DatagramSocket _statSocket;
    private DatagramPacket _statPacket;
    private AccessibleByteArrayOutputStream _statBuf;
    private OutputStreamWriter _statWriter;
    private String _localEndPointInfo;
    private String _remoteEndPointInfo;

    private long _timeSinceLastContact;
}
