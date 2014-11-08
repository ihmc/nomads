/*
 * Params.java
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
 * Class to be used to encapsulate the communication parameters into a single object.
 *
 * @author Marco Bindini
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.lang.IllegalArgumentException;

/**
 */
public class Params
{
    /**
     * Created a new object with the specified parameters.
     * 
     * @param tag               integer value that can be used to mark a flow of messages belonging to
     *                          the same type. The value has to be >0.
     * @param priority          used to assign a priority different than the default one (higher or lower).
     *                          The value has to be >0. Note that the priority of a message will grow every
     *                          time it is skipped in favor of a higher priority messages that gets sent first.
     *                          This mechanism is implemented to avoid message starvation. The range of priority
     *                          values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait
     *                          if there is no room in the outgoing buffer. A zero value indicates wait forever.
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery. A zero value indicates retry with no time limit.
     *                          Note that this parameter makes sense only if the flow is reliable, otherwise the
     *                          behavior is to transmit and forget about the packet.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @see Mocket.Sender#send(byte[], us.ihmc.mockets.Params) 
     * @see Mocket.Sender#send(byte[], int, int, us.ihmc.mockets.Params) 
     */
    public Params (int tag, short priority, long enqueueTimeout, long retryTimeout)
        throws IOException,IllegalArgumentException
    {
        init (tag, priority, enqueueTimeout, retryTimeout);
    }

    /**
     * Removes <code>params</code> object.
     */
    protected void finalize()
    {
        dispose();
    }

    /**
     * Native method to extract the tag. A tag is an integer value that can be used to
     * mark a flow of messages belonging to the same type. The value has to be >0.
     * 
     * @return  integer value of the tag.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int getTag()
        throws IOException, IllegalArgumentException;

    /**
     * Native method to extract the priority. The value has to be >0.
     * The range of priority values is 0-255.
     * 
     * @return  integer representing the priority value.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native short getPriority()
        throws IOException, IllegalArgumentException;

    /**
     * Native method to extract the enqueue timeout that indicates the length of time in milliseconds for which the
     * method will wait if there is no room in the outgoing buffer. A zero value indicates wait forever.
     * 
     * @return  value in milliseconds of the enqueue timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native long getEnqueueTimeout()
        throws IOException, IllegalArgumentException;

    /**
     * Native method to extract the retry timeout from Params. The retry timeout indicates the length of time for which
     * the transmitter will retransmit the packet to ensure successful delivery. A zero value indicates
     * retry with no time limit.
     * Note that this parameter makes sense only if the flow is reliable, otherwise the behavior is to
     * transmit and forget about the packet.
     * 
     * @return  value in milliseconds of the retry timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native long getRetryTimeout()
        throws IOException,IllegalArgumentException;

    /**
     * Native method to Remove <code>params</code> object.
     */
    private native void dispose();

    /**
     * Native method to initialize a new object <code>Params</code> with the specified parameters
     * 
     * @param tag               integer value that can be used to mark a flow of messages belonging to
     *                          the same type. The value has to be >0.
     * @param priority          used to assign a priority different than the default one (higher or lower).
     *                          The value has to be >0. Note that the priority of a message will grow every
     *                          time it is skipped in favor of a higher priority messages that gets sent first.
     *                          This mechanism is implemented to avoid message starvation. The range of priority
     *                          values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait
     *                          if there is no room in the outgoing buffer. A zero value indicates wait forever.
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery. A zero value indicates retry with no time limit.
     *                          Note that this parameter makes sense only if the flow is reliable, otherwise the
     *                          behavior is to transmit and forget about the packet.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    private native void init (int tag, short priority, long enqueueTimeout, long retryTimeout)
        throws IOException, IllegalArgumentException;

    // /////////////////////////////////////////////////////////////////////////
    private long _params;
}
