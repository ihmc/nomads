package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

/**
 * The HeartbeatChunk class represents a HEARTBEAT chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
class HeartbeatChunk extends Chunk
{
    HeartbeatChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_HEARTBEAT)
            throw new RuntimeException ("This is not a HEARTBEAT chunk!");
        
        if (_len != HEARTBEAT_CHUNK_LENGTH)            
            throw new RuntimeException ("Invalid chunk length field for HEARTBEAT chunk!");

        _heartbeatInfo = new HeartbeatInfo (buf, _off + Chunk.CHUNK_HEADER_SIZE);
    }
    
    HeartbeatChunk (HeartbeatInfo heartbeatInfo) 
    {
        super (Chunk.CHUNK_TYPE_HEARTBEAT, 
               HEARTBEAT_CHUNK_LENGTH);

        if (heartbeatInfo == null)
            throw new IllegalArgumentException("Invalid argument!");
        _heartbeatInfo = heartbeatInfo;
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built HEARTBEAT chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + HEARTBEAT_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);

        _heartbeatInfo.write (buf, off + Chunk.CHUNK_HEADER_SIZE);
    }

    public static final int HEARTBEAT_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE + HeartbeatInfo.HEARTBEAT_INFO_SIZE;

    private HeartbeatInfo _heartbeatInfo;
}
/*
 * vim: et ts=4 sw=4
 */

