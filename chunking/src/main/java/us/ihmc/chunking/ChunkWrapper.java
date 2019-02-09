package us.ihmc.chunking;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class ChunkWrapper
{
    private final byte _chunkId;
    private final byte _totalNumberOfChunks;
    private final String _mimeType;
    private byte[] _data;

    public ChunkWrapper(byte chunkId, byte totalNumberOfChunks, String mimeType)
    {
        this (null, chunkId, totalNumberOfChunks, mimeType);
    }

    public  ChunkWrapper (byte[] data, byte chunkId, byte totalNumberOfChunks, String mimeType)
    {
        _data = data;
        _chunkId = chunkId;
        _totalNumberOfChunks = totalNumberOfChunks;
        _mimeType = mimeType;
    }

    public byte getChunkId()
    {
        return _chunkId;
    }

    public byte[] getData()
    {
        return _data;
    }

    public void setData(byte[] data) 
    {
        _data = data;
    }
    
    public String getMimeType()
    {
        return _mimeType;
    }

    public byte getTotalNumberOfChunks()
    {
        return _totalNumberOfChunks;
    }

}

