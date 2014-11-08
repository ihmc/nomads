/*
 * AddressUtils.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.util;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * AddressUtils.java
 *
 * Class <code>AddressUtils</code> contains different methods and utilities to convert IP Addresses in different formats.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class AddressUtils
{
    /**
     * Method for converting an existing IP from primitive int to a <code>String</code> object.
     *
     * @param ip int representing the IP address
     * @return A String representation of the IP Address
     */
    public static String intToIpString (int ip) {

        //for inverted ips
        //    return ((ip >> 24) & 0xFF) + "." + ((ip >> 16) & 0xFF)
        //            + "." + ((ip >> 8) & 0xFF) + "." + (ip & 0xFF);

        return (ip & 0xFF) + "." + ((ip >> 8) & 0xFF)
                + "." + ((ip >> 16) & 0xFF) + "." + ((ip >> 24) & 0xFF);
    }

    public static InetAddress intToInetAddress(int hostAddress) {
        InetAddress inetAddress;
        byte[] addressBytes = { (byte)(0xff & hostAddress),
                (byte)(0xff & (hostAddress >> 8)),
                (byte)(0xff & (hostAddress >> 16)),
                (byte)(0xff & (hostAddress >> 24)) };

        try {
            inetAddress = InetAddress.getByAddress(addressBytes);
        } catch(UnknownHostException e) {
            return null;
        }

        return inetAddress;
    }
}
