/**
 * The ACKInfoBlock class represents a generic Acknowledgement information block
 * in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;
import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

abstract class ACKInfoBlock
{        
    ACKInfoBlock (byte[] buf, int off)
    {
        if (buf == null || buf.length < off + ACK_INFO_HEADER_SIZE)
            throw new IllegalArgumentException("Invalid argument!");
        
        _buf = buf;
        _off = off;
            
        _flags = ByteConverter.from1ByteToUnsignedByte (_buf, _off);
        assert (_flags & ~ACK_INFO_FLAGS_MASK) == 0 : "Unknown/unsupported flag!";
        
        _len  = ByteConverter.from2BytesToUnsignedShortInt (_buf, _off + 1);
            
        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    protected ACKInfoBlock (short flags)
    {
        _buf = null;
        _off = 0;
        
        assert (flags & ~ACK_INFO_FLAGS_MASK) == 0 : "Unknown/unsupported flag!";
        _flags = flags;
        
        _len = ACK_INFO_HEADER_SIZE;
        
        _logger = Logger.getLogger ("us.ihmc.mockets");
    }

    static ACKInfoBlock readACKInfoBlock (byte[] buf, int off)
    {
        ACKInfoBlock ret = null;
        
        short flags = ByteConverter.from1ByteToUnsignedByte (buf, off);
        
        if ((flags & ACK_INFO_FLAG_TYPE_RANGES) != 0) {
            assert (flags & (ACK_INFO_TYPE_FLAGS_MASK & ~ACK_INFO_FLAG_TYPE_RANGES)) == 0;
            ret = new ACKInfoBlockRanges (buf, off);
        } else if ((flags & ACK_INFO_FLAG_TYPE_SINGLES) != 0) {
            assert (flags & (ACK_INFO_TYPE_FLAGS_MASK & ~ACK_INFO_FLAG_TYPE_SINGLES)) == 0;
            ret = new ACKInfoBlockSingles (buf, off);
        } else {
            throw new RuntimeException ("Unknown/unsupported ACKInfoBlock type!");
        }
        
        return ret;
    }
    
    void setFlags (int flags)
    {
        // make sure this IS NOT an incoming ACKInfoBlock
        assert _buf == null;
        
        // sanity checks
        assert (flags & ~ACK_INFO_FLAGS_MASK) == 0 : "Unknown/unsupported flag!";

        _flags |= (flags & ACK_INFO_FLAGS_MASK);
    }

    boolean isFlagSet (int flag)
    {
        // sanity checks
        assert (flag & ~ACK_INFO_FLAGS_MASK) == 0 : "Unknown/unsupported flag!";

        return (_flags & flag) != 0;
    }

    abstract void _dump(PrintStream os);

    abstract int getType();
    
    // does this method need to be synchronized? 
    int getLength() {
        return _len;
    }

    abstract void write (byte[] buf, int off);

    protected void writeHeader (byte[] buf, int off) {
        ByteConverter.fromUnsignedByteTo1Byte (_flags, buf, off);
        ByteConverter.fromUnsignedShortIntTo2Bytes (_len, buf, off + 1);
    }

    protected static final short ACK_INFO_FLAG_CONTROL_FLOW = 0x01;
    protected static final short ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW = 0x02;
    protected static final short ACK_INFO_FLAG_RELIABLE_UNSEQUENCED_FLOW = 0x04;
    protected static final short ACK_INFO_FLAG_TYPE_RANGES = 0x10;
    protected static final short ACK_INFO_FLAG_TYPE_SINGLES = 0x20;
    private static final short ACK_INFO_FLAGS_MASK = ACK_INFO_FLAG_CONTROL_FLOW |
                                                     ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW |
                                                     ACK_INFO_FLAG_RELIABLE_UNSEQUENCED_FLOW |
                                                     ACK_INFO_FLAG_TYPE_RANGES |
                                                     ACK_INFO_FLAG_TYPE_SINGLES;
    private static final short ACK_INFO_TYPE_FLAGS_MASK = ACK_INFO_FLAG_TYPE_RANGES |
                                                          ACK_INFO_FLAG_TYPE_SINGLES;
    static final int ACK_INFO_TYPE_RANGES = 0;
    static final int ACK_INFO_TYPE_SINGLES = 1;

    /* 1 byte for flags, 2 bytes for length */
    static final int ACK_INFO_HEADER_SIZE = 3;

    protected byte[] _buf;
    protected int _off;
    protected short _flags;
    protected int _len;
    protected Logger _logger; 
}
/*
 * vim: et ts=4 sw=4
 */

