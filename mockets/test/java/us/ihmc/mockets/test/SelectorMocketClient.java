/*
 * SelectorMocketsClient.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.mockets.test;


import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.util.Iterator;
import us.ihmc.mockets.*;


/**
 *
 * @author ebenvegnu
 */
public class SelectorMocketClient
{


    public static void main (String []args) throws Exception
    {
        if (args.length != 1) {
            System.out.println ("usage: java SelectorMocketClient <hostname>");
            System.exit (1);
        }

        System.loadLibrary("mocketsjavawrapper");

        SelectorMocketClient sms = new SelectorMocketClient();
        sms.go (args);
    }

    public SelectorMocketClient ()
    {
        return;
    }

    public void go (String []args) throws Exception
    {
        //Allocate a ServerMocketChannel
        MocketChannel channel = MocketChannel.open();
        System.out.println ("Created a MocketChannel");

        InetAddress targetAddress = InetAddress.getByName (args[0]);
        // Set the channel to non-blocking mode
        channel.configureBlocking(false);
        // Test the connect call
        boolean connect_res, isPending, isConnected, finishConnect;
        connect_res = channel.connect(new InetSocketAddress (targetAddress, REMOTE_PORT));
        System.out.println ("Called connect");
        isPending = channel.isConnectionPending();
        isConnected = channel.isConnected();
        finishConnect = channel.finishConnect();
        if (!connect_res) {
            System.out.println ("Connecting...");
        }
        else {
            System.out.println ("Connected");
        }
        if (isPending) {
            System.out.println ("Pending...");
        }
        else {
            System.out.println ("Non pending");
        }
        if (isConnected) {
            System.out.println ("Connected");
        }
        else {
            System.out.println ("Non Connected");
        }

        while (!finishConnect) {
            System.out.println ("Still trying to connect to "+args[0]+" on port "+REMOTE_PORT);
            Thread.sleep(1000);
            try {
                finishConnect = channel.finishConnect();
            }
            catch (ConnectionException ex) {
                System.out.println("Connection attempt failed!!!!");
                System.exit (1);
            }
        }
        System.out.println ("Connected to "+args[0]+" on port "+REMOTE_PORT);

        // Create a buffer to read data into
        ByteBuffer bb = ByteBuffer.allocateDirect (1024);
        // Empty buffer
        bb.clear();
        // Read the welcome message
        int res = channel.read (bb);
        System.out.println ("Read "+res+" bytes. ByteBuffer bb position is: "+bb.position());
        //System.out.println ("This is the ByteBuffer I just received: "+bb);
        bb.flip();
        //System.out.println ("ByteBuffer after flip: "+bb);
        byte[] buffer = new byte[res];
        bb.get (buffer, 0, res-1);
        System.out.println ("This is the message I just received: "+new String (buffer));

        System.out.println ("Send answer to the peer");
        // Empty buffer
        bb.clear();
        bb.put ("Thank you :)\n".getBytes());
        bb.flip();
        // Answer to the peer
        int writeRes = channel.write (bb);
        System.out.println ("Write return with status: "+writeRes);
        
        

        
        System.out.println ("Now loop....");
        while (true) {
            Thread.sleep (10000);
        }
    }

    private static final int REMOTE_PORT = 4000;
}
