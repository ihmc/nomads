package us.ihmc.android.aci.dspro;

/**
 * MessageType.java
 *
 * Enum <code>MessageType</code> handles the different kinds of messages between the Service to the MainActivity
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public enum MessageType
{
    HANDSHAKE(0),
    //messages from Service to MainActivity
    PEER_LIST(1),
    CONFIGURE(2),

    //monitor
    BASIC_STATS(3),
    OVERALL_STATS(4),
    DUPLICATE_TRAFFIC_STATS(5),
    PEER_GROUP_STATS(6),
    PEER_LIST_REFRESH(7),
    PEER_TRAFFIC_INFO(8),
    INCOMING_RATE_INFO(9),
    RESET_SESSION_INFO(10);

    private final int _code;

    MessageType (int code)
    {
        _code = code;
    }

    public int code ()
    {
        return _code;
    }
}
