package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

/**
 * The HeartbeatInfo class represents the heartbeat info included in the 
 * HEARTBEAT and HEARTBEAT-ACK chunks of a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
class HeartbeatInfo
{
    HeartbeatInfo (byte[] buf, int off)
    {
        if (buf.length < off + HEARTBEAT_INFO_SIZE)
            throw new RuntimeException("Input buffer too small to contain a heartbeat info!");

        _buf = buf;
        _off = off;

        _timestamp = ByteConverter.from8BytesToLong (_buf, _off);
    }

    HeartbeatInfo (long timestamp)
    {
        _buf = null;
        _off = 0;
        
        if (timestamp < 0)
            throw new IllegalArgumentException ("Timestamp must be a positive value!");
        _timestamp = timestamp;
    }

    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built heartbeat info!");
        
        if (buf == null)
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + HEARTBEAT_INFO_SIZE)
            throw new RuntimeException("Not enough space in destination buffer!");

        ByteConverter.fromLongTo8Bytes (_timestamp, buf, off);
    }

    /* heartbeat info contains only a 64-bit timestamp */
    public static final int HEARTBEAT_INFO_SIZE = 8;
    
    private byte[] _buf;
    private int _off;
    private int _len;
    private long _timestamp;
}
/*
 * vim: et ts=4 sw=4
 */

