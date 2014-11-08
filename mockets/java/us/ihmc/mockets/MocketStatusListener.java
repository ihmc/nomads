/*
 * MocketStatusListener.java
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
 *This interface defines methods to be used by the Mocket to periodically notify the listeners about the mocket status.
 */

package us.ihmc.mockets;

public interface MocketStatusListener
{
    /**
     * Function to be invoked when no data (or keepalive) has been received from the peer mocket.
     * The callback will indicate the time (in milliseconds) since last contact.
     * If the callback returns true, the mocket connection will be closed.
     * 
     * @param timeSinceLastContact  milliseconds from last contact.
     * @return                      boolean value that will determine the behavior of the connection: close or keep going.
     */
    public boolean peerUnreachableWarning (long timeSinceLastContact);

    /**
     * Register a callback function to be invoked once peerUnreachable has been invoked and subsequently we have heard from the peer
     * The callback will indicate the time (in milliseconds) since last contact
     *
     * @param unreachabilityIntervalLength  milliseconds after which we heard from the peer again
     * @return                              bool
     */
    public boolean peerReachable (long unreachabilityIntervalLength);

    /**
     * Register a callback function to be invoked when a suspend message has been received.
     *
     * @param timeSinceSuspension   The callback will indicate the time (in milliseconds) since the connection has been suspended.
     * @return                      boolean value indicating if the Mocket connection should be closed.
     */
    public boolean suspendReceived (long timeSinceSuspension);
}
