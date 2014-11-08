/*
 * Client.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.security.AccessControlException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import com.esotericsoftware.kryo.Kryo;
import us.ihmc.kryomockets.FrameworkMessage.DiscoverHost;
import org.apache.log4j.Logger;

import static com.esotericsoftware.minlog.Log.*;

/**
 * KryoMockets is a Java library that provides a clean and simple API for efficient Mockets client/server
 * network communication.
 * KryoMockets uses the Kryo serialization library to automatically and efficiently transfer object graphs across the
 * network.
 * KryoMockets is inspired by KryoNet (http://code.google.com/p/kryonet/)
 * <p/>
 * Class <code>Client</code> represents a Mockets connection to a {@link Server}.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Client extends Connection implements EndPoint
{
    static {
        try {
            // Needed for NIO selectors on Android 2.2.
            System.setProperty("java.net.preferIPv6Addresses", "false");
        }
        catch (AccessControlException ignored) {
        }
    }

    private final static int DEFAULT_WRITE_BUFFER_SIZE = 8192;
    private final static int DEFAULT_OBJECT_BUFFER_SIZE = 2048;
    private final Serialization serialization;
    private volatile boolean mocketsRegistered;
    private final Object mocketsRegistrationLock = new Object();
    private volatile boolean connectionAttempted = false;
    private volatile boolean shutdown;
    private final Object updateLock = new Object();
    private Thread updateThread;
    private int connectTimeout;
    private InetAddress connectHost;
    private int connectMocketsPort;
    private final static org.apache.log4j.Logger LOG = Logger.getLogger(Client.class);

    /**
     * Creates a Client with a write buffer size of 8192 and an object buffer size of 2048.
     */
    public Client () throws IOException
    {
        this(DEFAULT_WRITE_BUFFER_SIZE, DEFAULT_OBJECT_BUFFER_SIZE);
    }

    /**
     * @param writeBufferSize  One buffer of this size is allocated. Objects are serialized to the write buffer where
     *                         the bytes are queued until they can be written to the Mocket.
     *                         <p/>
     *                         Normally the Mocket is writable and the bytes are written immediately. If the Mocket
     *                         cannot be written to and enough serialized objects are queued to overflow the buffer,
     *                         then the connection will be closed.
     *                         <p/>
     *                         The write buffer should be sized at least as large as the largest object that will be
     *                         sent, plus some head room to allow for some serialized objects to be queued in case the
     *                         buffer is temporarily not writable. The amount of head room needed is dependent upon the
     *                         size of objects being sent and how often they are sent.
     * @param objectBufferSize One buffer of this size is allocated. This buffer is used to hold the bytes for a single
     *                         object graph until it can be sent over the network or deserialized.
     *                         <p/>
     *                         The object buffers should be sized at least as large as the largest object that will
     *                         be sent or received.
     */
    public Client (int writeBufferSize, int objectBufferSize) throws IOException
    {
        this(writeBufferSize, objectBufferSize, new KryoSerialization());
    }

    public Client (int writeBufferSize, int objectBufferSize, Serialization serialization) throws IOException
    {
        super();
        endPoint = this;
        this.serialization = serialization;
        initialize(serialization, writeBufferSize, objectBufferSize);
    }

    public Serialization getSerialization ()
    {
        return serialization;
    }

    public Kryo getKryo ()
    {
        return ((KryoSerialization) serialization).getKryo();
    }

    /**
     * Opens a Mockets client.
     *
     * @see #connect(int, InetAddress, int)
     */
    public void connect (int timeout, String host, int mocketsPort) throws IOException
    {
        connect(timeout, InetAddress.getByName(host), mocketsPort);
    }

    /**
     * Opens a Mockets client. Blocks until the connection is complete or the timeout is reached.
     * <p/>
     * Because the framework must perform some minimal communication before the connection is considered successful,
     * {@link #update(int)} must be called on a separate thread during the connection process.
     *
     * @throws IllegalStateException if called from the connection's update thread.
     * @throws IOException           if the client could not be opened or connecting times out.
     */
    public void connect (int timeout, InetAddress host, int mocketsPort) throws IOException
    {
        if (host == null) throw new IllegalArgumentException("host cannot be null.");
        if (Thread.currentThread() == getUpdateThread())
            throw new IllegalStateException("Cannot connect on the connection's update thread.");
        this.connectTimeout = timeout;
        this.connectHost = host;
        this.connectMocketsPort = mocketsPort;
        close();
        LOG.info("Connecting: " + host + ":" + mocketsPort);
        id = -1;
        try {
            //removed new MocketConnection, moved to initialize(serialization, writeBufferSize, objectBufferSize)
            long endTime;
            LOG.trace("Trying Mockets registration..");
            InetSocketAddress mocketsAddress = new InetSocketAddress(host, mocketsPort);
            synchronized (updateLock) {
                mocketsRegistered = false;
                endTime = System.currentTimeMillis() + timeout;
                mockets.connect(mocketsAddress);
                connectionAttempted = true;
            }

            // Wait for RegisterMockets reply.
            synchronized (mocketsRegistrationLock) {
                while (!mocketsRegistered && System.currentTimeMillis() < endTime) {
                    FrameworkMessage.RegisterMockets registerMockets = new FrameworkMessage.RegisterMockets();
                    registerMockets.connectionID = id;
                    LOG.trace("About to send RegisterMockets...");
                    mockets.send(this, registerMockets, mocketsAddress);
                    try {
                        mocketsRegistrationLock.wait(100);
                    }
                    catch (InterruptedException ignored) {
                    }
                }
                if (!mocketsRegistered)
                    throw new SocketTimeoutException("Connected, but timed out during Mockets registration: " +
                            host + ":" + mocketsPort);
            }
        }
        catch (IOException ex) {
            close();
            throw ex;
        }
    }

    /**
     * Calls {@link #connect(int, InetAddress, int) connect} with the values last passed to connect.
     *
     * @throws IllegalStateException if connect has never been called.
     */
    public void reconnect () throws IOException
    {
        reconnect(connectTimeout);
    }

    /**
     * Calls {@link #connect(int, InetAddress, int) connect} with the specified timeout and the other values last
     * passed to
     * connect.
     *
     * @throws IllegalStateException if connect has never been called.
     */
    public void reconnect (int timeout) throws IOException
    {
        if (connectHost == null) throw new IllegalStateException("This client has never been connected.");
        connect(connectTimeout, connectHost, connectMocketsPort);
    }

    /**
     * Reads or writes any pending data for this client. Multiple threads should not call this method at the same time.
     *
     * @param timeout Wait for up to the specified milliseconds for data to be ready to process. May be zero to
     *                return immediately
     *                if there is no data to process.
     */
    public void update (int timeout) throws IOException
    {
        updateThread = Thread.currentThread();

        synchronized (updateLock) {

            if (!connectionAttempted) return; //return if main Thread hasn't done connect yet

            if (mockets.readFromAddress() == null) return;
            Object object = mockets.readObject(this);
            LOG.trace("Received object via Mockets");
            if (object == null) {
                LOG.trace("Object was null");
                return;
            }
            LOG.trace(this + " received Mockets: " + object.getClass().getSimpleName());

            if (mockets != null && !mocketsRegistered) {
                LOG.trace("Mockets not registered yet, trying to register");
                if (object instanceof FrameworkMessage.RegisterMockets) {
                    LOG.trace("Found message of type RegisterMockets. Waiting to obtain " +
                            "mocketsRegistrationLock");
                    synchronized (mocketsRegistrationLock) {
                        mocketsRegistered = true;
                        mocketsRegistrationLock.notifyAll();
                        LOG.trace(this + " received Mockets object: RegisterMockets");
                        LOG.debug("Mockets connected to: " + mockets.connectedAddress);
                        setConnected(true);
                    }
                    notifyConnected();
                }
                return;
            }
            if (!isConnected) return;
            if (DEBUG) {
                if (!(object instanceof FrameworkMessage)) {
                    LOG.debug(this + " received Mockets: " + object.getClass().getSimpleName());
                }
                else if (TRACE) {
                    LOG.trace(this + " received Mockets: " + object.getClass().getSimpleName());
                }
            }
            notifyReceived(object);
        }
//        if (isConnected) {
//            long time = System.currentTimeMillis();
//            //check if keeping alive the connection
//            if (mocketsRegistered && mockets.needsKeepAlive(time))
//                sendMockets(FrameworkMessage.keepAlive);
//        }

        try {
            Thread.sleep(timeout);
        }
        catch (InterruptedException e) {
            LOG.error(updateThread + " interrupted", e);
        }
    }

    public void run ()
    {
        LOG.trace("Client thread started.");
        shutdown = false;

        while (!shutdown) {
            try {
                update(DEFAULT_UPDATE_CYCLE);
            }
            catch (IOException ex) {
                if (isConnected)
                    LOG.trace("Unable to update connection: " + this, ex);
                else
                    LOG.trace("Unable to update connection.", ex);
                close();
            }
            catch (KryoNetException ex) {
                if (isConnected)
                    LOG.error("Error updating connection: " + this, ex);
                else
                    LOG.error("Error updating connection.", ex);
                close();
            }
        }
        LOG.trace("Client thread stopped.");
    }

    public void start ()
    {
        // Try to let any previous update thread stop.
        if (updateThread != null) {
            shutdown = true;
            try {
                updateThread.join(5000);
            }
            catch (InterruptedException ignored) {
            }
        }
        updateThread = new Thread(this, "Client");
        updateThread.setDaemon(true);
        updateThread.start();
    }

    public void stop ()
    {
        if (shutdown) return;
        close();
        if (TRACE) LOG.trace("Client thread stopping.");
        shutdown = true;
    }

    public void close ()
    {
        super.close();
    }

    public void addListener (Listener listener)
    {
        super.addListener(listener);
        if (TRACE) LOG.trace("Client listener added.");
    }

    public void removeListener (Listener listener)
    {
        super.removeListener(listener);
        if (TRACE) LOG.trace("Client listener removed.");
    }

    /**
     * An empty object will be sent if the Mockets connection is inactive more than the specified milliseconds. Network
     * hardware may keep a translation table of inside to outside IP addresses and a Mockets keep alive keeps this
     * table entry from expiring. Set to zero to disable. Defaults to 19000.
     */
    public void setKeepAliveMockets (int keepAliveMillis)
    {
        if (mockets == null) throw new IllegalStateException("Not connected via Mockets.");
        mockets.keepAliveMillis = keepAliveMillis;
    }

    public Thread getUpdateThread ()
    {
        return updateThread;
    }

    private void broadcast (int udpPort, DatagramSocket socket) throws IOException
    {
        ByteBuffer dataBuffer = ByteBuffer.allocate(64);
        serialization.write(null, dataBuffer, new DiscoverHost());
        dataBuffer.flip();
        byte[] data = new byte[dataBuffer.limit()];
        dataBuffer.get(data);
        for (NetworkInterface iface : Collections.list(NetworkInterface.getNetworkInterfaces())) {
            for (InetAddress address : Collections.list(iface.getInetAddresses())) {
                // Java 1.5 doesn't support getting the subnet mask, so try the two most common.
                byte[] ip = address.getAddress();
                ip[3] = -1; // 255.255.255.0
                socket.send(new DatagramPacket(data, data.length, InetAddress.getByAddress(ip), udpPort));
                ip[2] = -1; // 255.255.0.0
                socket.send(new DatagramPacket(data, data.length, InetAddress.getByAddress(ip), udpPort));
            }
        }
        LOG.debug("Broadcasted host discovery on port: " + udpPort);
    }

    /**
     * Broadcasts a UDP message on the LAN to discover any running servers. The address of the first server to
     * respond is returned.
     *
     * @param udpPort       The UDP port of the server.
     * @param timeoutMillis The number of milliseconds to wait for a response.
     * @return the first server found, or null if no server responded.
     */
    public InetAddress discoverHost (int udpPort, int timeoutMillis)
    {
        DatagramSocket socket = null;
        try {
            socket = new DatagramSocket();
            broadcast(udpPort, socket);
            socket.setSoTimeout(timeoutMillis);
            DatagramPacket packet = new DatagramPacket(new byte[0], 0);
            try {
                socket.receive(packet);
            }
            catch (SocketTimeoutException ex) {
                if (INFO) LOG.info("Host discovery timed out.");
                return null;
            }
            if (INFO) LOG.info("Discovered server: " + packet.getAddress());
            return packet.getAddress();
        }
        catch (IOException ex) {
            if (ERROR) LOG.error("Host discovery failed.", ex);
            return null;
        }
        finally {
            if (socket != null) socket.close();
        }
    }

    /**
     * Broadcasts a UDP message on the LAN to discover any running servers.
     *
     * @param udpPort       The UDP port of the server.
     * @param timeoutMillis The number of milliseconds to wait for a response.
     */
    public List<InetAddress> discoverHosts (int udpPort, int timeoutMillis)
    {
        List<InetAddress> hosts = new ArrayList<InetAddress>();
        DatagramSocket socket = null;
        try {
            socket = new DatagramSocket();
            broadcast(udpPort, socket);
            socket.setSoTimeout(timeoutMillis);
            while (true) {
                DatagramPacket packet = new DatagramPacket(new byte[0], 0);
                try {
                    socket.receive(packet);
                }
                catch (SocketTimeoutException ex) {
                    if (INFO) LOG.info("Host discovery timed out.");
                    return hosts;
                }
                if (INFO) LOG.info("Discovered server: " + packet.getAddress());
                hosts.add(packet.getAddress());
            }
        }
        catch (IOException ex) {
            if (ERROR) LOG.error("Host discovery failed.", ex);
            return hosts;
        }
        finally {
            if (socket != null) socket.close();
        }
    }
}
