package us.ihmc.netutils.protocol;

import org.apache.commons.lang3.builder.EqualsBuilder;
import org.apache.commons.lang3.builder.HashCodeBuilder;

import java.net.DatagramSocket;

/**
 * Protocol.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Protocol
{
    public Protocol (final Type type, final Socket socket)
    {
        this.type = type;
        this.socket = socket;
    }

    @Override
    public String toString ()
    {
        return type + separator + socket;
    }

    public enum Type
    {
        TCP,
        Mockets,
        UDP
    }

    @Override
    public int hashCode ()
    {
        return new HashCodeBuilder(17, 31). //two randomly chosen prime numbers
                append(type).
                append(socket).
                hashCode();
    }

    @Override
    public boolean equals (final Object o)
    {
        if (!(o instanceof Protocol))
            return false;

        Protocol p = (Protocol) o;
        return new EqualsBuilder().
                append(type, p.type).
                append(socket, p.socket).
                isEquals();
    }

    public static Protocol valueOf (String name)
    {
        if (name.equalsIgnoreCase("TCP") || name.equalsIgnoreCase("TCP" + separator + "Socket")) {
            return TCP;
        }
        else if (name.equalsIgnoreCase("Mockets") || name.equalsIgnoreCase("Mockets" + separator + "Mocket")) {
            return Mockets;
        }
        else if (name.equalsIgnoreCase("StreamMockets") || name.equalsIgnoreCase("Mockets" + separator +
                "StreamMocket")) {
            return StreamMockets;
        }
        else if (name.equalsIgnoreCase("UDP") || name.equalsIgnoreCase("UDP" + separator + "DatagramSocket")) {
            return UDP;
        }
        else {
            throw new IllegalArgumentException("Unrecognized protocol " + name);
        }
    }

    public enum Socket
    {
        Socket,
        DatagramSocket,
        Mocket,
        MulticastSocket,
        StreamMocket
    }

    public final Type type;
    public final Socket socket;
    public final static String separator = "-";

    public final static Protocol TCP = new Protocol(Type.TCP, Socket.Socket);
    public final static Protocol Mockets = new Protocol(Type.Mockets, Socket.Mocket);
    public final static Protocol StreamMockets = new Protocol(Type.Mockets, Socket.StreamMocket);
    public final static Protocol UDP = new Protocol(Type.UDP, Socket.DatagramSocket);
    public final static Protocol UDPMulticast = new Protocol(Type.UDP, Socket.MulticastSocket);
}


