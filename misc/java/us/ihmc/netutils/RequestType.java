package us.ihmc.netutils;

/**
 * Request.java
 *
 * @author Enrico Casini (ecasin@ihmc.us)
 */
enum RequestType
{
    confirm(0),
    error(1),
    register(2),
    registerCallback(3),
    onMessage(4),
    onStats(5),
    udpRegister(6),
    udpDisconnect(7),
    streamSize(8);

    RequestType (int i32code)
    {
       _i32code = i32code;
    }

    public int i32code ()
    {
        return _i32code;
    }

    public static RequestType fromI32code(int i32code)
    {
        switch (i32code) {
            case 0:
                return confirm;
            case 1:
                return error;
            case 2:
                return register;
            case 3:
                return registerCallback;
            case 4:
                return onMessage;
            case 5:
                return onStats;
            case 6:
                return udpRegister;
            case 7:
                return udpDisconnect;
                //if none of these, client is trying to send the size of the stream it will send
            default:
                return streamSize;
                //throw new NumberFormatException("Unrecognized code " + i32code);
        }
    }

    private final int _i32code;
}


