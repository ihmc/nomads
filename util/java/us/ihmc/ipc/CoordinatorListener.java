/*
 * CoordinatorListener.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 */

package us.ihmc.ipc;

public interface CoordinatorListener
{
    /**
     * Invoked when a message arrives from the master
     * 
     * @param msg   the message that was received
     */
    public void msgFromMaster (Object msg);
    
    /**
     * Invoked when a message arrives from a slave
     * 
     * @param msg   the message that was received
     */
    public void msgFromSlave (Object msg);
    
    /**
     * Invoked when the master died
     */
    public void masterDied();
    
    /**
     * Invoked when one of the slaves died
     */
    public void slaveDied();

}
