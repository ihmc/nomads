/**
 * The ACKInfoBlockSingles class represents a type 1 acknowledgement information block
 * in an acknowledgement information.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.logging.Logger;
import us.ihmc.util.ByteConverter;

class ACKInfoBlockSingles extends ACKInfoBlock
{        
    ACKInfoBlockSingles (byte[] buf, int off)
    {
        super (buf, off);
        
        assert (_len > 0 && ((_len - ACKInfoBlock.ACK_INFO_HEADER_SIZE) % 4) == 0);

        int numOfSingles = (_len - ACKInfoBlock.ACK_INFO_HEADER_SIZE) / 4;
        int offset = _off + ACKInfoBlock.ACK_INFO_HEADER_SIZE;        
        
        _singles = new LinkedList();

        for (int i = 0; i < numOfSingles; ++i) {
            _singles.addLast (new Long(ByteConverter.from4BytesToUnsignedInt(_buf, offset)));
            offset += 4;
        }

        _len = offset - off;
    }
    
    ACKInfoBlockSingles (boolean control, boolean sequenced)
    {
        super (ACKInfoBlock.ACK_INFO_FLAG_TYPE_SINGLES);

        if (control) {
            assert !sequenced;
            setFlags (ACKInfoBlock.ACK_INFO_FLAG_CONTROL_FLOW);
        } else if (sequenced) {
            setFlags (ACKInfoBlock.ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW);
        } else {
            setFlags (ACKInfoBlock.ACK_INFO_FLAG_RELIABLE_UNSEQUENCED_FLOW);
        }

        _singles = new LinkedList();
    }

    synchronized void addSingle (long tsn)
    {
        _singles.add (new Long(tsn));
        _len += 4;

        assert _len == (ACKInfoBlock.ACK_INFO_HEADER_SIZE + _singles.size() * 4);
    }

    synchronized void write (byte[] buf, int off) 
    {
        if (_buf != null)
            throw new RuntimeException("Can write only locally built ACKInformation instances!");

        if (buf == null)
            throw new IllegalArgumentException("Invalid argument!");

        if (buf.length < off + _len)
            throw new RuntimeException("Not enough space in destination buffer!");    

        writeHeader (buf, off);
        
        int offset = off + ACKInfoBlock.ACK_INFO_HEADER_SIZE;
        Iterator it = _singles.iterator(); 
        while (it.hasNext()) {
            Long tsn = (Long) it.next();
            ByteConverter.fromUnsignedIntTo4Bytes (tsn.longValue(), buf, offset);
            offset += 4;
        }
    }

    synchronized Iterator iterator()
    {
        return _singles.iterator();
    }

    int getType()
    {
        return ACKInfoBlock.ACK_INFO_TYPE_SINGLES;
    }

    void _dump (PrintStream os) {
        Iterator it = _singles.iterator(); 
        while (it.hasNext()) {
            os.print ((Long) it.next() + " ");
        }
        os.print ("\n");
    }

    private LinkedList _singles;
}

/*
 * vim: et ts=4 sw=4
 */

