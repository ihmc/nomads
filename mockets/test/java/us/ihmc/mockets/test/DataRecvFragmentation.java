
package us.ihmc.mockets.test;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import java.net.ServerSocket;
import java.net.Socket;

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
public class DataRecvFragmentation
{
    public static void main (String[] args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");
        
        boolean enqueue = false;
        
        int i = 0;

        while (i < args.length) {
            if (args[i].equals("-e") ||
                args[i].equals("--enqueue")) {
                enqueue = true;
            }
            else {
                // finished option parsing
                break;
            }
            ++i;
        }
        
        int argsLeftToParse = args.length - i;
        if (argsLeftToParse != 0) {
            System.out.println ("usage: java DataRecvFragmentation [-e|--enqueue]");
            System.exit (1);
        }

        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        //FileHandler fh = new FileHandler ("DataRecv%g.log");
        //fh.setLevel (Level.INFO);
        //fh.setFormatter (new SimpleFormatter());
        //logger.addHandler (fh);
        logger.setLevel (Level.OFF);

        /* create server socket and wait for connection */
        System.out.println ("DataRecv: creating ServerMocket");
        ServerMocket servMocket = new ServerMocket (LOCAL_PORT);
        //byte[] buffer = new byte[Mocket.getMaximumMTU()];
        byte[] buffer = new byte[DATA_SIZE];    // Create a very big buffer to be able to hold big messages from the client

        /* wait for connection */
        Mocket mocket = servMocket.accept();
        Mocket.Sender rlsq = mocket.getSender (true, true);
        System.out.println ("DataRecv: ServerMocket accepted a connection");

        /* start receiving data */
        System.out.println ("DataRecv: start receiving Data");
        long bytesRead = 1;
        long startTime = System.currentTimeMillis();
        int iter = 0;
        int nextMsgSize = 0;
        
        if (!enqueue) {
            while (true) {
                // Test getNextMessageSize: this function can be used to allocate a buffer
                // of the right size when the size of the messages we are going to receive is unknown.
                nextMsgSize = mocket.getNextMessageSize(-1);
                System.out.println("Ready to receive: "+nextMsgSize+" bytes");
                bytesRead = mocket.receive (buffer);
                if (bytesRead > 0) {
                    System.out.println ("DataRecv: received "+bytesRead+" bytes. Iteration: "+iter);
                    iter++;
                } 
                else {
                    System.out.println ("DataRecv: receive returned 0 so closing connection");
                    break;
                }
 //               Thread.sleep(100);
            }
        }
        else{
            int length = DATA_SIZE;
            int offset = 0;
            while (true) {
                System.out.println (" offset = "+offset+" buffer.length = "+buffer.length);
                bytesRead = mocket.receive (buffer, offset, buffer.length-offset);
                if (bytesRead > 0) {
                    System.out.println ("DataRecv: received "+bytesRead+" bytes. Iteration: "+iter);
                    //System.out.println ("Buffer length: "+ new String(buffer, 0, buffer.length));
                    iter++;
                    offset += bytesRead;
                } 
                else {
                    System.out.println ("DataRecv: receive returned 0 so closing connection.");
                    break;
                }
            }
        }
        

        Mocket.Statistics stats = mocket.getStatistics();
        System.out.println ("Mocket statistics:");
        System.out.println ("    Packets sent: " + stats.getSentPacketCount());
        System.out.println ("    Bytes sent: " + stats.getSentByteCount());
        System.out.println ("    Packets received: " + stats.getReceivedPacketCount());
        System.out.println ("    Bytes received: " + stats.getReceivedByteCount());
        System.out.println ("    Retransmits: " + stats.getRetransmittedPacketCount());
        System.out.println ("    Packets discarded: " + (stats.getDuplicatedDiscardedPacketCount() + stats.getNoRoomDiscardedPacketCount()));

        long time = System.currentTimeMillis() - startTime;
        System.out.println ("Time to receive " + bytesRead + " bytes = " + time + " (" + (bytesRead * 1000 / time) + " bytes/sec)");

        saveStats (mocket, "receiver", time);
        mocket.close();

    }

    private static void saveStats (Mocket mocket, String type, long txTime)
    {
        String outFile = "stats-" + type + ".txt";

        try {
            FileOutputStream fos = new FileOutputStream(outFile, true);
            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(fos));

            Mocket.Statistics stats = mocket.getStatistics();

            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t" + txTime
                        + "\t" + stats.getSentPacketCount()
                        + "\t" + stats.getSentByteCount()
                        + "\t" + stats.getReceivedByteCount()
                        + "\t" + stats.getRetransmittedPacketCount()
                        + "\t" + (stats.getDuplicatedDiscardedPacketCount() + stats.getNoRoomDiscardedPacketCount())
                        ;

            bw.write(line);
            bw.newLine();
            bw.flush();

            bw.close();
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private static void saveStats (Socket socket, String type, long txTime)
    {
        String outFile = "stats-" + type + ".txt";

        try {
            FileOutputStream fos = new FileOutputStream(outFile, true);
            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(fos));

            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t" + txTime
                        ;

            bw.write(line);
            bw.newLine();
            bw.flush();

            bw.close();
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private static final int LOCAL_PORT = 4000;
    private static final String CHARSET = "US-ASCII";
    private static final int DATA_SIZE = 1024*1024*10; // Used with enque method
}
