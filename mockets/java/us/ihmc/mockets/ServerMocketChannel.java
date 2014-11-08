/*
 * ServerMocketChannel.java
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 * 
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 * 
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 
 * @author ebenvegnu
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.nio.channels.SelectionKey;
import java.nio.channels.spi.AbstractSelectableChannel;
import java.nio.channels.spi.SelectorProvider;

/****/
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.channels.Selector;
import java.nio.channels.spi.SelectorProvider;
/****/

public class ServerMocketChannel extends AbstractSelectableChannel
{
    /*
     * Initializes a new instance of this class.
     * 
     */
    protected ServerMocketChannel (SelectorProvider provider)
            throws IOException
    {
        super (provider);
        _blockingMode = true;
        _serverMocket = new ServerMocket();
    }

    /*
     * Opens a server mocket channel.
     * <p>
     * The new channel is created by invoking the openServerMocketChannel method of the system-wide default SelectorProvider object.
     * The new channel's mocket is initially unbounded; it must be bound to a specific address via one of its mocket's bind methods before connections can be accepted.
     * 
     * @return  A new mocket channel
     */
    public static ServerMocketChannel open()
            throws IOException
    {
        return (MocketSelectorProvider.getProvider()).openServerMocketChannel();
    }

    /*
     * Retrieves the server mocket associated with this channel.
     *
     * @return  A server mocket associated with this channel
     */
    public ServerMocket mocket()
    {
        if (_serverMocket != null) {
            return _serverMocket;
        }
        return null;
    }

    /*
     * Returns an operation set identifying this channel's supported operations.
     * <p>
     * Server-mocket channels only support accepting of new connections, so this method returns SelectionKey.OP_CCEPT.
     *
     * @return  The valid-operation set
     */
    public final int validOps()
    {
        return SelectionKey.OP_ACCEPT;
    }

    /*
     * Accepts a connection made to this channel's mocket.
     * <p>
     * If this channel is in non-blocking mode then this method will immediately return null if there are no pending connections.
     * Otherwise it will block indefinitely until a new connection is available or an I/O error occurs.
     * <p>
     * The mocket channel returned by this method, if any, will be in blocking mode regardless of the blocking mode of this channel.
     *
     * @return  The mocket channel for the new connection, or null if this channel is in non-blocking mode and no connection is available to be accepted
     */
    public MocketChannel accept()
            throws IOException
    {
        return MocketChannel.open (_serverMocket.accept());
    }

    /**
     * Closes this selectable channel.
     *
     * <p> This method is invoked by the {@link java.nio.channels.Channel#close
     * close} method in order to perform the actual work of closing the
     * channel.  This method is only invoked if the channel has not yet been
     * closed, and it is never invoked more than once.
     *
     * <p> An implementation of this method must arrange for any other thread
     * that is blocked in an I/O operation upon this channel to return
     * immediately, either by throwing an exception or by returning normally.
     * </p>
     */
    protected void implCloseSelectableChannel() throws IOException
    {
        // TODO
        throw new UnsupportedOperationException("Not supported yet.");
    }

    /**
     * Adjusts this channel's blocking mode.
     *
     * <p> This method is invoked by the {@link #configureBlocking
     * configureBlocking} method in order to perform the actual work of
     * changing the blocking mode.  This method is only invoked if the new mode
     * is different from the current mode.  </p>
     *
     * @throws IOException
     *         If an I/O error occurs
     */
    protected void implConfigureBlocking(boolean block)
            throws IOException
    {
        _blockingMode = block;
    }

    ServerMocket _serverMocket;
    private boolean _blockingMode;

}
