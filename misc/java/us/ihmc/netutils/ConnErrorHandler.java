package us.ihmc.netutils;

/**
 * ConnErrorHandler.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ConnErrorHandler
{
    static String getConnectionError ()
    {
        return String.format("%s, unable to connect to server", CONNECTION_ERROR);
    }

    static String getServerConnectionError ()
    {
        return String.format("%s, lost connection with client", CONNECTION_ERROR);
    }

    static String getRegistrationError ()
    {
        return String.format("%s, unable to register proxy", REGISTRATION_ERROR);
    }

    static String getProtocolError ()
    {
        return String.format("%s, unable to match the requested command", PROTOCOL_ERROR);
    }

    static String getNoListenersError ()
    {
        return String.format("%s, unable to find any registered application", NO_LISTENERS_ERROR);
    }

    static String getBoundError (final int port)
    {
        return String.format("%s, unable to bound port %d to socket", BOUND_ERROR, port);
    }

    static String getServerConnectionError (final int port)
    {
        return String.format("%s on port %d", getServerConnectionError(), port);
    }

    static String getServerConnectionError (final short id, final int port)
    {
        return String.format("%s %d on port %d", getServerConnectionError(), id, port);
    }

    static String getConnectionError (final String host, final int port)
    {
        return String.format("%s to %s on host %s:%d", getConnectionError(),
                Server.class.toString(), host, port);
    }

    static String getRegistrationError (final String host, final int port)
    {
        return String.format("%s to %s on host %s:%d", getRegistrationError(),
                Server.class.toString(), host, port);
    }

    static String getProtocolError (final String host, final int port)
    {
        return String.format("%s to %s on host %s:%d", getProtocolError(),
                Server.class.toString(), host, port);
    }

    static String getProtocolError (String command)
    {
        return String.format("%s %s", getProtocolError(), command);
    }

    private ConnErrorHandler ()
    {
        throw new AssertionError();
    }

    private static final String BOUND_ERROR = "BOUND ERROR";
    private static final String CONNECTION_ERROR = "CONNECTION ERROR";
    private static final String REGISTRATION_ERROR = "REGISTRATION ERROR";
    private static final String PROTOCOL_ERROR = "PROTOCOL ERROR";
    private static final String NO_LISTENERS_ERROR = "NO LISTENERS ERROR";
}
