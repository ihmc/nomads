/*
 * MocketsConnection.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import us.ihmc.kryomockets.util.AddressUtils;
import org.apache.log4j.Logger;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.ServerMocket;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.nio.ByteBuffer;

/**
 * MocketsConnection.java
 * <p/>
 * Class <code>MocketsConnection</code> provide a set of methods to interact with <code>Mocket</code> and
 * <code>ServerMocket</code> instances.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MocketsConnection
{
    InetSocketAddress connectedAddress;
    private ServerMocket serverMocket;
    private final Mocket mocket;
    int keepAliveMillis = 19000;
    private final ByteBuffer readBuffer, writeBuffer;
    private final Serialization serialization;
    private final Object writeLock = new Object();
    private long lastCommunicationTime;
    private final static Logger LOG = Logger.getLogger(MocketsConnection.class);

    //native mockets library
    static {
        System.loadLibrary("mocketsjavawrapper");
    }

    public MocketsConnection (Serialization serialization, int writeBufferSize, int readBufferSize)
        throws IOException
    {
        //used by a Client
        mocket = new Mocket();
        this.serialization = serialization;
        readBuffer = ByteBuffer.allocate(readBufferSize);
        writeBuffer = ByteBuffer.allocateDirect(writeBufferSize);
    }

    public MocketsConnection (Mocket mocket, Serialization serialization, int writeBufferSize, int readBufferSize)
    {
        //used by a Server
        this.mocket = mocket;
        this.serialization = serialization;
        readBuffer = ByteBuffer.allocate(readBufferSize);
        writeBuffer = ByteBuffer.allocateDirect(writeBufferSize);
    }

    public void bind (InetSocketAddress localPort) throws IOException
    {
        close();
        readBuffer.clear();
        writeBuffer.clear();
        try {
            LOG.debug("Creating ServerMocket on " + localPort.getAddress().getHostAddress() + ":" + localPort.getPort
                    ());
            serverMocket = new ServerMocket(localPort.getPort(), localPort.getAddress().getHostAddress());
            LOG.debug("ServerMocket created successfully. Waiting to accept connections.");
            lastCommunicationTime = System.currentTimeMillis();
            serverMocket.bind(localPort.getPort());
            LOG.debug("ServerMocket bind on: " + localPort.getPort());
        }
        catch (IOException ex) {
            close();
            throw ex;
        }
    }

    public Mocket accept () throws IOException
    {
        Mocket mocket;
        try {
            LOG.trace("Waiting to accept a Mockets connection");
            mocket = serverMocket.accept();
            LOG.trace("Accepted Mockets connection");
        }
        catch (IOException ex) {
            close();
            throw ex;
        }

        return mocket;
    }

    public void connect (InetSocketAddress remoteAddress) throws IOException
    {
        close();
        readBuffer.clear();
        writeBuffer.clear();
        try {
            LOG.debug("Attempting connection to " + remoteAddress.getAddress().getHostAddress() + ":" + remoteAddress
                    .getPort());
            mocket.connect(remoteAddress);
            lastCommunicationTime = System.currentTimeMillis();
            connectedAddress = remoteAddress;
        }
        catch (IOException ex) {
            close();
            IOException ioEx = new IOException("Unable to connect to: " + remoteAddress);
            ioEx.initCause(ex);
            throw ioEx;
        }
    }

    public InetSocketAddress readFromAddress () throws IOException
    {
        Mocket mocket = this.mocket;
        if (mocket == null) throw new SocketException("Connection is closed.");
        lastCommunicationTime = System.currentTimeMillis();


//TODO Fix non-blocking getNextMessageSize in Mockets
//        int dataSize = mocket.getNextMessageSize(-1);    //blocking
//        LOG.trace("Incoming message size: " + dataSize);
//        byte[] b = new byte[dataSize];

        byte[] b = new byte[readBuffer.capacity()];
        int dataSize = mocket.receive(b);
        if (dataSize > readBuffer.capacity()) {
            LOG.error("Received message exceeds readBuffer capacity.");
            throw new IOException();
        }

        LOG.trace("Received bytes: " + dataSize);//blocking
        for (int i = 0; i < dataSize; i++) {
            readBuffer.put(b[i]);
        }

//        readBuffer.put(b);
        LOG.trace("Capacity of readBuffer: " + readBuffer.capacity());
        LOG.trace("Remaining bytes on readBuffer: " + readBuffer.remaining());
        InetAddress address = AddressUtils.intToInetAddress((int) mocket.getRemoteAddress());
        int port = mocket.getRemotePort();
        LOG.debug("Received data from " + address.toString() + ":" + port);
        return new InetSocketAddress(address, port);
    }

    public Object readObject (Connection connection)
    {
        readBuffer.flip();
        try {
            try {
                Object object = serialization.read(connection, readBuffer);
                if (readBuffer.hasRemaining())
                    throw new KryoNetException("Incorrect number of bytes (" + readBuffer.remaining()
                            + " remaining) used to deserialize object: " + object);
                return object;
            }
            catch (Exception ex) {
                throw new KryoNetException("Error during deserialization.", ex);
            }
        }
        finally {
            readBuffer.clear();
        }
    }

    /**
     * This method is thread safe.
     */
    public int send (Connection connection, Object object, SocketAddress address) throws IOException
    {
        Mocket mocket = this.mocket;
        if (mocket == null) throw new SocketException("Connection is closed.");
        Mocket.Sender sender = mocket.getSender(true, true);
        synchronized (writeLock) {
            try {
                try {
                    serialization.write(connection, writeBuffer, object);
                }
                catch (Exception ex) {
                    throw new KryoNetException("Error serializing object of type: " + object.getClass().getName(), ex);
                }
                writeBuffer.flip();
                int length = writeBuffer.limit();
                LOG.trace("Current writeBuffer limit is: " + length);
                byte[] b = new byte[writeBuffer.remaining()];
                writeBuffer.get(b);

                if (sender.send(b, 0, b.length) == 0)
                    LOG.debug("Successfully sent " + b.length + " bytes");
                else
                    LOG.error("Error sending the object.");

                lastCommunicationTime = System.currentTimeMillis();
                boolean wasFullWrite = !writeBuffer.hasRemaining();
                if (!wasFullWrite)
                    LOG.debug("writeBuffer has remaining bytes: " + writeBuffer.hasRemaining());
                return wasFullWrite ? length : -1;
            }
            finally {
                writeBuffer.clear();
            }
        }
    }

    public void close ()
    {
        connectedAddress = null;
        try {
            if (mocket != null) {
                mocket.close();
                //mocket = null;
            }
        }
        catch (IOException ex) {
            LOG.debug("Unable to close Mockets connection.", ex);
        }
    }

    public boolean needsKeepAlive (long time)
    {
        return connectedAddress != null && keepAliveMillis > 0 && time - lastCommunicationTime > keepAliveMillis;
    }

}
