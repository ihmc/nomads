//package us.ihmc.mockets.tests.messagexmit;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;

import java.util.Arrays;
import java.util.Random;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import us.ihmc.mockets.Mocket;

import us.ihmc.util.ByteConverter;


public class DataSend
{
    public static void main (String []args) throws Exception
    {
        final byte[] ack = "ACK".getBytes (CHARSET);
        boolean useSockets = false;
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
            else if (args[i].equals("--sockets")) {
                useSockets = true;
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
                System.out.println ("usage: java DataSend [--sockets] [-r|--reliable] [-s|--sequenced] hostname [numattempts]");
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
        byte[] data = new byte [ARRAY_LEN];
        Random random = new Random (1234);
        random.nextBytes (data);

        while (numAttempts-- > 0) {
            if (useSockets) {
                /* open connection to the server */
                Socket socket = new Socket (targetAddress, REMOTE_PORT);
                InputStream is = socket.getInputStream();
                OutputStream os = socket.getOutputStream();
                DataOutputStream dos = new DataOutputStream (os);
                
                /* write size of data to send */
                logger.info ("DataSend: Starting to send " + DATA_SIZE + " bytes of data to [" + destinationAddress + "]");
                dos.writeLong (DATA_SIZE);
                dos.flush();
                
                /* start writing data */
                long bytesSent = 0;
                int len = data.length;
                while (bytesSent < DATA_SIZE) {
                    os.write (data, 0, len);
                    bytesSent += len;
                }

                /* reading line from receiver */
                BufferedReader br = new BufferedReader (new InputStreamReader(is));
                String line = br.readLine();
                logger.info ("DataSend: received line from receiver :: '" + line + "'");

                /* closing connection */
                socket.close();
            } 
            else {
                /* open connection to the server */
                Mocket mocket = new Mocket();
                mocket.connect (new InetSocketAddress(targetAddress, REMOTE_PORT));
                Mocket.Sender sender = mocket.getSender (sequenced, reliable);
                Sender rlsq = mocket.getSender (true, true);

                /* write size of data to send */
                byte[] buffer = new byte[Mocket.getMaximumMTU()];
                ByteConverter.fromLongTo8Bytes (DATA_SIZE, buffer);
                rlsq.send (buffer, 0, 8); 

                /* wait ack from data receiver */
                int rr = mocket.receive (buffer, 0, buffer.length);
                if (rr != ack.length) {
                    System.err.println ("Wrong ack");
                    System.exit (10);
                }
                
                /* check that answer is actually "ACK" */
                for (int j = 0; j < ack.length; ++j) {
                    if (ack[j] != buffer[j]) {
                        System.err.println ("Wrong ack");
                        System.exit (20);
                    }
                }

                /* start writing data */
                logger.info ("DataSend: Starting to send " + DATA_SIZE + " bytes of data to [" + destinationAddress + "]");
                long bytesSent = 0;
                int len = data.length;
                while (bytesSent < DATA_SIZE) {
                    int toSend = (int)Math.min (DATA_SIZE - bytesSent, (long)len);
                    sender.send (data, 0, toSend);
                    logger.info ("Sent " + toSend + "/" + bytesSent + " bytes.");
                    bytesSent += toSend;
                }
                
                /* print statistics */
                Mocket.Statistics stats = mocket.getStatistics();
                logger.info ("Mocket statistics:");
                logger.info ("    Packets sent: " + stats.getSentPacketCount());
                logger.info ("    Bytes sent: " + stats.getSentByteCount());
                logger.info ("    Packets received: " + stats.getReceivedPacketCount());
                logger.info ("    Bytes received: " + stats.getReceivedByteCount());
                logger.info ("    Retransmits: " + stats.getRetransmittedPacketCount());
                logger.info ("    Packets discarded: " + stats.getDiscardedPacketCount());
                
                /* closing connection */
                mocket.close();
            }
        }
    }
    
    private static final long MEGA_BYTE = 1024 * 1024;
    //private static final long DATA_SIZE = 2 * MEGA_BYTE;
    private static final long DATA_SIZE = MEGA_BYTE / 4;
    private static final int REMOTE_PORT = 4000;
    private static final int ARRAY_LEN = 1367 /* Mocket.getMaximumMTU() - 20 */;
    private static final String CHARSET = "US-ASCII";
}
/*
 * vim: et ts=4 sw=4
 */
