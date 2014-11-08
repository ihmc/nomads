/**
 * The CookieEchoChunk class represents a COOKIE-ECHO chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class CookieEchoChunk extends Chunk
{
    CookieEchoChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_COOKIE_ECHO)
            throw new RuntimeException ("This is not a COOKIE-ECHO chunk!");
        
        if (_len != COOKIE_ECHO_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for COOKIE-ECHO chunk!");

        _stateCookie = new StateCookie (buf, _off + Chunk.CHUNK_HEADER_SIZE);
    }     

    CookieEchoChunk (StateCookie stateCookie)
    {
        super (Chunk.CHUNK_TYPE_COOKIE_ECHO, 
               COOKIE_ECHO_CHUNK_LENGTH);

        if (stateCookie == null)
            throw new IllegalArgumentException("Invalid argument!");
        _stateCookie = stateCookie;
    }
    
    void write (byte[] buf, int off) 
    {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built COOKIE-ECHO chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + COOKIE_ECHO_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");
        
        writeHeader (buf, off);
        
        _stateCookie.write (buf, off + Chunk.CHUNK_HEADER_SIZE);
    }

    StateCookie getCookie()
    {
        return _stateCookie;
    }
 
    public static final int COOKIE_ECHO_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE + StateCookie.STATE_COOKIE_SIZE;

    private StateCookie _stateCookie;
}
/*
 * vim: et ts=4 sw=4
 */

