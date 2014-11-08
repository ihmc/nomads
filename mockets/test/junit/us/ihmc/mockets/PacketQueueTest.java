/**
 * The PacketQueueTest class is a testcase for the 
 * PacketQueue class.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.util.logging.Level;
import java.util.logging.Logger;

import java.util.Random;
import java.util.Vector;

import junit.framework.*;


public class PacketQueueTest extends TestCase
{
    public static Test suite() 
    {
        return new TestSuite (PacketQueueTest.class);
    } 
    
    public static void main (String[] args) throws Exception
    {
        /* initialize logging */
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _logger.setLevel (Level.INFO);
            
        junit.textui.TestRunner.run (suite());
    }
   
    public void setUp()
    {
        _pq = new PacketQueue();
        _storedData = new byte[PACKET_NUM][];

        Random rGen = new Random (System.currentTimeMillis());
        
        assert PACKET_NUM % 2 == 0;
        
        for (int i = 0; i < PACKET_NUM/2; ++i) {
            byte dummy1[] = new byte[PACKET_SIZE];
            byte dummy2[] = new byte[PACKET_SIZE];
            byte dummy3[] = new byte[PACKET_SIZE];
            byte dummy4[] = new byte[PACKET_SIZE];
            byte dummy5[] = new byte[PACKET_SIZE * 3];

            System.arraycopy (dummy2, 0, dummy5, 0, PACKET_SIZE);
            System.arraycopy (dummy3, 0, dummy5, PACKET_SIZE, PACKET_SIZE);
            System.arraycopy (dummy4, 0, dummy5, PACKET_SIZE * 2, PACKET_SIZE);
            
            rGen.nextBytes (dummy1);
            rGen.nextBytes (dummy2);
            rGen.nextBytes (dummy3);
            rGen.nextBytes (dummy4);
                        
            _pq.insert (new DataBuffer (dummy1, 0, dummy1.length));
            _storedData[i*2] = dummy1;
            
            Vector v = new Vector();
            v.add (new DataBuffer (dummy2, 0, dummy2.length));
            v.add (new DataBuffer (dummy3, 0, dummy3.length));
            v.add (new DataBuffer (dummy4, 0, dummy4.length));

            _pq.insert (v);
            _storedData[i*2 + 1] = dummy5;
        }
    }
    
    public void tearDown()
    {
    }
    
    public void test1()
    {
        assertEquals ("test1 failed", PACKET_NUM, _pq.getPacketsInQueue());
    }

    public void test2() 
    {
        assert PACKET_NUM % 2 == 0;
        
        _logger = Logger.getLogger ("us.ihmc.mockets");
        
        for (int i = 0; i < PACKET_NUM/2; ++i) {
            ReceivedMessage tmp = _pq.extract(0);
            assertEquals ("test2 (iter " + i + ") a failed", PACKET_SIZE, tmp.getSize());
            _logger.info ("Extracted one packet from packet queue");
            
            ReceivedMessage tmp2 = _pq.extract(0);
            assertEquals ("test2 (iter " + i + ") b failed", PACKET_SIZE * 3, tmp2.getSize());
            _logger.info ("Extracted one packet from packet queue");
        }

        assertEquals ("test2 a failed", 0, _pq.getPacketsInQueue());
        assertTrue ("test2 b failed", _pq.isEmpty());
    }

    private static Logger _logger;    
    private byte[][] _storedData;
    private PacketQueue _pq;
    private static final int PACKET_NUM = 10;
    private static final int PACKET_SIZE = 1024;
}

/*
 * vim: et ts=4 sw=4
 */

