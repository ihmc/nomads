/**
 * The CancelledPacketsChunk class represents a CANCELLED PACKETS chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.LinkedList;
import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class CancelledPacketsChunk extends Chunk
{
    CancelledPacketsChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_CANCELLED_PACKETS)
            throw new RuntimeException ("This is not a CANCELLED PACKETS chunk!");
        
        if (_len < CANCELLED_PACKETS_CHUNK_HEADER_SIZE)
            throw new RuntimeException ("Invalid chunk length field for CANCELLED PACKETS chunk!");

        int cpInfoLen = _len - Chunk.CHUNK_HEADER_SIZE;
        _cpInfo = new ACKInformation (_buf, _off + Chunk.CHUNK_HEADER_SIZE, cpInfoLen);
    }
    
    CancelledPacketsChunk (ACKInformation cpInfo) 
    {
        super (Chunk.CHUNK_TYPE_CANCELLED_PACKETS, 
               /* allocate minimum length for the moment */
               CANCELLED_PACKETS_CHUNK_HEADER_SIZE);

        _cpInfo = cpInfo;

        _len = Chunk.CHUNK_HEADER_SIZE + _cpInfo.getLength();
    }
    
    void write (byte[] buf, int off) 
    {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built SACK chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + _len)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);

        _cpInfo.write (buf, off + Chunk.CHUNK_HEADER_SIZE);
    }

    ACKInformation getACKInformation() 
    {
        if (_buf == null)
            throw new RuntimeException("Can read ACKInformation only for CANCELLED PACKETS chunks belonging to incoming packets!");

        return _cpInfo;
    }

    static final int CANCELLED_PACKETS_CHUNK_HEADER_SIZE = Chunk.CHUNK_HEADER_SIZE;
    
    private ACKInformation _cpInfo;
}
/*
 * vim: et ts=4 sw=4
 */

