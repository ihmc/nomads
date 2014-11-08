//package us.ihmc.mockets.tests.messagexmit;

import java.io.FileOutputStream;
import java.io.InputStream;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;

import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.mockets.MessageServerMocket;
import us.ihmc.mockets.MessageMocket;
import us.ihmc.mockets.Sender;


public class EchoServer
{
    public static void main (String[] args) throws Exception
    {
        if (args.length != 0) {
            System.err.println ("Usage is: java EchoServer");
            System.exit (1);
        }
        
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        FileHandler fh = new FileHandler ("EchoServer%g.log");
        ConsoleHandler ch = new ConsoleHandler();
        
        ch.setLevel (Level.OFF);
        fh.setLevel (Level.FINEST);
        logger.addHandler (ch);
        logger.addHandler (fh);
        logger.setLevel (Level.OFF);

        // Create a new file to receive
        MessageMocket mocket = null;

        // Create server socket and wait for connection
        logger.info ("EchoServer: creating MessageServerMocket");
        MessageServerMocket servMocket = new MessageServerMocket (LOCAL_PORT);
        mocket = servMocket.accept();
        logger.info ("EchoServer: accepted a connection!");

        // Select reliable sequenced sender
        Sender rlsq = mocket.getSender (true, true);
        logger.info ("EchoServer: got sender!");

        // Start receiving and echoing data
        byte[] buffer = new byte[8192];
        int rr;
        while ((rr = mocket.receive (buffer, 0, buffer.length, -1)) != -1) {
            logger.fine ("server - received string: " + new String (buffer, 0, rr, "US-ASCII"));
            rlsq.send (buffer, 0, rr);
        }

        // Close mocket
        mocket.close();
    }

    static final int LOCAL_PORT = 8001;
}
/*
 * vim: et ts=4 sw=4
 */
