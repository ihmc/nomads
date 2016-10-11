package us.ihmc.netutils;

import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.ByteConverter;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

/**
 * UDPCommHelper.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class UDPCommHelper
{

    public UDPCommHelper ()
    {
        _socket = null;
    }

    public UDPCommHelper (DatagramSocket dSocket)
    {
        _socket = dSocket;
    }

    public static byte[] getHandshake ()
    {
        byte[] handshake = new byte[9];

        handshake[0] = 'h';
        handshake[1] = 'a';
        handshake[2] = 'n';
        handshake[3] = 'd';
        handshake[4] = 's';
        handshake[5] = 'h';
        handshake[6] = 'a';
        handshake[7] = 'k';
        handshake[8] = 'e';

        return handshake;
    }

    public void closeConnection ()
    {
        if (_socket != null) _socket.disconnect();
        if (_socket != null) _socket.close();
        _socket = null;
    }

    public boolean init (DatagramSocket dSocket)
    {
        _socket = dSocket;
        return true;
    }

    public DatagramSocket getSocket ()
    {
        return _socket;
    }

    public int getRemotePort ()
    {
        return _port;
    }

    public void setInetAddress (final String host) throws UnknownHostException
    {
        _address = InetAddress.getByName(host);
    }

    public void setPort (final int port)
    {
        _port = port;
    }

    public void sendLine (final String buf) throws CommException, ProtocolException
    {
        sendBlob(buf.getBytes());
    }

    public void sendBlob (byte[] buf) throws CommException, ProtocolException
    {
        if (buf.length > Server.MAX_MSG_SIZE) {
            throw new ProtocolException("Unable to send blobs of size > of " + Server.MAX_MSG_SIZE);
        }

        if (_socket == null) throw new CommException("DatagramSocket is null");

        ByteBuffer byteBuf = ByteBuffer.allocate(Server.MAX_MSG_SIZE + MAX_HEADER_SIZE);
        byteBuf.putLong(UDPCommHelper.SIGNATURE);
        byteBuf.putInt(buf.length);
        byteBuf.put(buf);
        DatagramPacket p = new DatagramPacket(byteBuf.array(), byteBuf.array().length, _address, _port);

        System.out.println("MSG SIZE + HEADER: " + byteBuf.array().length);
        try {
            _socket.send(p);
        }
        catch (IOException e) {
            throw new CommException("Unable to send UDP packet", e);
        }

    }

    public byte read8 () throws CommException, ProtocolException
    {
        byte[] buf = receiveBlob(1);

        short value = ByteConverter.from1ByteToUnsignedByte(buf);
        if (value > Byte.MAX_VALUE) {
            throw new ProtocolException ("Conversion from 'short' to 'byte' led to loss of data");
        }

        return (byte) value;
    }

    public int read32 () throws CommException, ProtocolException
    {
        byte[] buf = receiveBlob(4);

        long value = ByteConverter.from4BytesToUnsignedInt(buf, 0);
        if (value > Integer.MAX_VALUE) {
            throw new ProtocolException("Conversion from 'long' to 'int' led to loss of data");
        }

        return (int) value;
    }

    public byte[] receiveBlob (int bufSize) throws CommException, ProtocolException
    {
        if (bufSize > Server.MAX_MSG_SIZE + MAX_HEADER_SIZE) {
            throw new ProtocolException("Unable to send UDP message of size > of " + (Server.MAX_MSG_SIZE + MAX_HEADER_SIZE));
        }

        if (_socket == null) throw new CommException("DatagramSocket is null");

        byte[] buffer = new byte[bufSize];
        DatagramPacket rPacket = new DatagramPacket(buffer, buffer.length);
        try {
            _socket.receive(rPacket);
        }
        catch (IOException e) {
            throw new CommException("Unable to receive incoming UDP packet");
        }

        byte[] data = rPacket.getData();
        ByteBuffer dataBuf = ByteBuffer.wrap(data);
        long signature = dataBuf.getLong();
        if (signature != SIGNATURE) {
            throw new ProtocolException("Received packet with wrong signature, discarding packet");
        }
        int dataSize = dataBuf.getInt();
        byte[] message = new byte[dataSize];
        dataBuf.get(message, 0, dataSize);

        return message;
    }

    public String receiveLine () throws CommException, ProtocolException
    {
        byte[] buffer = receiveBlob(Server.MAX_MSG_SIZE + MAX_HEADER_SIZE);

        int i;
        for (i = 0; i < buffer.length; i++) {
            if (buffer[i] == '\0')
                break;
        }

        String line;
        try {
            line = new String(buffer, 0, i, "UTF-8");
        }
        catch (UnsupportedEncodingException e) {
            throw new CommException("UTF-8 encoding not supported, can't receive line", e);
        }

        return line;
    }

    public void write32 (int i32Val) throws CommException, ProtocolException
    {
        byte[] buf = new byte[4];
        ByteConverter.fromUnsignedIntTo4Bytes(i32Val, buf, 0);
        sendBlob(buf);
    }

    public void write8 (byte val) throws CommException, ProtocolException
    {
        byte[] buf = new byte[]{val};
        sendBlob(buf);
    }


    private InetAddress _address;
    private int _port;
    private DatagramSocket _socket;

    public static final int MAX_HEADER_SIZE = 12;
    public static final String DEFAULT_MULTICAST_ADDRESS = "239.0.0.239";
    public static final long SIGNATURE = 990990990;
}
