/**
 * The PendingPacketQueueTest class is a testcase for the 
 * PendingPacketQueue class.
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
class PendingPacketQueueTest 
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
            
        setup();
        test1();
        test2();

        setup();
        test3();
    }
   
    private static void setup() 
    {
        _ppq = new PendingPacketQueue (64 * 1024, false);
        _array = new MessagePacketInfo[10];
    }

    private static void test1() 
    {
        // insert 10 message packets
        for (int i = 0; i < _array.length; ++i) {
            // create empty message packet
            MessagePacket mp = new MessagePacket();
            MessagePacketInfo mpi = new MessagePacketInfo (mp, TxParams.MIN_PRIORITY, 0, System.currentTimeMillis(), 10000);

            _ppq.insert (mpi, 0);
            _array[i] = mpi;
        }
        
        _ppq._dump (System.out);
    }
    
    private static void test2() 
    {
        // remove half of the packets in the queue at random
        int counter = _array.length / 2;

        Random generator = new Random (System.currentTimeMillis());
        while (counter > 0) {
            int index = generator.nextInt (_array.length);
            if (_array[index] != null) {
                System.out.println ("removing element # " + (index + 1));
                _ppq.remove (_array[index]);
                _array[index] = null;
                --counter;
            }
        }

        _ppq._dump (System.out);
    }
    
    private static void test3() 
    {
        MessagePacketInfo mpi = null;
        MessagePacket f1 = new MessagePacket();
        f1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT | 
                     MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        mpi = new MessagePacketInfo (f1, 5, 0, System.currentTimeMillis(), 10000);
        _ppq.insert (mpi, 0);
        
        MessagePacket f2 = new MessagePacket();
        f2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        mpi = new MessagePacketInfo (f2, 5, 0, System.currentTimeMillis(), 10000);
        _ppq.insert (mpi, 0);
        
        MessagePacket f3 = new MessagePacket();
        f3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        mpi = new MessagePacketInfo (f3, 5, 0, System.currentTimeMillis(), 10000);
        _ppq.insert (mpi, 0);
        
        _ppq._dump (System.out);

        Random generator = new Random (System.currentTimeMillis());
        // insert 10 message packets
        for (int i = 0; i < _array.length - 3; ++i) {
            // create empty message packet
            MessagePacket mp = new MessagePacket();
            int priority = TxParams.MIN_PRIORITY + generator.nextInt (TxParams.MAX_PRIORITY - TxParams.MIN_PRIORITY + 1);
            mpi = new MessagePacketInfo (mp, priority, 0, System.currentTimeMillis(), 10000);

            _ppq.insert (mpi, 0);
        }

        _ppq._dump (System.out);
    }
    
    private static PendingPacketQueue _ppq;
    private static MessagePacketInfo _array[];
}

/*
 * vim: et ts=4 sw=4
 */
