/**
 * The MigrationFileSend class is a testcase for the message mockets framework.
 * It reads a file from the hard drive and sends it over the network.
 *
 * NOTE: need to compile C++ mockets code on the sender node using testcase makefile
 * and running make MigrationFileSend.
 * In this way the code will be compiled in migration debugging mode: stopping odd messages from exiting the mocket.
 * The odd messages will then be sent when the connection resumes from the receiver node.
 *
 */
package us.ihmc.mockets.test;

import java.io.*;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;

import java.util.logging.Level;
import java.util.logging.Logger;


import us.ihmc.mockets.*;

import us.ihmc.util.ByteConverter;


public class MigrationFileSend
{
    public static void main (String []args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");

        final byte[] ack = "ACK".getBytes (CHARSET);
        int dataMTU = DEFAULT_DATA_MTU_LEN;

        /* process options */
        boolean optionsError = false;
        boolean suspender = false;
        String filename = null;
        String serverHost = "localhost";
        String resumerHost = "localhost";
        if (args[0].equals("suspender")) {
            suspender = true;
            // Suspender parse remaining arguments
            if (args.length < 4) {
                optionsError = true;
            }
            else {
                filename = args[1];
                serverHost = args[2];
                resumerHost = args[3];
                if (args.length == 5) {
                    // An mtu is present parse it
                    try {
                        dataMTU = Integer.parseInt (args[4]);
                    }
                    catch (NumberFormatException e) {
                        System.err.println ("data-mtu requires an integer argument");
                        optionsError = true;
                    }
                }
            }
        }
        else if (args[0].equals("resumer")) {
            // Resumer parse remaining arguments
            if (args.length != 2) {
                optionsError = true;
            }
            else {
                // parse the server name
                serverHost = args[1];
            }
        }
        else {
            optionsError = true;
        }
        if (optionsError) {
            System.out.println ("usage: java FileSend <suspender> <filename> <server_hostname> <resumer_client_hostname> [<data-mtu size>]" +
                                "usage: java FileSend <resumer> <server_hostname>");
            System.exit (1);
        }

        /* initialize logging */
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        logger.setLevel (Level.WARNING);

        /* -------------- SUSPENDER SIDE -----------------*/
        if (suspender) {
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
                InetAddress serverRemoteIP = InetAddress.getByName (serverHost);
                InetAddress resumerRemoteIP = InetAddress.getByName (resumerHost);
                byte[] data = new byte [dataMTU];

                /* open connection to the server */
                Mocket mocket = new Mocket();
                Listener listener = new Listener();
                mocket.setStatusListener(listener);
                mocket.connect (new InetSocketAddress (serverRemoteIP, SERVER_REMOTE_PORT));
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
                             " bytes of data to [" + serverHost + "]");
                long bytesSent = 0;
                int len = data.length;
                // Send the file
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

                // Suspend
                int suspendRes = mocket.suspend();
                if (suspendRes != 0) {
                    System.out.println ("Suspend ended with error. Exit");
                    System.exit (10);
                }
                else {
                    System.out.println ("Suspend ended with status "+suspendRes);
                }

                // Open a socket to transfer the data to the new client node
                Socket socket = new Socket();
                socket.connect (new InetSocketAddress (resumerRemoteIP, RESUMER_REMOTE_PORT));
                OutputStream os = socket.getOutputStream();
                int getStateRes = mocket.getState(os);
                if (getStateRes != 0) {
                    System.out.println ("GetState ended with error. Exit");
                    System.exit (10);
                }
                else {
                    System.out.println ("GetState ended with status "+suspendRes);
                }
            }
            catch (Exception x) {
                x.printStackTrace();
            }
        }

        /* -------------- RESUMER SIDE -----------------*/
        else {
            /* wait for connection */
            ServerSocket servSocket = new ServerSocket (RESUMER_REMOTE_PORT);
            Socket socket = servSocket.accept();
            InputStream is = socket.getInputStream();

            Mocket mocket = new Mocket();
            Listener listener = new Listener();
            mocket.setStatusListener (listener);
/*            int resumeRes = mocket.resumeAndRestoreState (is);
            if (resumeRes != 0) {
                System.out.println ("resumeAndRestoreState ended with error. Exit");
                System.exit (10);
            }
            else {
                System.out.println ("resumeAndRestoreState ended with status "+resumeRes);
            }
*/
            Thread.sleep (1000);

            try {
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
            catch (Exception x) {
                x.printStackTrace();
            }
        }
    }

    private static final int SERVER_REMOTE_PORT = 4000;
    private static final int RESUMER_REMOTE_PORT = 4000;
    private static final int DEFAULT_DATA_MTU_LEN = 1300;
    private static final String CHARSET = "US-ASCII";
}

