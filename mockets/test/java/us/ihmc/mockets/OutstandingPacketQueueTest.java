/**
 * The OutstandingPacketQueueTest class is a testcase for the 
 * OutstandingPacketQueue class.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import java.util.Random;

//public 
class OutstandingPacketQueueTest 
//extends TestCase
{
    public static void main (String[] args) throws Exception
    {
        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        //FileHandler fh = new FileHandler ("DataRecv%g.log");
        //fh.setLevel (Level.INFO);
        //fh.setFormatter (new SimpleFormatter());
        //logger.addHandler (fh);
        logger.setLevel (Level.WARNING);
            
        System.out.println ("creating OutstandingPacketQueue opq");
        OutstandingPacketQueue opq = new OutstandingPacketQueue();
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
        
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("inserting 30 packets in opq");
        // insert 10 message packets
        for (int i = 0; i < 30; ++i) {
            // create empty message packet
            MessagePacket mp = new MessagePacket();
            mp.setFlags (MessagePacket.HEADER_FLAG_RELIABLE);
            mp.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED);
            mp.setSequenceNumber (i);
            MessagePacketInfo mpi = new MessagePacketInfo (mp, TxParams.MIN_PRIORITY, 0, 
                                                           System.currentTimeMillis(), 
                                                           Mocket.DEFAULT_RETRANSMISSION_TIMEOUT);
            opq.insert (mpi);
        }
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);

        int res = 0;
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging to TSN 3");
        res = opq.acknowledgePacketsTo (3, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
        
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging TSN range (7,11)");
        res = opq.processPacketsRange (7, 11, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);

        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging to TSN 15");
        res = opq.acknowledgePacketsTo (15, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
        
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging TSN range (13,17)");
        res = opq.processPacketsRange (13, 17, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
        
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging TSN range (23,27)");
        res = opq.processPacketsRange (23, 27, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
        
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging to TSN 20");
        res = opq.acknowledgePacketsTo (20, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
        
        System.out.println ("\n\n--------------------------------------------------");
        System.out.println ("acknowledging to TSN 25");
        res = opq.acknowledgePacketsTo (25, false, true);
        System.out.println ("removed " + res + " packets");
        System.out.println ("opq.isEmpty(): " + opq.isEmpty());
        opq._dump (System.out);
    }
}

/*
 * vim: et ts=4 sw=4
 */
