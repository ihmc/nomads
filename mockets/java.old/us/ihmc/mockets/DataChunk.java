/**
 * The DataChunk class represents a DATA chunk in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class DataChunk extends Chunk
{
    DataChunk (byte[] buf, int off)
    {
        super (buf, off);
        
        if (_type != Chunk.CHUNK_TYPE_DATA)
            throw new RuntimeException ("This is not a DATA chunk!");
        
        if (_len < DATA_CHUNK_HEADER_SIZE)
            throw new RuntimeException ("Invalid chunk length field for DATA chunk!");

        _tagID = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off + Chunk.CHUNK_HEADER_SIZE);
        
        _data = _buf;
        _dataOff = _off + DATA_CHUNK_HEADER_SIZE;
        _dataLen = _len - DATA_CHUNK_HEADER_SIZE;
    }
    
    DataChunk (int tagID, byte[] data, int dataOff, int dataLen) 
    {
        super (Chunk.CHUNK_TYPE_DATA, 
               /* allocate minimum length for the moment */
               DATA_CHUNK_HEADER_SIZE);

        _tagID = tagID;
        
        _data = data;
        _dataOff = dataOff;
        _dataLen = dataLen;
        
        _len = DATA_CHUNK_HEADER_SIZE + _dataLen;
    }
    
    void write (byte[] buf, int off) {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built DATA chunks!");
        
        if (buf == null) 
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + _len)
            throw new RuntimeException("Not enough space in destination buffer!");

        writeHeader (buf, off);
        
        ByteConverter.fromUnsignedShortIntTo2Bytes (_tagID, buf, off + Chunk.CHUNK_HEADER_SIZE);

        System.arraycopy (_data, _dataOff, buf, off + DATA_CHUNK_HEADER_SIZE, _dataLen);
    }

    DataBuffer getDataBuffer() {
        if (_buf == null)
            throw new RuntimeException("Can call getDataBuffer() only DATA chunks " + 
                                       "belonging to incoming packets!");
        
        return new DataBuffer (_data, _dataOff, _dataLen);
    }

    int getTagID() {
        return _tagID;
    }

    /* 2 bytes for tag ID */
    public static final int DATA_CHUNK_HEADER_SIZE = Chunk.CHUNK_HEADER_SIZE + 2;
    static int TAG_DEFAULT = 0;
    static int TAG_UNDEFINED = -1;

    private int _tagID;

    private byte[] _data;
    private int _dataOff;
    private int _dataLen;
}

/*
 * vim: et ts=4 sw=4
 */
