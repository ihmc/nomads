/*
 * FrameworkMessage.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

import com.esotericsoftware.minlog.Log;

/**
 * Class <code>FrameworkMessage</code> is a marker interface to denote that a message is used by the Ninja framework
 * and is generally invisible to the developer. Eg, these messages are only logged at the {@link Log#LEVEL_TRACE} level.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface FrameworkMessage
{
    static final FrameworkMessage.KeepAlive keepAlive = new KeepAlive();

    /**
     * Internal message to give the server the client's Mockets port.
     */
    static public class RegisterMockets implements FrameworkMessage
    {
        public int connectionID;
    }

    /**
     * Internal message to keep connections alive.
     */
    static public class KeepAlive implements FrameworkMessage
    {
    }

    /**
     * Internal message to discover running servers.
     */
    static public class DiscoverHost implements FrameworkMessage
    {
    }

    /**
     * Internal message to determine round trip time.
     */
    static public class Ping implements FrameworkMessage
    {
        public int id;
        public boolean isReply;
    }
}
