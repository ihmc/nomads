/*
 * Serialization.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import java.nio.ByteBuffer;

/**
 * Interface <code>Serialization</code> controls how objects are transmitted over the network.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface Serialization
{
    /**
     * @param connection May be null.
     */
    public void write (Connection connection, ByteBuffer buffer, Object object);

    public Object read (Connection connection, ByteBuffer buffer);

    /**
     * The fixed number of bytes that will be written by {@link #writeLength(ByteBuffer, int)} and read by
     * {@link #readLength(ByteBuffer)}.
     */
    public int getLengthLength ();

    public void writeLength (ByteBuffer buffer, int length);

    public int readLength (ByteBuffer buffer);
}
