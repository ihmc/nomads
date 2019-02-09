/*
 * Util.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr;

/**
 * Util.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Util
{
    /**
     * Method for converting an existing IP from primitive int to a <code>String</code> object.
     *
     * @param ip int representing the IP address
     * @return A String representation of the IP Address
     */
    public static String intToIp(int ip) {

        //for inverted ips
        //    return ((ip >> 24) & 0xFF) + "." + ((ip >> 16) & 0xFF)
        //            + "." + ((ip >> 8) & 0xFF) + "." + (ip & 0xFF);

        return (ip & 0xFF) + "." + ((ip >> 8) & 0xFF)
                + "." + ((ip >> 16) & 0xFF) + "." + ((ip >> 24) & 0xFF);
    }
}
