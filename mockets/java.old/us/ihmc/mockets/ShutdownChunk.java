/**
 * The ShutdownChunk class represents a SHUTDOWN chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class ShutdownChunk extends Chunk
{
    ShutdownChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_SHUTDOWN)
            throw new RuntimeException ("This is not a SHUTDOWN chunk!");
        
        if (_len != SHUTDOWN_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for SHUTDOWN chunk!");
    }     

    ShutdownChunk () {
        super (Chunk.CHUNK_TYPE_SHUTDOWN, 
               SHUTDOWN_CHUNK_LENGTH);
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built SHUTDOWN chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + SHUTDOWN_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");
        
        writeHeader (buf, off);
    }

    static final int SHUTDOWN_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE;
}
/*
 * vim: et ts=4 sw=4
 */

