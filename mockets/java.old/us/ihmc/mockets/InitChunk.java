package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

/**
 * The InitChunk class represents an INIT chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
class InitChunk extends Chunk
{
    InitChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_INIT)
            throw new RuntimeException ("This is not an INIT chunk!");
        
        if (_len != INIT_CHUNK_LENGTH)
            throw new RuntimeException ("Invalid chunk length field for INIT chunk!");

        _validation          = ByteConverter.from4BytesToUnsignedInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE);
        _initialControlTSN   = ByteConverter.from4BytesToUnsignedInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE + 4);
        _initialRelSeqTSN    = ByteConverter.from4BytesToUnsignedInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE + 8);
        _initialUnrelSeqTSN  = ByteConverter.from4BytesToUnsignedInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE + 12);
        _initialRelUnseqID   = ByteConverter.from4BytesToUnsignedInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE + 16);
        _initialUnrelUnseqID = ByteConverter.from4BytesToUnsignedInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE + 20);
    }
    
    InitChunk (long validation, long initialRelSeqTSN, long initialUnrelSeqTSN, 
               long initialControlTSN, long initialRelUnseqID,
               long initialUnrelUnseqID)
    {
        super (Chunk.CHUNK_TYPE_INIT, 
               INIT_CHUNK_LENGTH);
        
        if (validation < 1 || validation > 4294967295L)
            throw new IllegalArgumentException ("validation must be " +
                                                "a value between 1 and 2^32-1!");
        _validation = validation;
        
        if (initialRelSeqTSN < 0 || initialRelSeqTSN > 4294967295L)
            throw new IllegalArgumentException ("initialRelSeqTSN must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialRelSeqTSN = initialRelSeqTSN;

        if (initialUnrelSeqTSN < 0 || initialUnrelSeqTSN > 4294967295L)
            throw new IllegalArgumentException ("initialUnrelSeqTSN must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialUnrelSeqTSN = initialUnrelSeqTSN;

        if (initialControlTSN < 0 || initialControlTSN > 4294967295L)
            throw new IllegalArgumentException ("initialControlTSN must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialControlTSN = initialControlTSN;

        if (initialRelUnseqID < 0 || initialRelUnseqID > 4294967295L)
            throw new IllegalArgumentException ("initialRelUnseqID must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialRelUnseqID = initialRelUnseqID;

        if (initialUnrelUnseqID < 0 || initialUnrelUnseqID > 4294967295L)
            throw new IllegalArgumentException ("initialUnrelUnseqID must be " + 
                                                "a value between 0 and 2^32-1!");
        _initialUnrelUnseqID = initialUnrelUnseqID;
    }
    
    void write (byte[] buf, int off) 
    {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built INIT chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + INIT_CHUNK_LENGTH)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);
        
        ByteConverter.fromUnsignedIntTo4Bytes (_validation, buf, off + Chunk.CHUNK_HEADER_SIZE);
        ByteConverter.fromUnsignedIntTo4Bytes (_initialControlTSN,   buf, off + Chunk.CHUNK_HEADER_SIZE + 4);
        ByteConverter.fromUnsignedIntTo4Bytes (_initialRelSeqTSN,    buf, off + Chunk.CHUNK_HEADER_SIZE + 8);
        ByteConverter.fromUnsignedIntTo4Bytes (_initialUnrelSeqTSN,  buf, off + Chunk.CHUNK_HEADER_SIZE + 12);
        ByteConverter.fromUnsignedIntTo4Bytes (_initialRelUnseqID,   buf, off + Chunk.CHUNK_HEADER_SIZE + 16);
        ByteConverter.fromUnsignedIntTo4Bytes (_initialUnrelUnseqID, buf, off + Chunk.CHUNK_HEADER_SIZE + 20);
    }

    long getValidation()
    {
        return _validation;
    }

    long getInitialRelSeqTSN()
    {
        return _initialRelSeqTSN;
    }
    
    long getInitialUnrelSeqTSN()
    {
        return _initialUnrelSeqTSN;
    }
    
    long getInitialControlTSN()
    {
        return _initialControlTSN;
    }
    
    long getInitialRelUnseqID()
    {
        return _initialRelUnseqID;
    }
    
    long getInitialUnrelUnseqID()
    {
        return _initialUnrelUnseqID;
    }
    
    /* 4 bytes for validation, 4 bytes for each of the 5 sequence numbers */
    public static final int INIT_CHUNK_LENGTH = Chunk.CHUNK_HEADER_SIZE + 24;
    
    private long _validation;
    private long _initialControlTSN;
    private long _initialRelSeqTSN;
    private long _initialUnrelSeqTSN;
    private long _initialRelUnseqID;
    private long _initialUnrelUnseqID;
}

/*
 * vim: et ts=4 sw=4
 */

