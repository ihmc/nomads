/*
 * EndPoint.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import java.io.IOException;

import com.esotericsoftware.kryo.Kryo;

/**
 * Class <code>EndPoint</code> represents the local end point of a connection.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface EndPoint extends Runnable
{
    /**
     * Gets the serialization instance that will be used to serialize and deserialize objects.
     */
    public Serialization getSerialization ();

    /**
     * If the listener already exists, it is not added again.
     */
    public void addListener (Listener listener);

    public void removeListener (Listener listener);

    /**
     * Continually updates this end point until {@link #stop()} is called.
     */
    public void run ();

    /**
     * Starts a new thread that calls {@link #run()}.
     */
    public void start ();

    /**
     * Closes this end point and causes {@link #run()} to return.
     */
    public void stop ();

    /**
     * @see Client
     * @see Server
     */
    public void close ();

    /**
     * @see Client#update(int)
     * @see Server#update(int)
     */
    public void update (int timeout) throws IOException;

    /**
     * Returns the last thread that called {@link #update(int)} for this end point. This can be useful to detect when
     * long running
     * code will be run on the update thread.
     */
    public Thread getUpdateThread ();

    /**
     * Gets the Kryo instance that will be used to serialize and deserialize objects. This is only valid if
     * {@link KryoSerialization} is being used, which is the default.
     */
    public Kryo getKryo ();
}
