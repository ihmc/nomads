/**
 * The SequencedQueueTest class is a testcase for the 
 * SequencedPacketQueue class.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.Iterator;
import java.util.Random;

import junit.framework.*;


public class SequencedQueueTest extends TestCase
{
    public static Test suite() 
    {
        return new TestSuite (SequencedQueueTest.class);
    } 

    public static void main (String args[]) 
    {
        junit.textui.TestRunner.run (suite());
    }
    
    public void test1() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long startTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (startTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(startTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(startTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (startTSN);
        spq.insert (mpi1);
        assertEquals ("test1 1 wrong", 1, spq.packetsInQueue());
        spq.insert (mpi2);
        assertEquals ("test1 2 wrong", 2, spq.packetsInQueue());
        spq.insert (mpi3);
        assertEquals ("test1 3 wrong", 3, spq.packetsInQueue());
        
        int elements = 3;
        Iterator it = spq.iterator();
        while (it.hasNext()) {
            it.remove();
            assertEquals ("test1 iter wrong", --elements, spq.packetsInQueue());
        }
        
        //spq.dump (System.err);
        assertEquals ("test1 4 wrong", 0, spq.packetsInQueue());
    }
}

/*
 * vim: et ts=4 sw=4
 */

