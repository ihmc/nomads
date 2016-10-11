package us.ihmc.netutils.ssl;

import us.ihmc.mockets.StreamMocket;

import java.io.IOException;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

/**
 * StreamMocketFactory.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public abstract class StreamMocketFactory
{
    //
    // NOTE:  JDK 1.1 bug in class GC, this can get collected
    // even though it's always accessible via getDefault().
    //
    private static StreamMocketFactory theFactory;

    /**
     * Creates a <code>SocketFactory</code>.
     */
    protected StreamMocketFactory ()
    { /* NOTHING */ }


    /**
     * Returns a copy of the environment's default <code>StreamMocket</code> factory.
     *
     * @return the default <code>SocketFactory</code>
     */
    public static StreamMocketFactory getDefault ()
    {
        synchronized (StreamMocketFactory.class) {
            if (theFactory == null) {
                //
                // Different implementations of this method SHOULD
                // work rather differently.  For example, driving
                // this from a system property, or using a different
                // implementation than JavaSoft's.
                //
                theFactory = new DefaultStreamMocketFactory();
            }
        }

        return theFactory;
    }


    /**
     * Creates an unconnected <code>StreamMocket</code>.
     *
     * @return the unconnected <code>StreamMocket</code>
     * @throws IOException if the <code>StreamMocket</code> cannot be created
     */
    public StreamMocket createStreamMocket () throws IOException
    {
        throw new SocketException("Unconnected Mockets not implemented");
    }


    /**
     * Creates a <code>StreamMocket</code> and connects it to the specified remote host
     * at the specified remote port.  This <code>StreamMocket</code> is configured using
     * the <code>StreamMocket</code> options established for this factory.
     *
     * @param host the server host
     * @param port the server port
     * @return the <code>Socket</code>
     * @throws IOException          if an I/O error occurs when creating the <code>StreamMocket</code>
     * @throws UnknownHostException if the host is not known
     */
    public abstract StreamMocket createStreamMocket (String host, int port)
            throws IOException, UnknownHostException;


    /**
     * Creates a <code>StreamMocket</code> and connects it to the specified remote host
     * on the specified remote port.
     * The <code>StreamMocket</code> will also be bound to the local address and port supplied.
     * This <code>StreamMocket</code> is configured using
     * the <code>StreamMocket</code> options established for this factory.
     *
     * @param host      the server host
     * @param port      the server port
     * @param localHost the local address the <code>StreamMocket</code> is bound to
     * @param localPort the local port the <code>StreamMocket</code> is bound to
     * @return the <code>Socket</code>
     * @throws IOException          if an I/O error occurs when creating the <code>StreamMocket</code>
     * @throws UnknownHostException if the host is not known
     */
    public abstract StreamMocket
    createStreamMocket (String host, int port, InetAddress localHost, int localPort)
            throws IOException, UnknownHostException;


    /**
     * Creates a <code>StreamMocket</code> and connects it to the specified port number
     * at the specified address.  This <code>StreamMocket</code> is configured using
     * the <code>StreamMocket</code> options established for this factory.
     *
     * @param host the server host
     * @param port the server port
     * @return the <code>Socket</code>
     * @throws IOException if an I/O error occurs when creating the <code>StreamMocket</code>
     */
    public abstract StreamMocket createStreamMocket (InetAddress host, int port)
            throws IOException;


    /**
     * Creates a <code>StreamMocket</code> and connect it to the specified remote address
     * on the specified remote port.  The <code>StreamMocket</code> will also be bound
     * to the local address and port suplied.  The <code>StreamMocket</code> is configured using
     * the <code>StreamMocket</code> options established for this factory.
     *
     * @param address      the server network address
     * @param port         the server port
     * @param localAddress the client network address
     * @param localPort    the client port
     * @return the <code>Socket</code>
     * @throws IOException if an I/O error occurs when creating the <code>StreamMocket</code>
     */
    public abstract StreamMocket
    createStreamMocket (InetAddress address, int port,
                        InetAddress localAddress, int localPort)
            throws IOException;
}


//
// The default factory has NO intelligence about policies like tunneling
// out through firewalls (e.g. SOCKS V4 or V5) or in through them
// (e.g. using SSL), or that some ports are reserved for use with SSL.
//
// Note that at least JDK 1.1 has a low level "plainSocketImpl" that
// knows about SOCKS V4 tunneling, so this isn't a totally bogus default.
//
// ALSO:  we may want to expose this class somewhere so other folk
// can reuse it, particularly if we start to add highly useful features
// such as ability to set connect timeouts.
//
class DefaultStreamMocketFactory extends StreamMocketFactory
{

    public StreamMocket createStreamMocket () throws IOException
    {
        return new StreamMocket();
    }

    public StreamMocket createStreamMocket (String host, int port)
            throws IOException, UnknownHostException
    {
        return new StreamMocket(host, port);
    }

    public StreamMocket createStreamMocket (InetAddress address, int port)
            throws IOException
    {
        return new StreamMocket(address, port);
    }

    public StreamMocket createStreamMocket (String host, int port,
                                            InetAddress clientAddress, int clientPort)
            throws IOException, UnknownHostException
    {
        return new StreamMocket(host, port, clientAddress, clientPort);
    }

    public StreamMocket createStreamMocket (InetAddress address, int port,
                                            InetAddress clientAddress, int clientPort)
            throws IOException
    {
        return new StreamMocket(address, port, clientAddress, clientPort);
    }
}
