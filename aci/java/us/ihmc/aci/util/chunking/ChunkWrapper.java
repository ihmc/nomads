package us.ihmc.aci.util.chunking;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class ChunkWrapper extends us.ihmc.chunking.ChunkWrapper
{

    public ChunkWrapper(byte chunkId, byte totalNumberOfChunks, String mimeType)
    {
        super (null, chunkId, totalNumberOfChunks, mimeType);
    }

    public ChunkWrapper (byte[] data, byte chunkId, byte totalNumberOfChunks, String mimeType)
    {
        super (data, chunkId, totalNumberOfChunks, mimeType);
    }

    public void read (CommHelper _commHelper) throws CommException, ProtocolException
    {
        setData(_commHelper.receiveBlock());
    }

    public void write(CommHelper _commHelper) throws CommException
    {    
        _commHelper.sendBlock (getData());
    }
}

