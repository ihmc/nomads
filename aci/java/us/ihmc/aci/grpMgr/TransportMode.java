/*
 * TransportMode.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr;

/**
 * TransportMode.java
 * <p/>
 * Class <code>TransportMode</code> identifies the different transport layer modes supported by
 * <code>GroupManager</code>.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public enum TransportMode
{
    UDP_BROADCAST(0, "UDP Broadcast"),
    UDP_MULTICAST(1, "UDP Multicast"),
    UDP_MULTICAST_NORELAY(2, "UDP Multicast (No Relay)");

    private TransportMode (int mode, String modeStr)
    {
        _mode = mode;
        _modeStr = modeStr;
    }

    public int getModeAsInt ()
    {
        return _mode;
    }

    public String getModeAsString ()
    {
        return _modeStr;
    }

    private final int _mode;
    private final String _modeStr;
}
