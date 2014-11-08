/**
 * The ReliableSequencedRecompositionTest class is a testcase for the 
 * reliable sequenced fragment recomposition performed by the 
 * tryToProcessFirstPacketFromSequencedQueue method of the
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


public class ReliableSequencedRecompositionTest extends TestCase
{
    public static Test suite() 
    {
        return new TestSuite (ReliableSequencedRecompositionTest.class);
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
        long neTSN = (long) rGen.nextInt() + (1L << 31);
        byte dummyContent[] = new byte[128];
        rGen.nextBytes (dummyContent);
        
        DataChunk d1 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp1 = new MessagePacket();
        mp1.setSequenceNumber (neTSN);
        mp1.addChunk (d1);
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi2);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = SequentialArithmetic.add (neTSN, 3);
        
        assertEquals ("test1 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test1 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi2);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test2 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test2 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 8000, 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi2);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test2a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test2a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test3 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test3 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 12000, 1);
        
        DataChunk d3 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp3 = new MessagePacket();
        mp3.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp3.addChunk (d3);
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test3a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test3a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi2);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test4 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test4 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 1));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi2);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test4a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test4a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test5 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test5 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test5a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test5a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi2);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test6 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test6 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi2);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test6a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test6a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test7 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test7 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp3.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
        MessagePacketInfo mpi3 = new MessagePacketInfo (mp3, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi3);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test7a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test7a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis(), 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi2);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test8 neTSN wrong", expectedResult, newTSN);
        assertEquals ("test8 spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
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
        mp1.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi1 = new MessagePacketInfo (mp1, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        DataChunk d2 = new DataChunk (DataChunk.TAG_DEFAULT, dummyContent, 
                                      0, dummyContent.length);
        MessagePacket mp2 = new MessagePacket();
        mp2.setSequenceNumber (SequentialArithmetic.add(neTSN, 2));
        mp2.addChunk (d2);
        mp2.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED |
                      MessagePacket.HEADER_FLAG_RELIABLE |
                      MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
        MessagePacketInfo mpi2 = new MessagePacketInfo (mp2, TxParams.MAX_PRIORITY, 0, 
                                                        System.currentTimeMillis() - 4000, 1);
        
        SequencedPacketQueue spq = new SequencedPacketQueue (neTSN);
        spq.insert (mpi1);
        spq.insert (mpi2);
        
        MessagePacketProcessor mpp = new MessagePacketProcessor (new MessageMocket(),
                                                                 0, neTSN, 0);
        long newTSN = mpp.tryToProcessFirstPacketFromSequencedQueue (spq, neTSN, false);
        long expectedResult = neTSN;
        
        assertEquals ("test8a neTSN wrong", expectedResult, newTSN);
        assertEquals ("test8a spq.neTSN wrong", expectedResult, spq.getNextExpectedTSN());
    }
}

/*
 * vim: et ts=4 sw=4
 */

