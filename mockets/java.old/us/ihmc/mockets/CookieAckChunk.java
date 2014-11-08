/**
 * The CookieAckChunk class represents a COOKIE-ACK chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class CookieAckChunk extends Chunk
{
    CookieAckChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_COOKIE_ACK)
            throw new RuntimeException ("This is not a COOKIE-ACK chunk!");
        
        if (_len != COOKIE_ACK_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for COOKIE-ACK chunk!");

        _newPort = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE); 
    }
    
    CookieAckChunk (int newPort) {
        super (Chunk.CHUNK_TYPE_COOKIE_ACK, 
               COOKIE_ACK_CHUNK_LENGTH);

        if (newPort < 0 || newPort > 65535)
            throw new IllegalArgumentException ("newPort must be a value " +
                                                "between 0 and 2^16-1!");
        _newPort = newPort;
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException ("Can write only locally built COOKIE-ACK chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException ("Invalid argument!");

        if (buf.length < off + COOKIE_ACK_CHUNK_LENGTH)
            throw new RuntimeException ("Not enough space in destination buffer!");

        writeHeader (buf, off);

        ByteConverter.fromUnsignedShortIntTo2Bytes (_newPort, buf, off + Chunk.CHUNK_HEADER_SIZE);
    }

    int getPort() {
        return _newPort;
    }
    
    /* 2 bytes for new port */
    public static final int COOKIE_ACK_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE + 2;

    private int _newPort;
}
/*
 * vim: et ts=4 sw=4
 */

