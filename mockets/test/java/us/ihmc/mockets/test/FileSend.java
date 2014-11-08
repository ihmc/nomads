/**
 * The FileSend class is a testcase for the message mockets framework.
 * It reads a file from the hard drive and sends it over the network.
 *
 * @author Mauro Tortonesi
 */
//package us.ihmc.mockets.tests.messagexmit;
package us.ihmc.mockets.test;

import java.io.*;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

//import us.ihmc.mockets.Mocket;
//import us.ihmc.mockets.Mocket;
//import us.ihmc.mockets.Sender;
import us.ihmc.mockets.*;

import us.ihmc.util.ByteConverter;


public class FileSend
{
    public static void main (String []args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");
        
        boolean useSockets = false;
        final byte[] ack = "ACK".getBytes (CHARSET);
        int dataMTU = DEFAULT_DATA_MTU_LEN;

        /* process options */
        int i = 0;
        while (i < args.length) {
            if (args[i].equals("--data-mtu")) {
                try {
                    dataMTU = Integer.parseInt (args[++i]);
                }
                catch (NumberFormatException e) {
                    System.err.println ("--data-mtu requires an integer argument");
                    System.exit (1);
                }
            } else if (args[i].equals("--sockets")) {
                useSockets = true;
            } else {
                // finished option parsing
                break;
            }
            ++i;
        }
        
        /* process remaining arguments */
        String filename = null;
        String remoteHost = "localhost";
        int argsLeftToParse = args.length - i;
        switch (argsLeftToParse) {
            case 2:
                remoteHost = args[i + 1];
            case 1:
                filename = args[i];
                break;
            default:
                System.out.println ("usage: java FileSend [--data-mtu size] " +
                                    "[--sockets] filename [hostname]");
                System.exit (1);
        }
       
        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        //FileHandler fh = new FileHandler ("FileSend%g.log");
        //fh.setLevel (Level.FINEST);
        //fh.setFormatter (new SimpleFormatter());
        //logger.addHandler (fh);
        logger.setLevel (Level.WARNING);

        /* open the file to send */
        File file = new File (filename);
        long filesize = file.length();
        if ((file == null) || (filesize == 0)) {
            System.err.println ("Could not find file "+filename);
            System.exit (1);
        }
        FileInputStream fis = new FileInputStream (file);
        BufferedInputStream bis = new BufferedInputStream (fis);
        
        try {
            InetAddress remoteIP = InetAddress.getByName (remoteHost);
            byte[] data = new byte [dataMTU];

            // Open connection to the server
            if (!useSockets) {
                /* open connection to the server */
                Mocket mocket = new Mocket();
                Listener listener = new Listener();
                mocket.setStatusListener(listener);
                mocket.connect (new InetSocketAddress (remoteIP, REMOTE_PORT));
                //System.out.println("Debug test: "+mocket.getRemoteAddress()+" "+mocket.getRemotePort());
                mocket.getPeerName();
                Mocket.Sender rlsq = mocket.getSender (true, true);
                
                /* write size of data to send */
                byte[] buffer = new byte[Mocket.getMaximumMTU()];
                //byte[] buffer = new byte[1024];
                ByteConverter.fromLongTo8Bytes (filesize, buffer);
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
                logger.info ("FileSend: Starting to send " + filesize + 
                             " bytes of data to [" + remoteHost + "]");
                long bytesSent = 0;
                int len = data.length;
                while (bytesSent < filesize) {
                    int toSend = bis.read (data);
                    
                    if (toSend < 0) {
                        System.out.println("reached the EOF");
                    }

                    rlsq.send (data, 0, toSend);
                    bytesSent += toSend;
                    logger.info ("Sent " + toSend + "/" + bytesSent + " bytes.");
                    System.out.println ("Sent " + toSend + "/" + bytesSent + " bytes.");
                }
                
                System.out.println("waiting for the ACK.");
                // wait for a confirmation that all the bytes were received.
                mocket.receive(new byte[1]);
                
                Mocket.Statistics stats = mocket.getStatistics();
                System.out.println ("Mocket statistics:");
                System.out.println ("    Packets sent: " + stats.getSentPacketCount());
                System.out.println ("    Bytes sent: " + stats.getSentByteCount());
                System.out.println ("    Packets received: " + stats.getReceivedPacketCount());
                System.out.println ("    Bytes received: " + stats.getReceivedByteCount());
                System.out.println ("    Retransmits: " + stats.getRetransmittedPacketCount());
                //System.out.println ("    Packets discarded: " + stats.getDiscardedPacketCount());
                mocket.close();
            }
            else {
                Socket socket = new Socket();
                socket.connect (new InetSocketAddress (remoteIP, REMOTE_PORT));
                OutputStream os = socket.getOutputStream();
                InputStream is = socket.getInputStream();

                DataOutputStream dos = new DataOutputStream (os);

                // Start writing data
                System.out.println ("FileSend: Starting to send " + filesize + 
                                    " bytes of data to [" + remoteHost + "]");

                dos.writeLong(filesize);
                dos.flush();

                while (true) {
                    int n;
                    if ((n = fis.read (data)) <= 0) {
                        System.out.println ("FileSend: EOF reached so closing connection");
                        break;
                    } else {
                        os.write (data, 0, n);
                        //logger.fine();
                    }
                }

                BufferedReader br = new BufferedReader (new InputStreamReader(is));
                String line = br.readLine();
                System.out.println("FileSend: received line from receiver :: '" + line + "'");

                fis.close();
                socket.close();
            }
        }
        catch (Exception x) {
            x.printStackTrace();
        }
    }

    private static final int REMOTE_PORT = 4000;
    private static final int DEFAULT_DATA_MTU_LEN = 1300;
    private static final String CHARSET = "US-ASCII";
}
/*
 * vim: et ts=4 sw=4
 */

