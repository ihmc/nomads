package us.ihmc.netutils;

import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.CommHelperInterface;
import us.ihmc.comm.ProtocolException;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.MocketsCommHelper;
import us.ihmc.mockets.SocketAdaptor;
import us.ihmc.netutils.protocol.Protocol;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.*;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * CommHelperAdapter.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class CommHelperAdapter
{
    private CommHelperAdapter (final Protocol protocol)
    {
        _protocol = protocol;
        LOG.info("Instantiating new " + _protocol.type + " CommHelper with " + _protocol.socket);
        switch (protocol.type) {
            case TCP:
                _commHelper = new CommHelper();
                break;
            case Mockets:
                _mocketsCommHelper = ((protocol.socket.equals(Protocol.Socket.Mocket))
                        ? new MocketsCommHelper() : new CommHelper());
                break;
            case UDP:
                _udpCommHelper = new UDPCommHelper();
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + protocol);
        }
    }


    public static CommHelperAdapter newInstance (final Protocol protocol)
    {
        return new CommHelperAdapter(protocol);
    }

    public Protocol getProtocol ()
    {
        return _protocol;
    }

    public Mocket.Statistics getStatistics () throws IOException
    {
        switch (_protocol.type) {
            case Mockets:
                if (_protocol.socket.equals(Protocol.Socket.Mocket)) {
                    return ((MocketsCommHelper) _mocketsCommHelper).getMocket().getStatistics();
                }
                else {
                    throw new IllegalArgumentException("Protocol not supported: " + _protocol);
                }
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }

    }

    public String getRemoteSocketAddress () throws CommException, IOException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return ((InetSocketAddress) _commHelper.getSocket().getRemoteSocketAddress()).getHostString();
            case Mockets:
                if (_protocol.socket.equals(Protocol.Socket.StreamMocket)) {
                    //TODO use StreamMocket's own method by saving a reference to StreamMocket
                    throw new ProtocolException("StreamMocket does not support getRemoteSocketAddress()");
                }
                return intToIp((int) ((MocketsCommHelper) _mocketsCommHelper).getMocket().getRemoteAddress());
            case UDP:
                throw new ProtocolException("UDP does not support getRemoteSocketAddress()");
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public boolean closeConnection ()
    {
        if (!isInitialized())
            return false;

        switch (_protocol.type) {
            case TCP:
                _commHelper.closeConnection();
                break;
            case Mockets:
                _mocketsCommHelper.closeConnection();
                break;
            case UDP:
                _udpCommHelper.closeConnection();
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }

        return true;
    }

    public void init (final Socket socket)
    {
        _commHelper.init(socket);
        _isInitialized.set(true);
        LOG.info("Initialized " + _protocol + " CommHelper");
    }

    public void init (final Mocket mocket)
    {
        if (_protocol.socket.equals(Protocol.Socket.DatagramSocket)) {
            throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
        ((MocketsCommHelper) _mocketsCommHelper).init(mocket);
        _isInitialized.set(true);
        LOG.info("Initialized " + _protocol + " CommHelper");
    }

    public void init (final DatagramSocket dSocket)
    {
        _udpCommHelper.init(dSocket);
        _isInitialized.set(true);
        LOG.info("Initialized " + _protocol + " CommHelper");
    }


    public void init (final InputStream is, final OutputStream os)
    {
        switch (_protocol.type) {
            case TCP:
                _commHelper.init(is, os);
                break;
            case Mockets:
                if (_protocol.socket.equals(Protocol.Socket.Mocket)) {
                    throw new IllegalArgumentException("Protocol not supported: " + _protocol);
                }
                ((CommHelper) _mocketsCommHelper).init(is, os);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }

        _isInitialized.set(true);
        LOG.info("Initialized " + _protocol + " CommHelper through I/O Stream");
    }

    public void init (final DatagramSocket dSocket, String host, int port) throws UnknownHostException
    {
        _udpCommHelper.init(dSocket);
        _udpCommHelper.setInetAddress(host);
        _udpCommHelper.setPort(port);
        _isInitialized.set(true);
        LOG.info("Initialized UDP CommHelper");
    }

    public boolean isInitialized ()
    {
        return _isInitialized.get();
    }

    public void sendLine (final String buf) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                _commHelper.sendLine(buf);
                break;
            case Mockets:
                //TODO


                _mocketsCommHelper.sendLine(buf);
                break;
            case UDP:
                _udpCommHelper.sendLine(buf);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public void sendBlob (byte[] buf) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                _commHelper.sendBlob(buf);
                break;
            case Mockets:
                _mocketsCommHelper.sendBlob(buf);
                break;
            case UDP:
                _udpCommHelper.sendBlob(buf);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    //only available for TCP and Mockets
    public void sendBlob (byte[] buf, int off, int len) throws CommException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                _commHelper.sendBlob(buf, off, len);
                break;
            case Mockets:
                _mocketsCommHelper.sendBlob(buf, off, len);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public byte read8 () throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.read8();
            case Mockets:
                return _mocketsCommHelper.read8();
            case UDP:
                return _udpCommHelper.read8();
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public int read32 () throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.read32();
            case Mockets:
                return _mocketsCommHelper.read32();
            case UDP:
                return _udpCommHelper.read32();
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public byte[] receiveBlob (int bufSize) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.receiveBlob(bufSize);
            case Mockets:
                return _mocketsCommHelper.receiveBlob(bufSize);
            case UDP:
                return _udpCommHelper.receiveBlob(bufSize);
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public String receiveLine () throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.receiveLine();
            case Mockets:
                return _mocketsCommHelper.receiveLine();
            case UDP:
                return _udpCommHelper.receiveLine();
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    //only for TCP and Mockets
    public void receiveMatch (final String matchWith) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                _commHelper.receiveMatch(matchWith);
                break;
            case Mockets:
                _mocketsCommHelper.receiveMatch(matchWith);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    //only for TCP and Mockets
    public String[] receiveParsed () throws CommException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.receiveParsed();
            case Mockets:
                return _mocketsCommHelper.receiveParsed();
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    //only for TCP and Mockets
    public String[] receiveParsedSpecific (String format) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.receiveParsedSpecific(format);
            case Mockets:
                return _mocketsCommHelper.receiveParsedSpecific(format);
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    //only for TCP and Mockets
    public String[] receiveRemainingParsed (final String startsWith) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                return _commHelper.receiveRemainingParsed(startsWith);
            case Mockets:
                return _mocketsCommHelper.receiveRemainingParsed(startsWith);
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    public void write8 (final byte val) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                _commHelper.write8(val);
                break;
            case Mockets:
                _mocketsCommHelper.write8(val);
                break;
            case UDP:
                _udpCommHelper.write8(val);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }


    public void write32 (final int i32Val) throws CommException, ProtocolException
    {
        if (!isInitialized())
            throw new CommException("CommHelperAdapter wasn't initialized correctly");

        switch (_protocol.type) {
            case TCP:
                _commHelper.write32(i32Val);
                break;
            case Mockets:
                _mocketsCommHelper.write32(i32Val);
                break;
            case UDP:
                _udpCommHelper.write32(i32Val);
                break;
            default:
                throw new IllegalArgumentException("Protocol not supported: " + _protocol);
        }
    }

    /**
     * Method for converting an existing IP in Integer format to String format.
     *
     * @param integerIp The IP Address in Integer format
     * @return A String representation of the IP Address
     */
    public static String intToIp (int integerIp)
    {
        return (integerIp & 0xFF) + "." + ((integerIp >> 8) & 0xFF)
                + "." + ((integerIp >> 16) & 0xFF) + "." + ((integerIp >> 24) & 0xFF);
    }

    private final Protocol _protocol;
    private final AtomicBoolean _isInitialized = new AtomicBoolean(false);
    private CommHelper _commHelper;
    private CommHelperInterface _mocketsCommHelper;
    private UDPCommHelper _udpCommHelper;
    private final static Logger LOG = Logger.getLogger(CommHelperAdapter.class);
}
