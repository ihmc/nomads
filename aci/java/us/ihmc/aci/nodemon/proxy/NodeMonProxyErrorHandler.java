package us.ihmc.aci.nodemon.proxy;

/**
 * NodeMonProxyErrorHandler.java
 * <p/>
 * Class <code>NodeMonProxyErrorHandler</code> handles all common <code>NodeMonProxy</code> errors.
 *
 * @author Enrico Casini (ecasini@ihmc.us), Rita Lenzi (rlenzi@ihmc.us)
 */
public class NodeMonProxyErrorHandler
{
    /**
     * Gets the connection error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getConnectionError ()
    {
        return String.format("%s, unable to connect to server", CONNECTION_ERROR);
    }

    /**
     * Gets the server connection error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getServerConnectionError ()
    {
        return String.format("%s, lost connection with client", CONNECTION_ERROR);
    }

    /**
     * Gets the registration error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getRegistrationError ()
    {
        return String.format("%s, unable to register proxy", REGISTRATION_ERROR);
    }

    /**
     * Gets the protocol error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getProtocolError ()
    {
        return String.format("%s, unable to match the requested command", PROTOCOL_ERROR);
    }

    /**
     * Gets the no listener error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getNoListenersError ()
    {
        return String.format("%s, unable to find any registered application", NO_LISTENERS_ERROR);
    }

    /**
     * Gets the bound error
     *
     * @param port bounding port
     * @return a <code>String</code> representation of the error
     */
    static String getBoundError (final int port)
    {
        return String.format("%s, unable to bound port %d to socket", BOUND_ERROR, port);
    }

    /**
     * Gets the server connection error
     *
     * @param port bounding port
     * @return a <code>String</code> representation of the error
     */
    static String getServerConnectionError (final int port)
    {
        return String.format("%s on port %d", getServerConnectionError(), port);
    }

    /**
     * Gets the server connection error
     *
     * @param clientId id assigned to the client
     * @return a <code>String</code> representation of the error
     */
    static String getServerConnectionError (final short clientId)
    {
        return String.format("%s %d", getServerConnectionError(), clientId);
    }

    /**
     * Gets the connection error
     *
     * @param host hostname
     * @param port bounding port
     * @return a <code>String</code> representation of the error
     */
    static String getConnectionError (final String host, final int port)
    {
        return String.format("%s to %s on host %s:%d", getConnectionError(),
                NodeMonProxyErrorHandler.class.toString(), host, port);
    }

    /**
     * Gets the registration error
     *
     * @param host hostname
     * @param port bounding port
     * @return a <code>String</code> representation of the error
     */
    static String getRegistrationError (final String host, final int port)
    {
        return String.format("%s to %s on host %s:%d", getRegistrationError(),
                NodeMonProxyErrorHandler.class.toString(), host, port);
    }

    /**
     * Gets the protocol error
     *
     * @param host hostname
     * @param port bounding port
     * @return a <code>String</code> representation of the error
     */
    static String getProtocolError (final String host, final int port)
    {
        return String.format("%s to %s on host %s:%d", getProtocolError(),
                NodeMonProxyErrorHandler.class.toString(), host, port);
    }

    /**
     * Gets the protocol error related to a specific command
     *
     * @param command command associated to the error
     * @return a <code>String</code> representation of the error
     */
    static String getProtocolError (String command)
    {
        return String.format("%s %s", getProtocolError(), command);
    }

    /**
     * Gets the parsing error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getParsingError ()
    {
        return String.format("%s, unable to parse the data to create the InformationObject instance", PARSING_ERROR);
    }

    /**
     * Gets the serialization error
     *
     * @return a <code>String</code> representation of the error
     */
    static String getSerializationError()
    {
        return String.format ("%s, unable to serialize the data for %d", SERIALIZATION_ERROR);
    }

    /**
     * Gets the serialization error
     *
     * @param clientId id assigned to the client
     * @return a <code>String</code> representation of the error
     */
    static String getSerializationError (final short clientId)
    {
        return String.format ("%s for %d", getSerializationError(), clientId);
    }


    /**
     * Constructor for the <code>NodeMonProxyErrorHandler</code>
     */
    private NodeMonProxyErrorHandler ()
    {
        throw new AssertionError();
    }

    private static final String BOUND_ERROR = "BOUND ERROR";
    private static final String CONNECTION_ERROR = "CONNECTION ERROR";
    private static final String REGISTRATION_ERROR = "REGISTRATION ERROR";
    private static final String PROTOCOL_ERROR = "PROTOCOL ERROR";
    private static final String NO_LISTENERS_ERROR = "NO LISTENERS ERROR";
    private static final String PARSING_ERROR = "PARSING ERROR";
    private static final String SERIALIZATION_ERROR = "SERIALIZATION ERROR";
}
