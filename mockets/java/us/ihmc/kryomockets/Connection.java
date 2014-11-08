/*
 * Connection.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;

import com.esotericsoftware.kryo.Kryo;
import us.ihmc.kryomockets.FrameworkMessage.Ping;
import org.apache.log4j.Logger;
import us.ihmc.mockets.Mocket;


/**
 * Class <code>Connection</code> represents a Mockets connection between a {@link Client} and a {@link Server}.
 * If either underlying connection is closed or errors, both connections are closed.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Connection implements Runnable
{
    int id = -1;
    private String name;
    EndPoint endPoint;
    MocketsConnection mockets;
    InetSocketAddress mocketsRemoteAddress;
    private Listener[] listeners = {};
    private final Object listenerLock = new Object();
    private int lastPingID;
    private long lastPingSendTime;
    private int returnTripTime;
    volatile boolean isConnected;
    private Thread updateThread;
    protected final static int DEFAULT_UPDATE_CYCLE = 0;
    private final static Logger LOG = Logger.getLogger(Connection.class);

    protected Connection ()
    {
    }

    void initialize (Serialization serialization, int writeBufferSize, int objectBufferSize) throws IOException
    {
        mockets = new MocketsConnection(serialization, writeBufferSize, objectBufferSize);
    }

    void initialize (Mocket mocket, Serialization serialization, int writeBufferSize, int objectBufferSize)
    {
        mockets = new MocketsConnection(mocket, serialization, writeBufferSize, objectBufferSize);
    }

    @Override
    public void run ()
    {
        LOG.trace("Connection thread started.");
        while (isConnected()) {
            try {
                update(DEFAULT_UPDATE_CYCLE);
            }
            catch (IOException ex) {
                LOG.trace("Error updating connection.", ex);
                close();
            }
        }
        LOG.trace("Connection thread stopped.");
    }

    private void update (int timeout) throws IOException
    {
        updateThread = Thread.currentThread();

        InetSocketAddress fromAddress;
        try {
            fromAddress = mockets.readFromAddress();
        }
        catch (IOException ex) {
            LOG.warn("Error reading Mockets address.", ex);
            throw ex;
        }

        if (fromAddress == null) return;
        else mocketsRemoteAddress = fromAddress;

        Object object;
        try {
            object = mockets.readObject(this);
        }
        catch (KryoNetException ex) {
            String msgErr = "Error reading Mockets data from connection: ";
            LOG.error(msgErr + this, ex);
            throw new IOException(msgErr);
        }

        if (object instanceof FrameworkMessage) {
            if (object instanceof FrameworkMessage.RegisterMockets) {
                LOG.trace("Received RegisterMockets, replying with same");
                sendMockets(new FrameworkMessage.RegisterMockets());
                LOG.debug("Server child connected to: " + fromAddress);
                notifyConnected();
                return;
            }
        }

        String objectString = object == null ? "null" : object.getClass().getSimpleName();
        LOG.trace(this + " received Mockets object: " + objectString);
        notifyReceived(object);

        try {
            Thread.sleep(timeout);
        }
        catch (InterruptedException e) {
            LOG.error(updateThread + " interrupted", e);
        }
    }

    /**
     * Returns the server assigned ID. Will return -1 if this connection has never been connected or the last
     * assigned ID if this
     * connection has been disconnected.
     */
    public int getID ()
    {
        return id;
    }

    /**
     * Returns true if this connection is connected to the remote end. Note that a connection can become disconnected
     * at any time.
     */
    public boolean isConnected ()
    {
        return isConnected;
    }

    /**
     * Sends the object over the network using Mockets.
     *
     * @return The number of bytes sent.
     * @throws IllegalStateException if this connection was not opened with both TCP and UDP.
     * @see Kryo#register(Class, com.esotericsoftware.kryo.Serializer)
     */
    public int sendMockets (Object object)
    {
        if (object == null) throw new IllegalArgumentException("object cannot be null.");
        SocketAddress address = mocketsRemoteAddress;
        if (address == null && mockets != null) {
            LOG.trace("Remote address is: " + mockets.connectedAddress);
            address = mockets.connectedAddress;
        }
        if (address == null && isConnected) throw new IllegalStateException("Connection is not connected via Mockets.");

        try {
            if (address == null) throw new SocketException("Connection is closed.");

            int length = mockets.send(this, object, address);
            if (length == 0) {
                LOG.trace("Mockets had nothing to send.");
                return 0;
            }
            if (length != -1) {
                String objectString = object == null ? "null" : object.getClass().getSimpleName();
                if (!(object instanceof FrameworkMessage)) {
                    LOG.debug("Sent Mockets: " + objectString + " (" + length + ")");
                }
            }
            else
                LOG.debug(this + " was unable to send, Mockets socket buffer full.");
            return length;
        }
        catch (IOException ex) {
            LOG.debug("Unable to send Mockets with connection: " + this, ex);
            close();
            return 0;
        }
        catch (KryoNetException ex) {
            LOG.debug("Unable to send Mockets with connection: " + this, ex);
            close();
            return 0;
        }
    }

    public void close ()
    {
        boolean wasConnected = isConnected;
        isConnected = false;
        if (mockets != null && mockets.connectedAddress != null) mockets.close();
        if (wasConnected) {
            notifyDisconnected();
            LOG.info(this + " disconnected.");
        }
        setConnected(false);
    }

    /**
     * Requests the connection to communicate with the remote computer to determine a new value for the
     * {@link #getReturnTripTime() return trip time}. When the connection receives a {@link FrameworkMessage.Ping}
     * object with
     * {@link Ping#isReply isReply} set to true, the new return trip time is available.
     */
    public void updateReturnTripTime ()
    {
        Ping ping = new Ping();
        ping.id = lastPingID++;
        lastPingSendTime = System.currentTimeMillis();
        sendMockets(ping);
    }

    /**
     * Returns the last calculated TCP return trip time, or -1 if {@link #updateReturnTripTime()} has never been
     * called or the
     * {@link FrameworkMessage.Ping} response has not yet been received.
     */
    public int getReturnTripTime ()
    {
        return returnTripTime;
    }

    /**
     * If the listener already exists, it is not added again.
     */
    public void addListener (Listener listener)
    {
        if (listener == null) throw new IllegalArgumentException("listener cannot be null.");
        synchronized (listenerLock) {
            Listener[] listeners = this.listeners;
            int n = listeners.length;
            for (int i = 0; i < n; i++) {
                if (listener == listeners[i]) return;
            }
            Listener[] newListeners = new Listener[n + 1];
            newListeners[0] = listener;
            System.arraycopy(listeners, 0, newListeners, 1, n);
            this.listeners = newListeners;
        }
        LOG.trace("Connection listener added: " + listener.getClass().getName());
    }

    public void removeListener (Listener listener)
    {
        if (listener == null) throw new IllegalArgumentException("listener cannot be null.");
        synchronized (listenerLock) {
            Listener[] listeners = this.listeners;
            int n = listeners.length;
            if (n == 0) return;
            Listener[] newListeners = new Listener[n - 1];
            for (int i = 0, ii = 0; i < n; i++) {
                Listener copyListener = listeners[i];
                if (listener == copyListener) continue;
                if (ii == n - 1) return;
                newListeners[ii++] = copyListener;
            }
            this.listeners = newListeners;
        }
        LOG.trace("Connection listener removed: " + listener.getClass().getName());
    }

    void notifyConnected ()
    {
        Listener[] listeners = this.listeners;
        for (int i = 0, n = listeners.length; i < n; i++) {
            listeners[i].connected(this);
        }
    }

    void notifyDisconnected ()
    {
        Listener[] listeners = this.listeners;
        for (int i = 0, n = listeners.length; i < n; i++) {
            listeners[i].disconnected(this);
        }
    }

    void notifyIdle ()
    {
        Listener[] listeners = this.listeners;
        for (int i = 0, n = listeners.length; i < n; i++) {
            listeners[i].idle(this);
        }
    }

    void notifyReceived (Object object)
    {
        if (object instanceof Ping) {
            Ping ping = (Ping) object;
            if (ping.isReply) {
                if (ping.id == lastPingID - 1) {
                    returnTripTime = (int) (System.currentTimeMillis() - lastPingSendTime);
                    LOG.trace(this + " return trip time: " + returnTripTime);
                }
            }
            else {
                ping.isReply = true;
                sendMockets(ping);
            }
        }
        Listener[] listeners = this.listeners;
        for (int i = 0, n = listeners.length; i < n; i++) {
            listeners[i].received(this, object);
        }
    }

    /**
     * Returns the local {@link Client} or {@link Server} to which this connection belongs.
     */
    public EndPoint getEndPoint ()
    {
        return endPoint;
    }

    /**
     * Returns the IP address and port of the remote end of the Mockets connection, or null if this connection is not
     * connected.
     */
    public InetSocketAddress getRemoteAddressMockets ()
    {
        InetSocketAddress connectedAddress = mockets.connectedAddress;
        if (connectedAddress != null) return connectedAddress;
        return mocketsRemoteAddress;
    }


    /**
     * Sets the friendly name of this connection. This is returned by {@link #toString()} and is useful for providing
     * application
     * specific identifying information in the logging. May be null for the default name of "Connection X",
     * where X is the
     * connection ID.
     */
    public void setName (String name)
    {
        this.name = name;
    }

    public String toString ()
    {
        if (name != null) return name;
        return "Connection " + id;
    }

    void setConnected (boolean isConnected)
    {
        this.isConnected = isConnected;
        if (isConnected && name == null) name = "Connection " + id;
    }
}
