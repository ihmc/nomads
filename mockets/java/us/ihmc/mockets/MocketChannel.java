/*
 * MocketChannel.java
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
 *
 * @author ebenvegnu
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.spi.AbstractSelectableChannel;
import java.nio.channels.spi.SelectorProvider;

public class MocketChannel extends AbstractSelectableChannel
{
    /*
     * Initializes a new instance of this class.
     */
    protected MocketChannel (SelectorProvider provider)
        throws IOException
    {
        super (provider);
        _blockingMode = true;
        _isConnected = false;
        _isConnectionPending = false;
        _mocket = new Mocket();
    }

    /*
     * Initializes a new instance of this class given an existing Mocket.
     * <p>
     * Used when the new Mocket is created by an accept call on a ServerMocket.
     */
    protected MocketChannel (SelectorProvider provider, Mocket m)
    {
        super (provider);
        _blockingMode = true;
        _isConnected = false;
        _isConnectionPending = false;
        _mocket = m;
    }

    /*
     * Opens a mocket channel.
     * <p>
     * The new channel is created by invoking the openMocketChannel method of the system-wide default SelectorProvider object
     *
     * @return  A new mocket channel
     */
    public static MocketChannel open()
            throws IOException
    {
        return (MocketSelectorProvider.getProvider()).openMocketChannel();
    }

    /*
     * Opens a mocket channel.
     * <p>
     * The new channel is created by invoking the openMocketChannel method of the system-wide default SelectorProvider object.
     * <p>
     * Used when the new Mocket is created by an accept call on a ServerMocket.
     *
     * @return  A new mocket channel
     */
    public static MocketChannel open (Mocket m)
            throws IOException
    {
        if (m == null) {
            System.out.println ("ERROR The created Mocket is null!!!");
            return null;
        }
        return (MocketSelectorProvider.getProvider()).openMocketChannel (m);
    }

    /*
     * Opens a mocket channel and connects it to a remote address.
     * <p>
     * This convenience methos works as if by invoking the open() method,
     * invoking the connect method upon the resulting mocket channel, passing it remote, and then returning that channel.
     *
     * @param remoteHost     The remote address to which the new channel is to be connected
     * @param remotePort    The remote port to which the new channel is to be connected
     * @return  A new mocket channel
     */
    public static MocketChannel open (String remoteHost,int remotePort)
            throws IOException, ConnectionException
    {

        MocketChannel mc = (MocketSelectorProvider.getProvider()).openMocketChannel();
        mc.connect (remoteHost, remotePort);
        return mc;
    }

    /*
     * Returns an operation set identifying this channel's supported operations.
     * <p>
     * Mocket channels support connecting, reading and writing, so this method returns
     * (SelectionKey.OP_CONNECT  | SelectionKey.OP_READ | SelectionKey.OP_WRITE).
     *
     * @return The valid-operation set
     */
    public int validOps()
    {
        return (SelectionKey.OP_CONNECT  | SelectionKey.OP_READ | SelectionKey.OP_WRITE);
    }

    /*
     * Retrieves a mocket associated with this channel.
     *
     * @return A mocket associated with this channel
     */
    public Mocket mocket()
    {
        return _mocket;
    }

    /*
     * Tells weather or not this channel's network mocket is connected.
     *
     * @return  <code>true</code> if and only if this channel's network mocket is conected
     */
    public boolean isConnected()
    {
        return _isConnected;
    }

    /*
     * Tells weather or not a connection operation is in progress on this channel.
     *
     * @return  <code>true</code> if and only if a conection operation has been initiated on this channel but not yet completed by invoking the finishConnect method
     */
    public boolean isConnectionPending()
    {
        return _isConnectionPending;
    }

    /*
     * Connects this channel's mocket.
     * <p>
     * If this channel is in non-blocking mode then an invocation of this method initiates a non-blocking connection operation.
     * If the connection is established immediately, as can happen with a local connection, then this method returns true.
     * Otherwise this method returns false and the connection operation must later be completed by invoking the finishConnect method.
     * <p>
     * If this channel is in blocking mode then an invocation of this method will block until the connection is established or an I/O error occurs.
     *
     * @param remote    The remote address to which this channel is to be connected
     * @return  <code>true</code> if a connection was established, <code>false</code> if this channel is in non-blocking mode and the connection operation is in progress
     */
    public boolean connect (String remoteHost,int remotePort)
            throws IOException, ConnectionException
    {
        _isConnectionPending = true;
        
        if (_blockingMode) {
            // Blocking mode
            int res = 0;
            try {
                res = _mocket.connect(remoteHost, remotePort);
            }
            catch (IOException ex) {
                ex.printStackTrace();
                _isConnectionPending = false;
                return false;
            }
            if (res == 0) {
                _isConnectionPending = false;
                _isConnected = true;
                return true;
            }
            else {
                _isConnectionPending = false;
                // Throw an exception
                throw new ConnectionException();
            }
        }
        // Non-blocking mode
        _mocket.connectAsync(remoteHost, remotePort);
        return false;
    }

    /*
     * Connects this channel's mocket.
     * <p>
     * If this channel is in non-blocking mode then an invocation of this method initiates a non-blocking connection operation.
     * If the connection is established immediately, as can happen with a local connection, then this method returns true.
     * Otherwise this method returns false and the connection operation must later be completed by invoking the finishConnect method.
     * <p>
     * If this channel is in blocking mode then an invocation of this method will block until the connection is established or an I/O error occurs.
     *
     * @param remote    The remote address to which this channel is to be connected
     * @return  <code>true</code> if a connection was established, <code>false</code> if this channel is in non-blocking mode and the connection operation is in progress
     */
    public boolean connect (SocketAddress remoteAddress)
            throws IOException, ConnectionException
    {
        _isConnectionPending = true;

        if (_blockingMode) {
            // Blocking mode
            int res = 0;
            try {
                res = _mocket.connect(remoteAddress);
            }
            catch (IOException ex) {
                ex.printStackTrace();
                _isConnectionPending = false;
                return false;
            }
            if (res == 0) {
                _isConnectionPending = false;
                _isConnected = true;
                return true;
            }
            else {
                _isConnectionPending = false;
                // Throw an exception
                throw new ConnectionException();
            }
        }
        // Non-blocking mode
        _mocket.connectAsync(remoteAddress);
        return false;
    }

    /*
     * Finishes the process of connecting a mocket channel
     * <p>
     * A non-blocking connection operation is initiated by placing a mocket channel in non-blocking mode and then invoking its connect method.
     * Once the connection is established, or the attempt has failed, the mocket channel will become connectable and this method
     * may be invoked to complete the connection sequence. If the connection operation failed then invoking this method will cause
     * an appropriate IOException to be thrown.
     * <p>
     * If this channel is already connected then this method will not block and will immediately return true.
     * If this channel is in non-blocking mode then this method will return false if the connection process is not yet complete.
     * If this channel is in blocking mode then this method will block until the connection either completes or fails,
     * and will always either return true or throw a checked exception describing the failure.
     * <p>
     * This method may be invoked at any time. If a read or write operation upon this channel is invoked while an invocation of this
     * method is in progress then that operation will first block until this invocation is complete. If a connection attempt fails,
     * that is, if an invocation of this method throws a checked exception, then the channel will be closed.
     *
     * @return  <code>true</code> if and only if this channel's mocket is now connected
     */
    public boolean finishConnect()
            throws InterruptedException, ConnectionException
    {
        // Check if this channel is already connected
        if (_isConnected) {
            return true;
        }
        // Check whether a connection has been started but not yet completed
        if (_isConnectionPending) {
            int connected = 0;
            // If this channel is in non-blocking mode check if the connection process is complete
            if (!_blockingMode) {
                connected = _mocket.finishConnect();
            }
            // If this channel is in blocking mode block until the connection process completes or fails
            // Note: this happens only if the channel was in non-blocking mode when connect was called and
            // had since been changed to blocking mode (does this make sense??).
            else {
                while (connected == 0) {
                    connected = _mocket.finishConnect();
                    Thread.sleep(30);
                }
            }

            if (connected == 1) {
                // Connection established
                _isConnectionPending = false;
                _isConnected = true;
                return true;
            }
            else if (connected < 0) {
                // An error occurred during the connection process
                _isConnectionPending = false;
                // Throw an exception
                throw new ConnectionException("Error code "+connected);
            }
        }
        return false;
    }

    /**
     * Retrieves the data from next message that is ready to be delivered to the application.
     * <p>
     * This is the regular Mocket style receive.
     *
     * @param dst The buffer into wich bytes are to be transferred
     * @return  The number of bytes read, possibly zero, or <code>-1</code> if the channel has reached end-of-stream
     */
    public int receive (ByteBuffer dst)
    {
        byte[] buffer = new byte[dst.remaining()];
        int res = 0;
        try {
            if (!_blockingMode) {
                // If the mode is non-blocking we call receive with a timeout of 1 ms.
                res = _mocket.receive (buffer, 0, buffer.length, 1);
            }
            else {
                // If we are in blocking mode we wait indefinitely (-1 timeout value)
                res = _mocket.receive (buffer, 0, buffer.length, -1);
            }
        }
        catch (IOException ioEx) {
            System.out.println ("MocketChannel read IOException: "+ioEx);
            return -10;
        }
        catch (IllegalArgumentException iaEx) {
            System.out.println ("MocketChannel read IllegalArgumentException: "+iaEx);
            return -10;
        }
        dst.put (buffer, 0, res);   // No need to copy the whole buffer since we have res bytes in it (res may be equal to buffer.length)
        return res;
    }

    /*
     * Reads a sequence of bytes from this channel into the given buffer.
     * <p>
     * An attempt is made to read up to r bytes from the channel,
     * where r is the number of bytes remaining in the buffer, that is, dst.remaining(), at the moment this method is invoked.
     * <p>
     * Suppose that a byte sequence of length n is read, where 0 <= n <= r. This byte sequence will be transferred into the buffer
     * so that the first byte in the sequence is at index p and the last byte is at index p + n - 1, where p is the buffer's position
     * at the moment this method is invoked. Upon return the buffer's position will be equal to p + n; its limit will not have changed.
     * <p>
     * A read operation might not fill the buffer, and in fact it might not read any bytes at all. Whether or not it does so depends
     * upon the nature and state of the channel. A socket channel in non-blocking mode, for example, cannot read any more bytes than
     * are immediately available from the socket's input buffer; similarly, a file channel cannot read any more bytes than remain in
     * the file. It is guaranteed, however, that if a channel is in blocking mode and there is at least one byte remaining in the
     * buffer then this method will block until at least one byte is read.
     * <p>
     * This method may be invoked at any time. If another thread has already initiated a read operation upon this channel, however,
     * then an invocation of this method will block until the first operation is complete.
     * <p>
     * This read correspond to <code>int read (ByteBuffer dst)</code> of the SocketChannel class.
     *
     * @param dst   The buffer into wich bytes are to be transferred
     * @return  The number of bytes read, possibly zero, or <code>-1</code> if the channel has reached end-of-stream
     */
    public int read (ByteBuffer dst)
    {
        byte[] buffer = new byte[dst.remaining()];
        int res = 0;
        try {
            if (!_blockingMode) {
                // If the mode is non-blocking we call receive with a timeout of 1 ms.
                res = _mocket.receive (buffer, 0, buffer.length, 1);
            }
            else {
                // If we are in blocking mode we wait indefinitely (-1 timeout value)
                res = _mocket.receive (buffer, 0, buffer.length, -1);
            }
        }
        catch (IOException ioEx) {
            System.out.println ("MocketChannel read IOException: "+ioEx);
            return -10;
        }
        catch (IllegalArgumentException iaEx) {
            System.out.println ("MocketChannel read IllegalArgumentException: "+iaEx);
            return -10;
        }
        dst.put (buffer, 0, res);   // No need to copy the whole buffer since we have res bytes in it (res may be equal to buffer.length)
        return res;
    }

    /*
     * Reads a sequence of bytes from this channel into a subsequence of the given buffers.
     * <p>
     * An invocation of this method attempts to read up to r bytes from this channel, where r
     * is the total number of bytes remaining the specified subsequence of the given buffer array.
     * <p>
     * Suppose that a byte sequence of length n is read, where 0 <= n <= r. Up to the first dsts[offset].remaining() bytes of this
     * sequence are transferred into buffer dsts[offset], up to the next dsts[offset+1].remaining() bytes are transferred into buffer
     * dsts[offset+1], and so forth, until the entire byte sequence is transferred into the given buffers. As many bytes as possible
     * are transferred into each buffer, hence the final position of each updated buffer, except the last updated buffer, is guaranteed
     * to be equal to that buffer's limit.
     * <p>
     * This method may be invoked at any time. If another thread has already initiated a read operation upon this channel, however,
     * then an invocation of this method will block until the first operation is complete.
     * <p>
     * This read correspond to <code>long read (ByteBuffer[] dsts, int offset, int length)</code> of the SocketChannel class.
     *
     * @param dsts      The buffers into which bytes are to be transferred
     * @param offset    The offset within the buffer array of the first buffer into which bytes are to be transferred; must be non-negative and no larger than dsts.length
     * @param length    The maximum number of buffers to be accessed; must be non-negative and no larger than dsts.length - offset
     *
     * @return          The number of bytes read, possibly zero, or -1 if the channel has reached end-of-stream
     *
     */
    public long read (ByteBuffer[] dsts, int offset, int length)
    {
        // Check the parameters
        if ((offset < 0) || (offset > dsts.length)) {
            throw new IllegalArgumentException ("MocketChannel read: IllegalArgumentException offset parameter out of bound.");
        }
        if ((length < 0) || (length > dsts.length - offset)) {
            throw new IllegalArgumentException ("MocketChannel read: IllegalArgumentException length parameter out of bound.");
        }
        // Count how many bytes we can accept with the read
        int dstsCount = 0;
        for (int i=0; i<length-offset; i++) {
            dstsCount += dsts[i].remaining();
        }
        byte[] buffer = new byte[dstsCount];
        int res = 0;
        try {
            if (!_blockingMode) {
                // If the mode is non-blocking we call receive with a timeout of 1 ms.
                res = _mocket.receive (buffer, 0, buffer.length, 1);
            }
            else {
                // If we are in blocking mode we wait indefinitely (-1 timeout value)
                res = _mocket.receive (buffer, 0, buffer.length, -1);
            }
        }
        catch (IOException ioEx) {
            System.out.println ("MocketChannel read IOException: "+ioEx);
            return -10;
        }
        catch (IllegalArgumentException iaEx) {
            System.out.println ("MocketChannel read IllegalArgumentException: "+iaEx);
            return -10;
        }
        int remaining = res;
        int bufferOffset = 0;
        int i = offset;
        while (remaining > 0) {
            if (dsts[i].remaining() < remaining) {
                dsts[i].put(buffer, bufferOffset, dsts[i].remaining());
                bufferOffset += dsts[i].remaining();
            }
            else {
                // Last bytes to be copied
                dsts[i].put(buffer, bufferOffset, remaining);
            }
            i++;
        }
        return res;
    }

    /*
     * Reads a sequence of bytes from this channel into the given buffers.
     * <p>
     * An invocation of this method of the form c.read(dsts)  behaves in exactly the same manner as the invocation
     * <code>c.read(dsts, 0, srcs.length);</code>
     * <p>
     * This read correspond to <code>long read (ByteBuffer[] dsts)</code> of the SocketChannel class.
     *
     * @param dsts  The buffers into wich bytes are to be transferred
     *
     * @return      The number of bytes read, possibly zero, or <code>-1</code> if the channel has reached end-of-stream
     */
    public long read (ByteBuffer[] dsts)
    {
        return read (dsts, 0, dsts.length);
    }

    /**
     * Enqueues the specified data for transmission.
     * <p>
     * This is the regular Mocket style send.
     * 
     * @param reliable      Select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced     Select the type of flow: sequenced (true) or unsequenced (false).
     * @param src           The buffer from wich bytes are to be retrieved
     * 
     * @return              The number of bytes written, possibly zero
     */
    public int send (boolean reliable, boolean sequenced, ByteBuffer src)
    {
        // TODO: evaulate non-blocking mode
        byte[] buffer = new byte[src.remaining()];
        src.get(buffer);
        //System.out.println ("MocketChannel write: sending buffer: "+new String (buffer));
        // Using default parameters for the send call and using reliable sequenced service
        int defaultTag = 0;
        short defaultPriority = 5;
        long defaultEnqueueTimeout = 0;
        long defaultRetryTimeout = 0;
        int res = 0;
        try {
            res = _mocket.send (reliable, sequenced, buffer, defaultTag, defaultPriority, defaultEnqueueTimeout, defaultRetryTimeout);
        }
        catch (IOException ex) {
            System.out.println ("MocketChannel write: IOException during send call");
            return -10;
        }
        return res;
    }

    /**
     * Enqueues the specified data for transmission.
     * <p>
     * This is the regular Mocket style send.
     *
     * @param reliable          Select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced         Select the type of flow: sequenced (true) or unsequenced (false).
     * @param src               The buffer from wich bytes are to be retrieved
     * @param tag               Used to identify the type of the packet.
     * @param priority          Indicates the priority of the packet. The range of priority values is 0-255.
     *                          Default value = 5.
     * @param enqueueTimeout    Indicates the length of time in milliseconds for which the method will wait if
     *                          there is no room in the outgoing buffer (a zero value indicates wait forever).
     *                          Default value = 0.
     * @param retryTimeout      Indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery (a zero value indicates retry with no time limit).
     *                          Default value = 0.
     *
     * @return              The number of bytes written, possibly zero
     */
    public int send (boolean reliable, boolean sequenced, ByteBuffer src, int tag, short priority,
            long enqueueTimeout,long retryTimeout)
    {
        // TODO: evaulate non-blocking mode
        byte[] buffer = new byte[src.remaining()];
        src.get(buffer);
        //System.out.println ("MocketChannel write: sending buffer: "+new String (buffer));
        // Using default parameters for the send call and using reliable sequenced service
        int res = 0;
        try {
            res = _mocket.send (reliable, sequenced, buffer, tag, priority, enqueueTimeout, retryTimeout);
        }
        catch (IOException ex) {
            System.out.println ("MocketChannel write: IOException during send call");
            return -10;
        }
        return res;
    }

    /*
     * Writes a sequence of bytes to this channel from the given buffer.
     * <p>
     * An attempt is made to write up to r bytes to the channel, where r is the number of bytes remaining in the buffer,
     * that is, dst.remaining(), at the moment this method is invoked.
     * <p>
     * Suppose that a byte sequence of length n is written, where 0 <= n <= r. This byte sequence will be transferred from
     * the buffer starting at index p, where p is the buffer's position at the moment this method is invoked; the index of
     * the last byte written will be p + n - 1. Upon return the buffer's position will be equal to p + n; its limit will not have changed.
     * <p>
     * Unless otherwise specified, a write operation will return only after writing all of the r requested bytes. Some types of channels,
     * depending upon their state, may write only some of the bytes or possibly none at all. A socket channel in non-blocking mode,
     * for example, cannot write any more bytes than are free in the socket's output buffer.
     * <p>
     * This method may be invoked at any time. If another thread has already initiated a write operation upon this channel, however,
     * then an invocation of this method will block until the first operation is complete.
     * <p>
     * This write correspond to <code>int write (ByteBuffer src)</code> of the SocketChannel class.
     *
     * @param src   The buffer from wich bytes are to be retrieved
     *
     * @return      The number of bytes written, possibly zero
     */
    public int write (ByteBuffer src)
    {
        // TODO: evaulate non-blocking mode
        byte[] buffer = new byte[src.remaining()];
        src.get(buffer);
        //System.out.println ("MocketChannel write: sending buffer: "+new String (buffer));
        // Using default parameters for the send call and using reliable sequenced service
        int defaultTag = 0;
        short defaultPriority = 5;
        long defaultEnqueueTimeout = 0;
        long defaultRetryTimeout = 0;
        int res = 0;
        try {
            res = _mocket.send (true, true, buffer, defaultTag, defaultPriority, defaultEnqueueTimeout, defaultRetryTimeout);
        }
        catch (IOException ex) {
            System.out.println ("MocketChannel write: IOException during send call");
            return -10;
        }
        return res;
    }

    /*
     * Writes a sequence of bytes to this channel from a subsequence of the given buffers.
     * <p>
     * An attempt is made to write up to r bytes to this channel, where r is the total number
     * of bytes remaining in the specified subsequence of the given buffer array.
     * <p>
     * Suppose that a byte sequence of length n is written, where 0 <= n <= r. Up to the first srcs[offset].remaining() bytes
     * of this sequence are written from buffer srcs[offset], up to the next srcs[offset+1].remaining() bytes are written from
     * buffer srcs[offset+1], and so forth, until the entire byte sequence is written. As many bytes as possible are written from
     * each buffer, hence the final position of each updated buffer, except the last updated buffer, is guaranteed to be equal to
     * that buffer's limit.
     * <p>
     * Unless otherwise specified, a write operation will return only after writing all of the r requested bytes. Some types of
     * channels, depending upon their state, may write only some of the bytes or possibly none at all. A socket channel in non-blocking
     * mode, for example, cannot write any more bytes than are free in the socket's output buffer.
     * <p>
     * This method may be invoked at any time. If another thread has already initiated a write operation upon this channel, however,
     * then an invocation of this method will block until the first operation is complete.
     * <p>
     * This write correspond to <code>long write (ByteBuffer[] srcs, int offset, int length)</code> of the SocketChannel class.
     *
     * @param srcs      The buffers from which bytes are to be retrieved
     * @param offset    The offset within the buffer array of the first buffer from which bytes are to be retrieved; must be non-negative and no larger than <code>srcs.length</code>
     * @param length    The maximum number of buffers to be accessed; must be non-negative and no larger than srcs.length - offset
     *
     * @return          The number of bytes written, possibly zero
     */
    public long write (ByteBuffer[] srcs, int offset, int length)
    {
        if ((offset < 0) || (offset > srcs.length)) {
            throw new IllegalArgumentException ("MocketChannel read: IllegalArgumentException offset parameter out of bound.");
        }
        if ((length < 0) || (length > srcs.length - offset)) {
            throw new IllegalArgumentException ("MocketChannel read: IllegalArgumentException length parameter out of bound.");
        }
        // Strategy: if we call send for the data in each ByteBuffer, we may have a lot of overhead (header) for a very little amount of data
        // whereas if we read from the ByteBuffers and then call a single send we may have a big packet that will get fragmented.
        // A good trade off could be to move to a byte array and send enough data to make 1 MTU
        int srcsCount = 0;
        for (int i=0; i<length-offset; i++) {
            srcsCount += srcs[i].remaining();
        }
        // Create a byte array big enough to hold the bytes from the array of ByteBuffers
        byte[] buffer = new byte[srcsCount];
        // Copy the information over into the byte array
        int bufferOffset = 0;
        int temp;
        for (int i=offset; i<offset+length; i++) {
            temp = srcs[i].remaining();
            srcs[i].get(buffer, bufferOffset, bufferOffset+temp);
            bufferOffset += temp;
        }
        // Using default parameters for the send call
        long res = 0;
        int defaultTag = 0;
        short defaultPriority = 5;
        long defaultEnqueueTimeout = 0;
        long defaultRetryTimeout = 0;
        try {
            res = _mocket.send (true, true, buffer, defaultTag, defaultPriority, defaultEnqueueTimeout, defaultRetryTimeout);
        }
        catch (IOException ex) {
            System.out.println ("MocketChannel write: IOException during send call");
            return -10;
        }
        return res;
    }

    /*
     * Writes a sequence of bytes to this channel from the given buffers.
     * <p>
     * An invocation of this method of the form c.write(srcs)  behaves in exactly the same manner as the invocation
     * <code>c.write(srcs, 0, srcs.length);</code>
     * <p>
     * This write correspond to <code>long write (ByteBuffer[] srcs)</code> of the SocketChannel class.
     *
     * @param srcs  The buffers from which bytes are to be retrieved
     *
     * @return      The number of bytes written, possibly zero
     */
    public long write (ByteBuffer[] srcs)
    {
        return write (srcs, 0, srcs.length);
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
    protected void implCloseSelectableChannel()
            throws IOException
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
    protected void implConfigureBlocking (boolean block)
            throws IOException
    {
        _blockingMode = block;
    }

    private Mocket _mocket;
    private boolean _blockingMode;
    boolean _isConnected;
    boolean _isConnectionPending;
}
