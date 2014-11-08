/*
 * SelectorMocketsServer.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.mockets.test;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.util.Iterator;
import us.ihmc.mockets.*;

/**
 * Test class to use a single Selector object that listens to the server mocket and all mocket channels
 *
 * @author ebenvegnu
 */
public class SelectorMocketsServer
{


    public static void main (String []args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");

        SelectorMocketsServer sms = new SelectorMocketsServer();
        sms.go (args);
    }

    public SelectorMocketsServer ()
    {
        return;
    }

    public void go (String[] args) throws Exception
    {
        //Allocate a ServerMocketChannel
        ServerMocketChannel serverChannel = ServerMocketChannel.open();
        System.out.println ("Created a ServerMocketChannel");
        // Get the associated ServerMocket
        ServerMocket serverMocket = serverChannel.mocket();
        System.out.println ("Fetched the ServerMocket undelying the mocket channel");
        // Create a new Selector
        Selector selector = Selector.open();

        // Set the port the server channel will listen to
        serverMocket.bind (LOCAL_PORT);
        System.out.println ("ServerMocketChannel is bound to the port: "+LOCAL_PORT);

        // Set non-blocking mode for the listening mocket
//        serverChannel.configureBlocking (false);

        // Register the ServerMocketChannel with the Selector
//        serverChannel.register (selector, SelectionKey.OP_ACCEPT);

        // LOOP
        while (true) {
            System.out.println ("Ready to accept incoming connections");
            MocketChannel channel = serverChannel.accept();

            System.out.println ("Accepted a new connection!!");
            
            // temporary to try read & write!!!
            sayHello (channel);
            System.out.println ("Sent greetings to the new peer");

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
            ///////////////////////////////////
/*            // This may block!! Use select(timeout) or selectNow() to avoid to blocking
            int n = selector.select();

            if (n==0) {
                continue;
            }

            // Get an iterator over the set of selected keys
            Iterator it = selector.selectedKeys().iterator();

            // Look at each key in the selected set
            while (it.hasNext()) {
                SelectionKey key = (SelectionKey)it.next();

                // Is a new connection coming in?
                if (key.isAcceptable()) {
                    ServerMocketChannel server = (ServerMocketChannel)key.channel();
                    MocketChannel channel = server.accept();

                    // Check that we really have a channel and register it with the selector
                    if (channel != null) {
                        // Set the chanel to non-blocking
                        channel.configureBlocking (false);
                        // Register it with the selctor for readiness to read action
                        channel.register(selector, SelectionKey.OP_READ);
                        // Send something to the new channel
                        sayHello (channel);
                    }
                }

                // Is there data to read on this channel?
                if (key.isReadable()) {
                    readDataFromMocket (key);
                }

                // Remove the key from the selected set
                it.remove();
            }
*/
        }
    }

    private void sayHello (MocketChannel channel) throws Exception
    {
        buffer.clear();
        buffer.put ("Welcome new peer!\n".getBytes());
        buffer.flip();

        int writeRes = channel.write (buffer);
        System.out.println ("Write return with status: "+writeRes);
    }

    private void readDataFromMocket (SelectionKey key)
    {
        // Retrieve the channel
        MocketChannel mocketChannel = (MocketChannel)key.channel();
        int count;

        // Empty buffer
        buffer.clear();
        // Loop while data is available
/*        while ((count = mocketChannel.read(buffer)) > 0) {
            // Do something with the data

            buffer.clear();
        }

        if (count < 0) {
            // Close channel on EOF, invalidate the key
            try {
                mocketChannel.close();
            }
            catch (Exception ex) {
                System.out.println ("close did not work!!");
            }
        }
*/
    }

    private static final int LOCAL_PORT = 4000;
    private ByteBuffer buffer = ByteBuffer.allocateDirect (1024);
}
