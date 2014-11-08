/**
 * The ACKInfoBlockRanges class represents a type 0 acknowledgement information block
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

class ACKInfoBlockRanges extends ACKInfoBlock
{        
    ACKInfoBlockRanges (byte[] buf, int off)
    {
        super (buf, off);
        
        assert (_len > 0 && ((_len - ACKInfoBlock.ACK_INFO_HEADER_SIZE) % ACKInfoRange.getLength()) == 0);

        int numOfRanges = (_len - ACKInfoBlock.ACK_INFO_HEADER_SIZE) / ACKInfoRange.getLength();
        int offset = _off + ACKInfoBlock.ACK_INFO_HEADER_SIZE;        
        
        _ranges = new LinkedList();
        
        for (int i = 0; i < numOfRanges; ++i) {
            ACKInfoRange air = new ACKInfoRange (_buf, offset);
            _ranges.addLast (air);
            offset += ACKInfoRange.getLength();
        }

        _len = offset - off;
    }
    
    ACKInfoBlockRanges (boolean control, boolean sequenced)
    {
        super (ACKInfoBlock.ACK_INFO_FLAG_TYPE_RANGES);

        if (control) {
            assert !sequenced;
            setFlags (ACKInfoBlock.ACK_INFO_FLAG_CONTROL_FLOW);
        } else if (sequenced) {
            setFlags (ACKInfoBlock.ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW);
        } else {
            setFlags (ACKInfoBlock.ACK_INFO_FLAG_RELIABLE_UNSEQUENCED_FLOW);
        }

        _ranges = new LinkedList();
    }

    synchronized void addRange (ACKInfoRange air)
    {
        _ranges.add (air);
        _len += ACKInfoRange.getLength();

        assert _len == (ACKInfoBlock.ACK_INFO_HEADER_SIZE + _ranges.size() * ACKInfoRange.getLength());
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

        Iterator it = _ranges.iterator();
        while (it.hasNext()) {
            ACKInfoRange air = (ACKInfoRange) it.next();
            assert air != null;
            air.write (buf, offset);
            offset += ACKInfoRange.getLength();
        }
    }

    synchronized Iterator iterator()
    {
        return _ranges.iterator();
    }

    int getType()
    {
        return ACKInfoBlock.ACK_INFO_TYPE_RANGES;
    }

    void _dump (PrintStream os) {
        Iterator it = _ranges.iterator(); 
        while (it.hasNext()) {
            ACKInfoRange air = (ACKInfoRange) it.next();
            air._dump (os);
            os.print ("; ");
        }
        os.print ("\n");
    }
    
    private LinkedList _ranges;
}

/*
 * vim: et ts=4 sw=4
 */
