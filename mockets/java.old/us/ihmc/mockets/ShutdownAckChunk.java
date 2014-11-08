/**
 * The ShutdownAckChunk class represents a SHUTDOWN-ACK chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class ShutdownAckChunk extends Chunk
{
    ShutdownAckChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_SHUTDOWN_ACK)
            throw new RuntimeException ("This is not a SHUTDOWN-ACK chunk!");
        
        if (_len != SHUTDOWN_ACK_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for SHUTDOWN-ACK chunk!");
    }
    
    ShutdownAckChunk () {
        super (Chunk.CHUNK_TYPE_SHUTDOWN_ACK, 
               SHUTDOWN_ACK_CHUNK_LENGTH);
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built SHUTDOWN-ACK chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + SHUTDOWN_ACK_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);
    }

    static final int SHUTDOWN_ACK_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE;
}
/*
 * vim: et ts=4 sw=4
 */

