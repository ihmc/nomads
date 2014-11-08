/**
 * The ACKInformation class represents the aggregate acknowledgment information 
 * included in SACK and CANCELLED PACKETS chunks.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.logging.Logger;

import us.ihmc.util.ByteConverter;


class ACKInformation
{
    ACKInformation (byte[] buf, int off, int len)
    {
        _buf = buf;
        _off = off;
        _len = len;
        
        assert buf.length >= off + len;

        _ackInfoBlocks = new LinkedList();

        int offset = _off;

        while (offset < _off + _len) {
            ACKInfoBlock aib = ACKInfoBlock.readACKInfoBlock (_buf, offset);
            _ackInfoBlocks.addLast (aib);
            offset += aib.getLength();

            if (offset > _off + _len)
                throw new RuntimeException ("Error in ACKInformation.__init__!!!");
        }

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    ACKInformation() 
    {
        _buf = null;
        _off = 0;
        _len = 0;

        _ackInfoBlocks = new LinkedList();

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }

    synchronized void addACKInfoBlock (ACKInfoBlock aib)
    {
        assert aib != null;
        //_logger.fine ("Adding ACKInfoBlock.");
        _ackInfoBlocks.addLast (aib);
        _len += aib.getLength();
    }

    synchronized int getLength()
    {
        return _len;
    }
    
    synchronized void write (byte[] buf, int off)
    {
        if (_buf != null)
            throw new RuntimeException ("Can write only locally built ACKInformation instances!");
        
        if (buf == null) 
            throw new IllegalArgumentException ("Invalid argument!");

        if (buf.length < off + _len)
            throw new RuntimeException ("Not enough space in destination buffer!");

        //_logger.fine ("Trying to write " + _ackInfoBlocks.size() + " AckInfoBlocks.");
        int offset = off;
        Iterator it = _ackInfoBlocks.iterator();
        while (it.hasNext()) {
            ACKInfoBlock aib = (ACKInfoBlock) it.next();
            assert aib != null;
            aib.write (buf, offset);
            offset += aib.getLength();
        }
    }

    synchronized void notify (ACKInformationListener ail) 
    {
        // selective acknowledgement
        Iterator i = _ackInfoBlocks.iterator();
        while (i.hasNext()) {
            ACKInfoBlock aib = (ACKInfoBlock) i.next();
            boolean control;
            boolean sequenced;
            switch (aib.getType()) {
                case ACKInfoBlock.ACK_INFO_TYPE_RANGES:
                    assert (aib instanceof ACKInfoBlockRanges);
                    ACKInfoBlockRanges aibr = (ACKInfoBlockRanges) aib;
                    control = aibr.isFlagSet (ACKInfoBlock.ACK_INFO_FLAG_CONTROL_FLOW);
                    sequenced = control ? false : aibr.isFlagSet (ACKInfoBlock.ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW);
                    Iterator j = aibr.iterator();
                    while (j.hasNext()) {
                        ACKInfoRange air = (ACKInfoRange) j.next();
                        ail.processPacketsRange (air.getStart(), air.getEnd(), control, sequenced);
                    }
                    break;
                case ACKInfoBlock.ACK_INFO_TYPE_SINGLES:
                    assert (aib instanceof ACKInfoBlockSingles);
                    ACKInfoBlockSingles aibs = (ACKInfoBlockSingles) aib;
                    control = aibs.isFlagSet (ACKInfoBlock.ACK_INFO_FLAG_CONTROL_FLOW);
                    sequenced = control ? false : aibs.isFlagSet (ACKInfoBlock.ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW);
                    Iterator k = aibs.iterator();
                    while (k.hasNext()) {
                        Long tsnToAck = (Long) k.next();
                        ail.processPacket (tsnToAck.longValue(), control, sequenced);
                    }
                    break;
                default:
                    throw new RuntimeException ("Acknowledge information block type unknown or unsupported.");
           }
        }
    }

    void _dump (PrintStream os) {
        Iterator i = _ackInfoBlocks.iterator();
        while (i.hasNext()) {
            ACKInfoBlock aib = (ACKInfoBlock) i.next();
            aib._dump (os);
        }
    }

    private byte[] _buf;
    private int _off;
    private int _len;
    private LinkedList _ackInfoBlocks;

    private Logger _logger;
}

/*
 * vim: et ts=4 sw=4
 */

