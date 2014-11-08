/**
 * The Chunk class represents a chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

abstract class Chunk
{        
    Chunk (byte[] buf, int off)
    {
        if (buf == null || buf.length < off + CHUNK_HEADER_SIZE)
            throw new IllegalArgumentException ("Invalid argument!");
        
        _buf = buf;
        _off = off;
            
        _type = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off);
        _len  = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off + 2);
            
        if (!MessagePacket.isValidChunkType (_type)) 
            throw new RuntimeException ("Invalid chunk!");

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    protected Chunk (int type, int len)
    {
        _buf = null;
        _off = 0;
        
        if (!MessagePacket.isValidChunkType(type))
            throw new IllegalArgumentException ("Invalid chunk type!");
        _type = type;
        
        if (len < 0 || len > 65535)
            throw new IllegalArgumentException ("Chunk length must be " + 
                                                "a value between 0 and 2^16-1!");
        _len  = len;
        
        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    int getType() {
        return _type;
    }
    
    int getLength() {
        return _len;
    }

    abstract void write (byte[] buf, int off);

    protected void writeHeader (byte[] buf, int off) {
        ByteConverter.fromUnsignedShortIntTo2Bytes (_type, buf, off);
        ByteConverter.fromUnsignedShortIntTo2Bytes (_len, buf, off + 2);
    }
    
    /* 2 bytes for chunk type, 2 bytes for chunk length */
    static final int CHUNK_HEADER_SIZE            = 4;

    static final int CHUNK_CLASS_METADATA         = 0x1000;
    static final int CHUNK_CLASS_DATA             = 0x2000;
    static final int CHUNK_CLASS_STATECHANGE      = 0x4000;
                                                  
    static final int CHUNK_TYPE_SACK              = CHUNK_CLASS_METADATA    | 0x0001;
    static final int CHUNK_TYPE_HEARTBEAT         = CHUNK_CLASS_METADATA    | 0x0002;
    static final int CHUNK_TYPE_CANCELLED_PACKETS = CHUNK_CLASS_METADATA    | 0x0004;
                                                  
    static final int CHUNK_TYPE_DATA              = CHUNK_CLASS_DATA        | 0x0001;
                                                  
    static final int CHUNK_TYPE_INIT              = CHUNK_CLASS_STATECHANGE | 0x0001;
    static final int CHUNK_TYPE_INIT_ACK          = CHUNK_CLASS_STATECHANGE | 0x0002;
    static final int CHUNK_TYPE_COOKIE_ECHO       = CHUNK_CLASS_STATECHANGE | 0x0003;
    static final int CHUNK_TYPE_COOKIE_ACK        = CHUNK_CLASS_STATECHANGE | 0x0004;
    static final int CHUNK_TYPE_SHUTDOWN          = CHUNK_CLASS_STATECHANGE | 0x0005;
    static final int CHUNK_TYPE_SHUTDOWN_ACK      = CHUNK_CLASS_STATECHANGE | 0x0006;
    static final int CHUNK_TYPE_SHUTDOWN_COMPLETE = CHUNK_CLASS_STATECHANGE | 0x0007;
    static final int CHUNK_TYPE_ABORT             = CHUNK_CLASS_STATECHANGE | 0x0008;

    protected byte[] _buf;
    protected int _off;
    protected int _type;
    protected int _len;
    protected Logger _logger; 
}

/*
 * vim: et ts=4 sw=4
 */
