package us.ihmc.mockets;

import java.util.LinkedList;
import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

/**
 * The SACKChunk class represents a SACK chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
class SACKChunk extends Chunk
{
    SACKChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_SACK)
            throw new RuntimeException ("This is not a SACK chunk!");
        
        if (_len < SACK_CHUNK_HEADER_SIZE)
            throw new RuntimeException ("Invalid chunk length field for SACK chunk!");

        int sackInfoLen = _len - Chunk.CHUNK_HEADER_SIZE;
        _sackInfo = new SACKInformation (_buf, _off + Chunk.CHUNK_HEADER_SIZE, sackInfoLen);
    }
    
    SACKChunk (SACKInformation sackInfo) {
        super (Chunk.CHUNK_TYPE_SACK, 
               /* allocate minimum length for the moment */
               SACK_CHUNK_HEADER_SIZE);

        _sackInfo = sackInfo;

        _len = Chunk.CHUNK_HEADER_SIZE + _sackInfo.getLength();
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built SACK chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + _len)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);

        _sackInfo.write (buf, off + Chunk.CHUNK_HEADER_SIZE);
    }

    SACKInformation getSACKInformation() {
        if (_buf == null)
            throw new RuntimeException("Can read SACKInformation only for SACK chunks belonging to incoming packets!");

        return _sackInfo;
    }

    static final int SACK_CHUNK_HEADER_SIZE = Chunk.CHUNK_HEADER_SIZE;
    
    private SACKInformation _sackInfo;
}
/*
 * vim: et ts=4 sw=4
 */

