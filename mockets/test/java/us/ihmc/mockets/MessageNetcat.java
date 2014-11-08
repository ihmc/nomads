package us.ihmc.mockets.tests.messagexmit;

import java.io.BufferedReader;
import java.io.InputStreamReader;

import java.net.InetSocketAddress;

import java.util.logging.FileHandler;
import java.util.logging.ConsoleHandler;
import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.Sender;


public class MessageNetcat  
{
    /*
    private static String toHex (byte b) 
    {
        return Integer.toString((b & 0xff) + 0x100, 16).substring(1) + " ";
    }
    */
    
    public static void main (String[] args) throws Exception
    {
        if (args.length != 1) {
            System.err.println ("Usage is: java MessageNetcat hostname");
            System.exit (1);
        }

        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        FileHandler fh = new FileHandler ("MessageNetcat%g.log");
        ConsoleHandler ch = new ConsoleHandler();
        
        ch.setLevel (Level.OFF);
        fh.setLevel (Level.FINEST);
        logger.addHandler (ch);
        logger.addHandler (fh);
        logger.setLevel (Level.OFF);
        
        // Connect the mocket to the server
        InetSocketAddress isa = new InetSocketAddress (args[0], REMOTE_PORT);
        Mocket mocket = new Mocket();
        mocket.connect (isa);
        logger.info ("MessageNetcat: connected!");

        // Select reliable sequenced sender
        Sender rlsq = mocket.getSender (true, true);
        logger.info ("MessageNetcat: got sender!");
        
        // Start reading from console and forwarding data to the server
        BufferedReader stdIn = new BufferedReader (new InputStreamReader (System.in));
        String userInput;
        byte buf[] = new byte[8192];

        while ((userInput = stdIn.readLine()) != null) {
            logger.fine ("MessageNetcat: userInput = " + userInput);
            byte tmp[] = userInput.getBytes ("US-ASCII");
            /*
            String dummy = "";
            for (int i = 0; i < tmp.length; ++i)
                dummy += toHex (tmp[i]);
            logger.info (dummy);
            */
            rlsq.send (tmp, 0, tmp.length);
            logger.fine ("sent data");
            int cc = mocket.receive (buf, 0, buf.length, -1);
            System.out.println ("server echoed: " + new String (buf, 0, cc, "US-ASCII"));
        }

        // Close mocket
        mocket.close();
    }

    static final int REMOTE_PORT = 8001;
}
/*
 * vim: et ts=4 sw=4
 */
