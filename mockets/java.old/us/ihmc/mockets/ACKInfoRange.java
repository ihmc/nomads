/**
 * The ACKInfoRange class represents a sequence number range in an ACKInfoBlockRanges.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import us.ihmc.util.ByteConverter;

class ACKInfoRange
{
    ACKInfoRange (byte[] buf, int off) 
    {
        if (buf == null || buf.length < off + ACK_INFO_RANGE_LEN)
            throw new IllegalArgumentException ("Invalid argument!");
                
        _begin = ByteConverter.from4BytesToUnsignedInt (buf, off);
        _end = ByteConverter.from4BytesToUnsignedInt (buf, off + 4);
    }
    
    ACKInfoRange (long firstTSN, long lastTSN) 
    {
        if (firstTSN < 0 || lastTSN < 0)
            throw new IllegalArgumentException ("ACKInfoRange endpoints must be non negative.");
        
        _begin = firstTSN;
        _end = lastTSN;
    }

    /* checks if the range defined in this object overlaps
     * the one defined in r */
    synchronized boolean overlaps (ACKInfoRange r) 
    {
        return (((_begin <= r._end + 1) &&
                 (_begin >= r._begin)) || 
                ((_end >= r._begin - 1) &&
                 (_end <= r._end)));
    }

    synchronized void merge (ACKInfoRange r)
    {
        if (!this.overlaps (r))
            throw new RuntimeException ("Trying to merge two ranges that do not overlap.");
        
        _begin = Math.min (_begin, r._begin);
        _end = Math.max (_end, r._end);
    }

    synchronized long getStart()
    {
        return _begin;
    }
    
    synchronized long getEnd()
    {
        return _end;
    }

    synchronized void write (byte[] buf, int off)
    {
        if (buf == null || buf.length < off + ACK_INFO_RANGE_LEN)
            throw new IllegalArgumentException ("Invalid argument!");

        ByteConverter.fromUnsignedIntTo4Bytes (_begin, buf, off);
        ByteConverter.fromUnsignedIntTo4Bytes (_end, buf, off + 4);
    }

    static int getLength()
    {
        return ACK_INFO_RANGE_LEN;
    }

    void _dump (PrintStream os) {
        os.print ("(" + _begin + "," + _end + ")");
    }
    
    /* 4bytes for start of range, 4bytes for end of range */
    private static int ACK_INFO_RANGE_LEN = 8;

    private long _begin;
    private long _end;
}

/*
 * vim: et ts=4 sw=4
 */

