package us.ihmc.mockets.test;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.util.Arrays;
import java.util.Random;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import us.ihmc.mockets.*;

import us.ihmc.util.ByteConverter;

/**
 *
 * @author ebenvegnu
 */
public class DataSendFragmentation
{
    public static void main (String []args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");
        
        boolean reliable = false;
        boolean sequenced = false;

        int i = 0;
        while (i < args.length) {
            if (args[i].equals("-r") ||
                args[i].equals("--reliable")) {
                reliable = true;
            } 
            else if (args[i].equals("-s") || args[i].equals("--sequenced")) {
                sequenced = true;
            } 
            else {
                // finished option parsing
                break;
            }
            ++i;
        }

        int numAttempts = 1;
        int argsLeftToParse = args.length - i;
        String destinationAddress = null;
        switch (argsLeftToParse) {
            case 2:
                numAttempts = Integer.parseInt (args[i + 1]);
            case 1:
                destinationAddress = args[i];
                break;
            default:
                System.out.println ("usage: java DataSendFragmentation [-r|--reliable] [-s|--sequenced] hostname [numattempts]");
                System.exit (1);
        }    

        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        //FileHandler fh = new FileHandler ("DataSend%g.log");
        //fh.setLevel (Level.INFO);
        //fh.setFormatter (new SimpleFormatter());
        //logger.addHandler (fh);
        logger.setLevel (Level.OFF);

        InetAddress targetAddress = InetAddress.getByName (destinationAddress);
        byte[] data = new byte [DATA_SIZE];
        
        //Random random = new Random (1234);
        //random.nextBytes (data);

        while (numAttempts-- > 0) {
            /* open connection to the server */
            Mocket mocket = new Mocket();
            mocket.connect (new InetSocketAddress(targetAddress, REMOTE_PORT));
            Mocket.Sender sender = mocket.getSender (sequenced, reliable);
            
            //Test getPeerName
            InetSocketAddress peerName = (InetSocketAddress) mocket.getPeerName();
            if (peerName == null) {
                System.out.println ("PeerName is not set");
            }
            else {
                System.out.println("PeerName: "+peerName.toString());
            }

            /* start writing data */
            logger.info ("DataSend: Starting to send " + DATA_SIZE + " bytes of data to [" + destinationAddress + "]");
            System.out.println ("DataSend: Starting to send " + DATA_SIZE + " bytes of data to [" + destinationAddress + "]");

            int iter = 0;
            for (int k=0; k<DATA_SIZE; k++) {
                data[k]= (byte)(""+iter).charAt(0);
            }
            while (iter < NUM_ITERATIONS) {    
                sender.send (data);
                logger.info ("Sent bytes.");
                System.out.println ("Sent bytes. Iteration: "+iter);
 /*               // Sending also messages of different dimension
                sender.send (data, 0, 1024);
                logger.info ("Sent bytes.");
                System.out.println ("Sent bytes. Iteration: "+iter);
                //
                sender.send (data, 0, 50*1024);
                logger.info ("Sent bytes.");
                System.out.println ("Sent bytes. Iteration: "+iter);
                //
  */
                iter ++;
            }

            /* print statistics */
            Mocket.Statistics stats = mocket.getStatistics();
            logger.info ("Mocket statistics:");
            logger.info ("    Packets sent: " + stats.getSentPacketCount());
            logger.info ("    Bytes sent: " + stats.getSentByteCount());
            logger.info ("    Packets received: " + stats.getReceivedPacketCount());
            logger.info ("    Bytes received: " + stats.getReceivedByteCount());
            logger.info ("    Retransmits: " + stats.getRetransmittedPacketCount());
            logger.info ("    Packets discarded: " + (stats.getDuplicatedDiscardedPacketCount() + stats.getNoRoomDiscardedPacketCount()));

            /* closing connection */
            mocket.close();
        }
    }

    private static final int DATA_SIZE = 1024*500;
    private static final int REMOTE_PORT = 4000;
    private static final int NUM_ITERATIONS = 10;
    private static final String CHARSET = "US-ASCII";
}
