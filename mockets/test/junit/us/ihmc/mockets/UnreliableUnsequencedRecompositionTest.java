/**
 * The UnreliableUnsequencedRecompositionTest class is a testcase for the 
 * reliable unsequenced fragment recomposition performed by the 
 * tryToDefragmentPacketFromUnsequencedQueue method of the
 * MessagePacketProcessor class.
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

import java.net.SocketException;
import java.util.Random;

import junit.framework.*;


public class UnreliableUnsequencedRecompositionTest extends TestCase
{
    public static Test suite() 
    {
        return new TestSuite (UnreliableUnsequencedRecompositionTest.class);
    } 

    public static void main (String args[]) 
    {
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        logger.setLevel (Level.OFF);
        
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
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test1 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi2);
        assertEquals ("test1 2 wrong", 2, upq.packetsInQueue());
        upq.insert (mpi3);
        assertEquals ("test1 3 wrong", 3, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertTrue ("test1 4 wrong", ret);
        assertEquals ("test1 5 wrong", 0, upq.packetsInQueue());
    }
    
    public void test2() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi2);
        assertEquals ("test2 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi3);
        assertEquals ("test2 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test2 3 wrong", ret);
        assertEquals ("test2 4 wrong", 2, upq.packetsInQueue());
    }
    
    public void test2a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 8000, 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi2);
        assertEquals ("test2a 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi3);
        assertEquals ("test2a 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test2a 3 wrong", ret);
        assertEquals ("test2a 4 wrong", 0, upq.packetsInQueue());
    }
    
    public void test3() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test3 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi3);
        assertEquals ("test3 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test3 3 wrong", ret);
        assertEquals ("test3 4 wrong", 2, upq.packetsInQueue());
    }
    
    public void test3a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 12000, 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test3a 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi3);
        assertEquals ("test3a 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test3a 3 wrong", ret);
        assertEquals ("test3a 4 wrong", 2, upq.packetsInQueue());
    }
    
    public void test4() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test4 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi2);
        assertEquals ("test4 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test4 3 wrong", ret);
        assertEquals ("test4 4 wrong", 2, upq.packetsInQueue());
    }
    
    public void test4a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test4a 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi2);
        assertEquals ("test4a 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test4a 3 wrong", ret);
        assertEquals ("test4a 4 wrong", 0, upq.packetsInQueue());
    }
    
    public void test5() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test5 1 wrong", 1, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test5 2 wrong", ret);
        assertEquals ("test5 3 wrong", 1, upq.packetsInQueue());
    }
    
    public void test5a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test5a 1 wrong", 1, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test5a 2 wrong", ret);
        assertEquals ("test5a 3 wrong", 1, upq.packetsInQueue());
    }
    
    public void test6() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi2);
        assertEquals ("test6 1 wrong", 1, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test6 2 wrong", ret);
        assertEquals ("test6 3 wrong", 1, upq.packetsInQueue());
    }
    
    public void test6a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi2);
        assertEquals ("test6a 1 wrong", 1, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test6a 2 wrong", ret);
        assertEquals ("test6a 3 wrong", 0, upq.packetsInQueue());
    }
    
    public void test7() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi3);
        assertEquals ("test7 1 wrong", 1, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test7 2 wrong", ret);
        assertEquals ("test7 3 wrong", 1, upq.packetsInQueue());
    }
    
    public void test7a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi3);
        assertEquals ("test7a 1 wrong", 1, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test7a 2 wrong", ret);
        assertEquals ("test7a 3 wrong", 0, upq.packetsInQueue());
    }
    
    public void test8() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test8 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi2);
        assertEquals ("test8 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test8 3 wrong", ret);
        assertEquals ("test8 4 wrong", 2, upq.packetsInQueue());
    }
    
    public void test8a() throws Exception
    {
        Random rGen = new Random (System.currentTimeMillis());
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        UnsequencedPacketQueue upq = new UnsequencedPacketQueue();
        upq.insert (mpi1);
        assertEquals ("test8a 1 wrong", 1, upq.packetsInQueue());
        upq.insert (mpi2);
        assertEquals ("test8a 2 wrong", 2, upq.packetsInQueue());
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, 0, 0);
        boolean ret = mpp.tryToDefragmentPacketFromUnsequencedQueue (upq, false);
        
        assertFalse ("test8a 3 wrong", ret);
        assertEquals ("test8a 4 wrong", 0, upq.packetsInQueue());
    }
}

/*
 * vim: et ts=4 sw=4
 */

