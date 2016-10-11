/*
 * NetUtils.java
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

package us.ihmc.net;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Enumeration;
import java.util.Vector;

import us.ihmc.util.ByteConverter;

/**
 * Created on April 7, 2003, 10:04 AM
 *
 * @author agranados@ihmc.us
 * @author tcowin@ihmc.us [made to be compatible with jdk1.2.X]
 * @author Matteo Rebeschini <mrebeschini@ihmc.us>
 *
 * @version $Revision: 1.20 $
 * $Date: 2005/09/27 23:30:49
 */
public class NetUtils
{

    /**
     * Returns true if the host is the local host
     *
     * @param host - the host
     * @return true if the host is the local host, false otherwise
     */
    public static boolean isLocalHost (String host)
    {
        InetAddress destInAd = null;
        try {
            destInAd = InetAddress.getByName (host);
        }
        catch (UnknownHostException e) {
            e.printStackTrace();
        }

        return isLocalAddress (destInAd);
    }

    /**
     * Returns true if the address points to the local host
     *
     * @param address - the address
     * @return true if the address points to the local host, false otherwise
     */
    public static boolean isLocalAddress (InetAddress address)
    {
        return _localIPAddrs.contains (address);
    }

    /**
     * Checks if <code>address</code> is the address of one the Netowork
     * Interfaces contained in <code>netIFs</code>
     *
     * @param address 
     * @param localNetIFs Vector<NICInfo>
     * @return <code>true</code> if a match is found
     */
    public static boolean isLocalAddress (InetAddress address, Vector localNetIFs)
    { 
        if (localNetIFs == null) {
            localNetIFs = NetUtils.getNICsInfo (true, false);
        }
        for (int i = 0; i < localNetIFs.size(); i++) {
            if (address.equals (((NICInfo) localNetIFs.elementAt (i)).ip)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns true if the host is a remote host
     *
     * @param host - the host
     * @return true if the host is remote host, false otherwise
     */
    public static boolean isRemoteHost (String host)
    {
        return !isLocalHost (host);
    }

    /**
     * Returns true if the address points to a remote host
     *
     * @param address - the address
     * @return true if the address is remote, false otherwise
     */
    public static boolean isRemoteAddress (InetAddress address)
    {
        return !isLocalAddress (address);
    }
    
    /**
     * Checks whether ipAddr is reachable directly with one of the locaNetIFs.
     * 
     * Note: since the netmask information is not available, we are not supporting
     * subnets (only class A, B & C networks).
     * 
     * @param ipAddr
     * @param localNICs
     * @return
     */
    public static boolean isReachableDirectly (InetAddress ipAddr, Vector localNetIFs)
    {
    	NICInfo ni = new NICInfo (ipAddr);
    	
    	if (localNetIFs == null) {
    		localNetIFs = NetUtils.getNICsInfo (true, false);
        }
        for (int i = 0; i < localNetIFs.size(); i++) {
        	NICInfo localNIC = (NICInfo) localNetIFs.elementAt (i);
        	if (NetUtils.areInSameNetwork(ni, localNIC)) {
        		return true;
        	}
        }
    	return false;
    }

    /**
     * Best attempt to get the local hostname, with or without the 
     * existence of a DNS entry
     */ 
    public static String getLocalHostName()
    {
        String localHostName = null;
        try {
            InetAddress addr = InetAddress.getLocalHost();
            localHostName = addr.getHostName();
        } 
        catch (UnknownHostException uhe) {
            String host = uhe.getMessage();
            if (host != null) {
                return host.substring (0, host.indexOf (':'));
            }
            return null; 
        }

        return localHostName;
    }
 
    
    /**
     * Get the IP addresses of the installed network interfaces.
     * 
     * @param includeLoopback Include loopback interface
     * @param uniqueNetworks Include only unique networks (if two interfaces are on the
     *        same network, only the IP address of one of them will be returned.
     * @return Vector<us.ihmc.net.NICInfo>
     */
    public static Vector getNICsInfo (boolean includeLoopback, boolean uniqueNetworks)
    {
        Vector netIFs = new Vector();
        
    ipaddr:
        for (int i=0; i < _localIPAddrs.size(); i++) {
            InetAddress inetAddr = (InetAddress) _localIPAddrs.elementAt (i);;
            if (!includeLoopback && inetAddr.isLoopbackAddress()) {
                continue;
            }
            if (uniqueNetworks) {
                for (int j = 0; j < netIFs.size(); j++) {
                    if (areInSameNetwork (new NICInfo (inetAddr), 
                                          (NICInfo) netIFs.elementAt (j))) {
                        continue ipaddr;
                    }
                }
            }
            netIFs.add (new NICInfo (inetAddr));
        }
        return netIFs;   
    }

    public static NICInfo getNICInfo (InetAddress ipAddr)
    {
        for (int i=0; i < _localIPAddrs.size(); i++) {
            if (((InetAddress) _localIPAddrs.elementAt (i)).equals (ipAddr)) {
                return new NICInfo (ipAddr);
            }
        }
        return null;
    }

    /**
     * Given the network interfaces of the local node and of a remote node, it determines which IP address of the
     * remote node is reachable from the local node. It returns the first IP address that is reachable.
     * <p>
     * If the local network interfaces are not specified (NULL), they are determined automatically.
     *
     * @param remoteNetIFs Vector<NICInfo>
     * @param localNetIFs Vector<NICInfo>
     *
     * @return first IP address reachable
     */
    public static InetAddress determineDestIPAddr (Vector remoteNetIFs, Vector localNetIFs)
    {
        if (localNetIFs == null) {
            localNetIFs = NetUtils.getNICsInfo (false, true);
        }
        
        for (int i = 0; i < localNetIFs.size(); i++) {
            for (int j = 0; j < remoteNetIFs.size(); j++) {
                if (NetUtils.areInSameNetwork ((NICInfo) localNetIFs.elementAt (i), 
                                               (NICInfo) remoteNetIFs.elementAt (j))) {
                    return ((NICInfo) remoteNetIFs.elementAt(j)).ip;
                }
            }
        }

        // NOTE: if we cannot find any address that is directly reachable
        // we assume that the OS will properly route the packet. 
        return ((NICInfo) remoteNetIFs.elementAt(0)).ip;
    }

    public static boolean isMulticastAddress (String ipAddr)
    {
        try {
            return isMulticastAddress (InetAddress.getByName (ipAddr));
        }
        catch (Exception e) {
            return false;
        }
    }

    public static boolean isMulticastAddress (InetAddress ipAddr)
    {
        int firstOctet = ipAddr.getAddress()[0];
        if (firstOctet < 0) {
            firstOctet += 255;
        }
        return ((firstOctet >= 224) && (firstOctet < 240));
    }

    private static boolean areInSameNetwork (NICInfo a, NICInfo b)
    {
        return ((ByteConverter.from4BytesToUnsignedInt (a.ip.getAddress(), 0) &
                 ByteConverter.from4BytesToUnsignedInt (a.netmask.getAddress(), 0)) ==
                (ByteConverter.from4BytesToUnsignedInt (b.ip.getAddress(), 0) &
                 ByteConverter.from4BytesToUnsignedInt (b.netmask.getAddress(), 0)));
    }
    
    private static void loadNICsInfo()
    {
        Enumeration allNetIFs;
        try {
           allNetIFs = java.net.NetworkInterface.getNetworkInterfaces();
        }
        catch (SocketException se) {
            System.err.println ("NetUtils.loadNICsInfo: error getting network interfaces information");
            return;
        }

        while (allNetIFs.hasMoreElements()) {
            java.net.NetworkInterface netIF = (java.net.NetworkInterface) allNetIFs.nextElement();
            Enumeration ips = netIF.getInetAddresses();

            while (ips.hasMoreElements()) {
                InetAddress inetAddr = (InetAddress) ips.nextElement();

                // we are only interested in IPv4 addresses (for now)
                if (inetAddr instanceof Inet4Address) {
                    _localIPAddrs.add (inetAddr);
                }
            }
        }
    }

    
    //Class Variables
    private static Vector _localIPAddrs = new Vector(); //Vector<InetAddress>

    static
    {
        // load the local addresses.        
        loadNICsInfo();
    }
}
