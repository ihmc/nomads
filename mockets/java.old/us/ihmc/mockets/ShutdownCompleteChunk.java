package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

/**
 * The ShutdownCompleteChunk class represents a SHUTDOWN-COMPLETE chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
class ShutdownCompleteChunk extends Chunk
{
    ShutdownCompleteChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_SHUTDOWN_COMPLETE)
            throw new RuntimeException ("This is not a SHUTDOWN-COMPLETE chunk!");
        
        if (_len != SHUTDOWN_COMPLETE_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for SHUTDOWN-COMPLETE chunk!");
    }
    
    ShutdownCompleteChunk () {
        super (Chunk.CHUNK_TYPE_SHUTDOWN_COMPLETE, 
               SHUTDOWN_COMPLETE_CHUNK_LENGTH);
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built SHUTDOWN-COMPLETE chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + SHUTDOWN_COMPLETE_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);
    }

    public static final int SHUTDOWN_COMPLETE_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE;
}
/*
 * vim: et ts=4 sw=4
 */

