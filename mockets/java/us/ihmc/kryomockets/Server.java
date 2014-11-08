/*
 * Server.java
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
import java.util.ArrayList;
import java.util.List;

import com.esotericsoftware.kryo.Kryo;
import org.apache.log4j.Logger;
import us.ihmc.mockets.Mocket;

import static com.esotericsoftware.minlog.Log.*;

/**
 * KryoMockets is a Java library that provides a clean and simple API for efficient Mockets client/server
 * network communication.
 * KryoMockets uses the Kryo serialization library to automatically and efficiently transfer object graphs across the
 * network.
 * KryoMockets is inspired by KryoNet (http://code.google.com/p/kryonet/)
 * <p/>
 * <p/>
 * Class <code>Server</code>Manages Mockets connections from many {@link Client Clients}.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Server implements EndPoint
{
    private final static int DEFAULT_WRITE_BUFFER_SIZE = 16384;
    private final static int DEFAULT_OBJECT_BUFFER_SIZE = 2048;
    private final static int DEFAULT_CONNECTIONS_UPDATE_CYCLE = 50;
    private final Serialization serialization;
    private final int writeBufferSize, objectBufferSize;
    private MocketsConnection mockets;
    private final List<Connection> connections = new ArrayList<Connection>();
    Listener[] listeners = {};
    private final Object listenerLock = new Object();
    private final Object updateLock = new Object();
    private int nextConnectionID = 1;
    private volatile boolean shutdown;

    private Thread updateThread;
    private final static Logger LOG = Logger.getLogger(Server.class);

    private Listener dispatchListener = new Listener()
    {
        public void connected (Connection connection)
        {
            Listener[] listeners = Server.this.listeners;
            for (int i = 0, n = listeners.length; i < n; i++) {
                listeners[i].connected(connection);
            }
        }

        public void disconnected (Connection connection)
        {
            removeConnection(connection);
            Listener[] listeners = Server.this.listeners;
            for (int i = 0, n = listeners.length; i < n; i++) {
                listeners[i].disconnected(connection);
            }
        }

        public void received (Connection connection, Object object)
        {
            Listener[] listeners = Server.this.listeners;
            for (int i = 0, n = listeners.length; i < n; i++) {
                listeners[i].received(connection, object);
            }
        }

        public void idle (Connection connection)
        {
            Listener[] listeners = Server.this.listeners;
            for (int i = 0, n = listeners.length; i < n; i++) {
                listeners[i].idle(connection);
            }
        }
    };

    /**
     * Creates a Server with a write buffer size of 16384 and an object buffer size of 2048.
     */
    public Server ()
    {
        this(DEFAULT_WRITE_BUFFER_SIZE, DEFAULT_OBJECT_BUFFER_SIZE);
    }

    /**
     * @param writeBufferSize  One buffer of this size is allocated for each connected client. Objects are serialized
     *                         to the write buffer where the bytes are queued until they can be written to the Mocket.
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
    public Server (int writeBufferSize, int objectBufferSize)
    {
        this(writeBufferSize, objectBufferSize, new KryoSerialization());
    }

    public Server (int writeBufferSize, int objectBufferSize, Serialization serialization)
    {
        this.writeBufferSize = writeBufferSize;
        this.objectBufferSize = objectBufferSize;
        this.serialization = serialization;
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
     * Opens a Mockets server.
     *
     * @param mocketsPort the port associated with Mockets.
     * @throws IOException if the server could not be opened.
     */
    public void bind (int mocketsPort) throws IOException
    {
        bind(new InetSocketAddress(mocketsPort));
    }

    /**
     * @param mocketsPort the port associated with Mockets.
     */
    public void bind (InetSocketAddress mocketsPort) throws IOException
    {
        close();
        synchronized (updateLock) {
            try {
                mockets = new MocketsConnection(serialization, writeBufferSize, objectBufferSize);
                mockets.bind(mocketsPort);
                LOG.debug("Bind. Accepting connections on port: " + mocketsPort + "/Mockets");
            }
            catch (IOException ex) {
                close();
                throw ex;
            }
        }
        if (INFO) LOG.info("Server opened.");
    }

    /**
     * Accepts any new connections and delegate read/write operations to child threads.
     *
     * @param timeout Wait for up to the specified milliseconds for a connection to be ready to process. May be zero
     *                to return immediately if there are no connections to process.
     */
    public void update (int timeout) throws IOException
    {
        updateThread = Thread.currentThread();

        synchronized (updateLock) {
            if (mockets == null) return;

            Connection connection = newConnection();
            try {
                Mocket clientMocket = mockets.accept(); //blocking

                connection.initialize(clientMocket, serialization, writeBufferSize, objectBufferSize);
                connection.endPoint = this;

                int id = nextConnectionID++;
                if (nextConnectionID == -1) nextConnectionID = 1;
                connection.id = id;
                connection.setConnected(true);
                connection.addListener(dispatchListener);

                //adding connection
                addConnection(connection);

                //delegate to the new Connection thread
                new Thread(connection, "Connection " + id).start();

                Thread.sleep(timeout);
            }
            catch (IOException ex) {
                connection.close();
                LOG.error("Unable to accept Mockets connection.", ex);
            }
            catch (InterruptedException e) {
                LOG.error(updateThread + " interrupted", e);
            }
        }
    }

    public void run ()
    {
        LOG.trace("Server thread started.");
        shutdown = false;
        while (!shutdown) {
            try {
                update(DEFAULT_CONNECTIONS_UPDATE_CYCLE);
            }
            catch (IOException ex) {
                LOG.trace("Error updating server connections.", ex);
                close();
            }
        }
        LOG.trace("Server thread stopped.");
    }

    public void start ()
    {
        new Thread(this, "Server").start();
    }

    public void stop ()
    {
        if (shutdown) return;
        close();
        LOG.trace("Server thread stopping.");
        shutdown = true;
    }

    /**
     * Allows the connections used by the server to be subclassed. This can be useful for storage per connection
     * without an
     * additional lookup.
     */
    protected Connection newConnection ()
    {
        return new Connection();
    }

    private void addConnection (Connection connection)
    {
        connections.add(connection);
    }

    void removeConnection (Connection connection)
    {
        connections.remove(connection);
    }

    // BOZO - Provide mechanism for sending to multiple clients without serializing multiple times.

    public void sendToAll (Object object)
    {
        List<Connection> connections = this.connections;
        for (Connection connection : connections) {
            connection.sendMockets(object);
        }
    }

    public void sendToAllExcept (int connectionID, Object object)
    {
        List<Connection> connections = this.connections;
        for (Connection connection : connections) {
            if (connection.id != connectionID) connection.sendMockets(object);
        }
    }

    public void sendTo (int connectionID, Object object)
    {
        List<Connection> connections = this.connections;
        for (Connection connection : connections) {
            if (connection.id == connectionID) {
                connection.sendMockets(object);
                break;
            }
        }
    }

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
        if (TRACE) LOG.trace("Server listener added: " + listener.getClass().getName());
    }

    public void removeListener (Listener listener)
    {
        if (listener == null) throw new IllegalArgumentException("listener cannot be null.");
        synchronized (listenerLock) {
            Listener[] listeners = this.listeners;
            int n = listeners.length;
            Listener[] newListeners = new Listener[n - 1];
            for (int i = 0, ii = 0; i < n; i++) {
                Listener copyListener = listeners[i];
                if (listener == copyListener) continue;
                if (ii == n - 1) return;
                newListeners[ii++] = copyListener;
            }
            this.listeners = newListeners;
        }
        if (TRACE) LOG.trace("Server listener removed: " + listener.getClass().getName());
    }

    /**
     * Closes all open connections and the server port(s).
     */
    public void close ()
    {
        List<Connection> connections = this.connections;
        if (INFO && connections.size() > 0) LOG.info("Closing server connections...");

        for (Connection connection : connections) {
            connection.close();
        }

        connections.clear();

        MocketsConnection mockets = this.mockets;
        if (mockets != null) {
            mockets.close();
            this.mockets = null;
        }
    }

    public Thread getUpdateThread ()
    {
        return updateThread;
    }

    /**
     * Returns the current connections. The array returned should not be modified.
     */
    public List<Connection> getConnections ()
    {
        return connections;
    }
}
