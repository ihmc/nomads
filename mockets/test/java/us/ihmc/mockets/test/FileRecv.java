/**
 * The FileRecv class is a testcase for the message mockets framework.
 * It reads a file from the network and writes it on the hard drive.
 *
 * @author Mauro Tortonesi
 */
//package us.ihmc.mockets.tests.messagexmit;
package us.ihmc.mockets.test;

import java.io.*;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.LogRecord;
import java.util.logging.SimpleFormatter;
import java.util.logging.SocketHandler;

//import us.ihmc.mockets.Mocket;
//import us.ihmc.mockets.ServerMocket;
//import us.ihmc.mockets.Mocket;
//import us.ihmc.mockets.Sender;
import us.ihmc.mockets.*;

import us.ihmc.util.ByteConverter;


public class FileRecv
{
    public static void main (String[] args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");
        
        boolean useSockets = false;
        final byte[] ack = "ACK".getBytes (CHARSET);
        int dataMTU = Mocket.getMaximumMTU();
        //int dataMTU = 1024;
        
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

        /* process remaining arguments - we should have only a filename */
        int argsLeftToParse = args.length - i;
        if (argsLeftToParse != 1) {
            System.out.println ("usage: java FileRecv [--data-mtu size] " + 
                                "[--sockets] filename");
            System.exit (1);
        }
        
        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        //FileHandler fh = new FileHandler ("FileRecv%g.log");
        //fh.setLevel (Level.INFO);
        //fh.setFormatter (new SimpleFormatter());
        //logger.addHandler (fh);
        logger.setLevel (Level.WARNING);
        
        /* Create a new file to receive */
        FileOutputStream fos = new FileOutputStream (args[i]);
        BufferedOutputStream bos = new BufferedOutputStream (fos);

        /* create server socket and wait for connection */
        if (!useSockets) {
            System.out.println ("FileRecv: creating ServerMocket");
            ServerMocket servMocket = new ServerMocket (LOCAL_PORT);
            //byte[] buffer = new byte[dataMTU];
            byte[] buffer = new byte[1024];
            /* wait for connection */
            Mocket mocket = servMocket.accept();
            Listener listener = new Listener();
            mocket.setStatusListener(listener);
            Mocket.Sender rlsq = mocket.getSender (true, true);
            System.out.println ("FileRecv: ServerMocket accepted a connection");
            
            /* read size of data to receive */
            int rr = mocket.receive (buffer, 0, buffer.length);
            if (rr != 8) {
                System.err.println ("Wrong data size message");
                System.exit (10);
            }
            long datasize = ByteConverter.from8BytesToLong (buffer, 0);
            System.out.println ("FileRecv: will read [" + datasize + "] bytes.");

            /* send ack */
            System.arraycopy (ack, 0, buffer, 0, ack.length);
            rlsq.send (buffer, 0, ack.length);
            
            /* start receiving data */
            System.out.println ("FileRecv: start receiving Data");
            long bytesRead = 0;
            long startTime = System.currentTimeMillis();
            
            while (bytesRead < datasize) {
                //long bytesToRead = Math.min ((long)buffer.length, datasize - bytesRead);
                //if (bytesToRead == 0) {
                //   break;
                //}
                byte[] buffer2 = null;
                if ((buffer2 = mocket.receive(0)) != null) {
                    bos.write (buffer2, 0, buffer2.length);
                    bytesRead += buffer2.length;
                    float speed = (float)(bytesRead * 1000)/ 
                                  (float)(System.currentTimeMillis() - startTime);
                    System.out.println ("FileRecv: received " + buffer2.length + " bytes (" + bytesRead + 
                                        " total) - average bandwidth: " + speed + " bytes/sec");
                } 
                else {
                    System.out.println ("FileRecv: EOF reached - closing connection");
                    break;
                }
            }
            
            System.out.println ("sending the ACK.");
            rlsq.send(new byte[]{(byte)'.'});
            
            bos.flush();
            bos.close();
            fos.close();
        
            Mocket.Statistics stats = mocket.getStatistics();
            System.out.println ("Mocket statistics:");
            System.out.println ("    Packets sent: " + stats.getSentPacketCount());
            System.out.println ("    Bytes sent: " + stats.getSentByteCount());
            System.out.println ("    Packets received: " + stats.getReceivedPacketCount());
            System.out.println ("    Bytes received: " + stats.getReceivedByteCount());
            //System.out.println ("    Retransmits: " + stats.getRetransmittedPacketCount());
            //System.out.println ("    Packets discarded: " + stats.getDiscardedPacketCount());

            long time = System.currentTimeMillis() - startTime;
            System.out.println ("Time to receive " + bytesRead + " bytes = " + time + 
                                " (" + (bytesRead * 1000 / time) + " bytes/sec)");
            
            saveStats (mocket, "receiver", time);
            mocket.close();
        } else {
            System.out.println ("FileRecv: creating ServerSocket");
            ServerSocket servSocket = new ServerSocket (LOCAL_PORT);
            //byte[] buffer = new byte[Mocket.getMaximumMTU()];
            byte[] buffer = new byte[1024];
            
            /* wait for connection */
            Socket socket = servSocket.accept();
            System.out.println ("FileRecv: ServerSocket accepted a connection");
            InputStream is = socket.getInputStream();
            OutputStream os = socket.getOutputStream();
            DataInputStream dis = new DataInputStream (is);
            
            /* read size of data to receive */
            long datasize = dis.readLong();
            System.out.println ("FileRecv: will read [" + datasize + "] bytes.");
            
            /* start receiving data */
            System.out.println ("FileRecv: start receiving Data");
            long bytesRead = 0;
            long startTime = System.currentTimeMillis();

            while (true) {
                long bytesToRead = Math.min ((long)buffer.length, datasize - bytesRead);
                if (bytesToRead == 0) {
                    break;
                }
                if ((i = is.read (buffer, 0, (int)bytesToRead)) > 0) {
                    //Thread.sleep(2000);
                    bos.write(buffer, 0, i);
                    bytesRead += i;
                    float speed = (float)(bytesRead * 1000)/ (float)(System.currentTimeMillis() - startTime);
                    System.out.println ("FileRecv: received " + i + " bytes (" + bytesRead + " total)" +
                                        " - average bandwidth: " + speed + " bytes/sec");
                } else {
                    System.out.println ("FileRecv: EOF reached - closing connection");
                    break;
                }
            }

            /* send ack */
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(os));
            bw.write ("OK\n");
            bw.flush();

            /* print and save statistics */
            long time = System.currentTimeMillis() - startTime;
            System.out.println ("Time to receive " + bytesRead + " bytes = " + time + " (" + (bytesRead * 1000 / time) + " bytes/sec)");
            saveStats (socket, "receiver", time);
            
            /* close connection */
            socket.close();
        }
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
                        + "\t" + stats.getReceivedByteCount();
                        //+ "\t" + stats.getRetransmittedPacketCount()
                        //+ "\t" + stats.getDiscardedPacketCount();
                        

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
    private static final int DEFAULT_DATA_MTU_LEN = 1300;
    private static final String CHARSET = "US-ASCII";
}
/*
 * vim: et ts=4 sw=4
 */

