/**
 * The SACKInformation class represents the acknowledgment information 
 * included in a SACK chunk.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.logging.Logger;

import us.ihmc.util.ByteConverter;


class SACKInformation
{
    SACKInformation (byte[] buf, int off, int len)
    {
        _buf = buf;
        _off = off;
        _len = len;
        
        assert buf.length >= off + len;

        _controlCumAck = ByteConverter.from4BytesToUnsignedInt (_buf, _off);
        _relSeqCumAck = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 4);
        _relUnseqCumAck = ByteConverter.from4BytesToUnsignedInt (_buf, _off + 8);

        _ackInfo = new ACKInformation (_buf, _off + 12, _len - 12);

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }
    
    SACKInformation (long controlCumAck,
                     long relSeqCumAck,
                     long relUnseqCumAck,
                     ACKInformation ai) 
    {
        _buf = null;
        _off = 0;
        _len = SACK_INFO_HEADER_SIZE;

        _controlCumAck = controlCumAck;
        _relSeqCumAck = relSeqCumAck;
        _relUnseqCumAck = relUnseqCumAck;
        
        _ackInfo = ai; //new ACKInformation();
        _len += _ackInfo.getLength();

        _logger = Logger.getLogger ("us.ihmc.mockets");
    }

    synchronized void addACKInfoBlock (ACKInfoBlock aib)
    {
        _ackInfo.addACKInfoBlock (aib);
        _len = SACK_INFO_HEADER_SIZE + _ackInfo.getLength();
    }

    synchronized int getLength()
    {
        return _len;
    }
    
    synchronized void write (byte[] buf, int off)
    {
        if (_buf != null)
            throw new RuntimeException ("Can write only locally built SACKInformation instances!");
        
        if (buf == null) 
            throw new IllegalArgumentException ("Invalid argument!");

        if (buf.length < off + _len)
            throw new RuntimeException ("Not enough space in destination buffer!");

        /* write header */
        ByteConverter.fromUnsignedIntTo4Bytes (_controlCumAck, buf, off);
        ByteConverter.fromUnsignedIntTo4Bytes (_relSeqCumAck, buf, off + 4);
        ByteConverter.fromUnsignedIntTo4Bytes (_relUnseqCumAck, buf, off + 8);

        _ackInfo.write (buf, off + SACK_INFO_HEADER_SIZE);
    }

    synchronized void acknowledge (OutstandingPacketQueue opq) 
    {
        // cumulative acknowledgement
        synchronized (opq) {
            opq.acknowledgePacketsTo (_controlCumAck, true, false);
            opq.acknowledgePacketsTo (_relSeqCumAck, false, true);
            _logger.info ("calling opq.acknowledgePacketsTo " + _relSeqCumAck + "RS");
            opq.acknowledgePacketsTo (_relUnseqCumAck, false, false);
            
            _ackInfo.notify (opq);
        }
    }

    void _dump (PrintStream os)
    {
        os.println ("CTL: " + _controlCumAck + " RS: " + _relSeqCumAck + " RU: " + _relUnseqCumAck);
        _ackInfo._dump (os);
    }

    /* 4 bytes for each cumulative ack */
    static final int SACK_INFO_HEADER_SIZE = 12;
    
    private byte[] _buf;
    private int _off;
    private int _len;
    private long _relSeqCumAck;
    private long _controlCumAck;
    private long _relUnseqCumAck;
    private ACKInformation _ackInfo;

    private Logger _logger;
}

/*
 * vim: et ts=4 sw=4
 */

