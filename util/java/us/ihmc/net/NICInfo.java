/*
 * NICInfo.java
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

import java.net.InetAddress;

/**
 * Class describing a Network Interface
 * <p>
 * Created on October 02, 2006
 *
 * @author Matteo Rebeschini <mrebeschini@ihmc.us>
 * @version $Revision: 1.6 $
 * $Date: 2005/09/27 23:30:49
 */
public class NICInfo
{
    public NICInfo()
    {
    }

    /**
     * This constructor guesses the netmask and the IF broadcast 
     * addresses based on the IP address of interface. 
     *
     * We are therefore considering only class A, B and C networks.
     */
    public NICInfo (InetAddress ipAddr)
    {
        byte[] ip = ipAddr.getAddress();
        byte[] netmask = new byte[4];
        byte[] broadcast = new byte[4];

        int firstOct = (((int) ip[0]) + 256) % 256;
        
        if (firstOct <= 127) { // class A
            netmask[0] = -1;
            netmask[1] = 0;
            netmask[2] = 0;
            netmask[3] = 0;
        }
        else if ((firstOct >= 128) && (firstOct <= 191)) { //class B
            netmask[0] = -1;
            netmask[1] = -1;
            netmask[2] = 0;
            netmask[3] = 0;
        }
        else if ((firstOct >= 192) && (firstOct <= 223)) { // class C
            netmask[0] = -1;
            netmask[1] = -1;
            netmask[2] = -1;
            netmask[3] = 0;
        }
      
        for (int i=0; i<4; i++) {
            broadcast[i] = (byte) (ip[i] | ~netmask[i]);
        }
        
        this.ip = ipAddr;  

        try { 
            this.netmask = InetAddress.getByAddress (netmask);    
            this.broadcast = InetAddress.getByAddress (broadcast);
        }
        catch (java.net.UnknownHostException uhe) {
            uhe.printStackTrace();
        }
    }

    public NICInfo (InetAddress ipAddr, InetAddress netmaskAddr)
    {
        byte[] ip = ipAddr.getAddress();
        byte[] netmask = netmaskAddr.getAddress();
        byte[] broadcast = new byte[4];

        for (int i=0; i<4; i++) {
            broadcast[i] = (byte) (ip[i] | ~netmask[i]);
        }

        this.ip = ipAddr;
        this.netmask = netmaskAddr;

        try {
            this.broadcast = InetAddress.getByAddress (broadcast);
        }
        catch (java.net.UnknownHostException uhe) {
            uhe.printStackTrace();
        }
    }

    public NICInfo (InetAddress ipAddr, InetAddress netmaskAddr, InetAddress broadcastAddr)
    {
        this.ip = ipAddr;
        this.netmask = netmaskAddr;
        this.broadcast = broadcastAddr;
    }

    public InetAddress ip;
    public InetAddress netmask;
    public InetAddress broadcast;

    public String toString()
    {
        return ip.getHostAddress() + "/" + 
               netmask.getHostAddress() + "/" + 
               broadcast.getHostAddress();
    }
}
