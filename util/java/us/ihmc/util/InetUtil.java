/*
 * InetUtil.java
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

package us.ihmc.util;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;
import java.util.HashMap;

/**
 * InetUtil
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.6 $
 * @ $Date: 2016/06/09 20:02:46 $
 * @ Created on 12:40:08 AM Feb 18, 2006
 */

@SuppressWarnings("rawtypes")
public class InetUtil
{
    /**
     *
     */
    public static InetAddress getPreferredLocalAddress()
    {
        return _preferredLocalAddress;
    }

    /**
     *
     */
    public static boolean isLocalAddress(InetAddress address)
    {
        return _localAdds.containsKey(address);
    }

    /**
     *
     */
    public static String getFQDN (InetAddress inetAddress)
    {
        try {
            inetAddress = InetAddress.getByName (inetAddress.getHostAddress());
            return inetAddress.getHostName();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     * Initializing local addresses.
     */
    @SuppressWarnings({ "unchecked" })
    private static HashMap<InetAddress, String> loadLocalInterfaceAddresses()
    {
        HashMap ipMap = new HashMap();

        try {
            Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
            while (interfaces.hasMoreElements()) {
                NetworkInterface netIF = interfaces.nextElement();
                Enumeration<InetAddress> ips = netIF.getInetAddresses();
                while (ips.hasMoreElements()) {
                    InetAddress inetAdd = ips.nextElement();
                    ipMap.put(inetAdd, "");
                }
            }

            return ipMap;
        }
        catch (Exception e) {
            System.out.println("[InetUtil] got Exception when loading list of IP_Addrresses");
            e.printStackTrace();
        }

        return null;
    }

    // /////////////////////////////////////////////////////////////////////////
    private static HashMap<InetAddress, String> _localAdds;
    private static InetAddress _preferredLocalAddress = null;

    // /////////////////////////////////////////////////////////////////////////
    // STATIC INITIALIZED //////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    static
    {
        // load the local addresses...
        _localAdds = loadLocalInterfaceAddresses();
        if (_localAdds == null) {
            _localAdds = new HashMap<InetAddress, String>();
        }

        String prefix = null;
        //check if a configloader has been initilzied
        try {
            ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
            prefix = cloader.getProperty(InetUtil.INETUTIL_PREFERRED_IP_PREFIX);
        }
        catch (RuntimeException re) {
            //If not initialized, check if there's an env.variable set for the prefix
            prefix = System.getProperty(InetUtil.INETUTIL_PREFERRED_IP_PREFIX);
        }

        // now, determine the preferred local address.
        try {
           if (_localAdds != null && prefix != null) {
                for (InetAddress inetAdd : _localAdds.keySet()) {
                    if (inetAdd.getHostAddress().startsWith(prefix)) {
                        _preferredLocalAddress = inetAdd;
                        break;
                    }
                }
            }
        }
        catch (Exception ex) {
            System.out.println("[InetUtil] exception trying to determine the preferred local address.");
            ex.printStackTrace();
        }

        if (_preferredLocalAddress == null) {
            //if conditions are not met, use localAddress as last resort
            try {
                _preferredLocalAddress = InetAddress.getLocalHost();
            }
            catch (Exception ex) {
                System.out.println("[InetUtil] could not determine the preferred local address.");
                ex.printStackTrace();
            }
        }
    }

    public static final String INETUTIL_PREFERRED_IP_PREFIX =  "inetutil.preferred.ip.prefix";
}

