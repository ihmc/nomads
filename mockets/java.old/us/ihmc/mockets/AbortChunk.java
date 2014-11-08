package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

/**
 * The AbortChunk class represents a ABORT chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
class AbortChunk extends Chunk
{
    AbortChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_ABORT)
            throw new RuntimeException ("This is not a ABORT chunk!");
        
        if (_len != ABORT_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for ABORT chunk!");
    }
    
    AbortChunk () {
        super (Chunk.CHUNK_TYPE_ABORT, 
               ABORT_CHUNK_LENGTH);
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built ABORT chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + ABORT_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);
    }
    
    public static final int ABORT_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE;
}
/*
 * vim: et ts=4 sw=4
 */

