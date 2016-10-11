package us.ihmc.netutils;

import us.ihmc.comm.ProtocolException;


/**
 * Request.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Request
{
    public Request (RequestType requestType, short clientId, String clientName)
    {
        _requestType = requestType;
        _clientId = clientId;
        _clientName = clientName;
    }

    public RequestType getRequestType ()
    {
        return _requestType;
    }

    public short getClientId ()
    {
        return _clientId;
    }

    public String getClientName ()
    {
        return _clientName;
    }

    public static Request parse (String value) throws ProtocolException
    {
        try {
            String[] parsed = value.split(separator);
            RequestType requestType = RequestType.valueOf(parsed[0]);
            short clientId = Short.valueOf(parsed[1]);
            String clientName = null;
            if (parsed.length > 2) clientName = parsed[2];
            return new Request(requestType, clientId, clientName);

        }
        catch (Exception e) {
            throw new ProtocolException("Unable to parse valid Request from String " + value);
        }
    }

    private final RequestType _requestType;
    private final short _clientId;
    private final String _clientName;

    public final static String separator = "@";
}
