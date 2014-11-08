/**
 * The DeliveryPrerequisites class represents a DeliveryPrerequisites 
 * option header in a MessagePacket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class DeliveryPrerequisites
{        
    DeliveryPrerequisites (byte[] buf, int off)
    {
        if (buf == null || buf.length < off + DELIVERY_PREREQUISITES_SIZE)
            throw new IllegalArgumentException("Invalid argument!");
        
        _TSN1 = ByteConverter.from4BytesToUnsignedInt (buf, off);
        _TSN2 = ByteConverter.from4BytesToUnsignedInt (buf, off + 4);
            
        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    // if one of the sequence number is < 0 then it's ignored
    DeliveryPrerequisites (long TSN1, long TSN2)
    {
        if (TSN1 < 0 || TSN1 > SequentialArithmetic.MaximumTSN ||
            TSN2 < 0 || TSN2 > SequentialArithmetic.MaximumTSN)
            throw new IllegalArgumentException ("Sequence numbers must be greater " +
                                                "than 0 and less than " +
                                                SequentialArithmetic.MaximumTSN);

        _TSN1 = TSN1;
        _TSN2 = TSN2;
        
        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    synchronized void write (byte[] buf, int off) 
    {
        if (buf == null)
            throw new IllegalArgumentException("Invalid argument!");
        
        if (buf.length < off + DELIVERY_PREREQUISITES_SIZE)
            throw new RuntimeException("Not enough space in destination buffer!");

        ByteConverter.fromUnsignedIntTo4Bytes (_TSN1, buf, off);
        ByteConverter.fromUnsignedIntTo4Bytes (_TSN2, buf, off + 4);
    }

    synchronized boolean isSatisfied (long TSN1, long TSN2) 
    {
        if (SequentialArithmetic.greaterThan(_TSN1, TSN1) ||
            SequentialArithmetic.greaterThan(_TSN2, TSN2)) {
            return false;
        }
        return true;
    }

    /* 4 bytes for each of the 2 sequence numbers */
    static final int DELIVERY_PREREQUISITES_SIZE = 8;

    private long _TSN1;
    private long _TSN2;
    private Logger _logger; 
}
/*
 * vim: et ts=4 sw=4
 */

