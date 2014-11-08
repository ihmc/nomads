package us.ihmc.mockets.test;

import java.net.InetAddress;
import java.net.InetSocketAddress;

import java.util.Random;

import us.ihmc.mockets.Mocket;

public class MsgSend
{
    public static void main (String[] args)
        throws Exception
    {
        if (args.length != 1) {
            System.out.println ("usage: java MsgSend <hostname>");
            System.exit (1);
        }

        boolean reliable = true;
        boolean sequenced = true;
        int numMsgs = 10;
        InetAddress targetAddress = InetAddress.getByName (args[0]);
        byte[] data = new byte [MSG_LEN];
        Random random = new Random (1234);
        random.nextBytes (data);

        // Open connection to the server
        Mocket mocket = new Mocket();
        mocket.connect (new InetSocketAddress (targetAddress, REMOTE_PORT));
        Mocket.Sender sender = mocket.getSender (sequenced, reliable);

        // Send messages
        while (numMsgs-- > 0) {
            sender.send (data);
            System.out.println ("Sent message of size " + data.length);
        }
        mocket.close();
        System.out.println ("Press Enter or Return to terminate once the data has been received");
        System.in.read();
    }

    private static final int REMOTE_PORT = 4000;
    private static final int MSG_LEN = 10240;
}
