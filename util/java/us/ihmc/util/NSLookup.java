/*
 * NSLookup.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

/**
 * A nslookup emulator in java
 * 
 * @author Marco Carvalho
 * @version $Revision  $
 */

import java.net.InetAddress;
import java.util.StringTokenizer;

public class NSLookup
{
    public static String getHostname()
    {
        try {
            StringTokenizer stk = new StringTokenizer (getFQDN(), ".");
            return stk.nextToken();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static String getFQDN()
    {
        try {
            InetAddress add = null;
            add = InetAddress.getLocalHost();
            add = InetAddress.getByName (add.getHostAddress());
            return add.getHostName();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static String getHostname (InetAddress inetAddress)
    {
        try {
            StringTokenizer stk = new StringTokenizer (getFQDN (inetAddress), ".");
            return stk.nextToken();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    
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
    
    public static String getHostAddress()
    {
        try {
            InetAddress add = null;
            add = InetAddress.getLocalHost();
            add = InetAddress.getByName (add.getHostAddress());
            return add.getHostAddress();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static void main(String args[]) throws Exception
    {
        String host = null;
        if(args.length>0) {
            host = args[0];
        }
        
        try 
        {   InetAddress add = null;
            if(host == null) {
                add = InetAddress.getLocalHost();
                add = InetAddress.getByName(add.getHostAddress());
            } else {
                add = InetAddress.getByName(host);
            }
            byte[] ip = add.getAddress();
            for(int i=0;i<ip.length;i++)
            {   if(i>0) System.out.print(".");
                System.out.print((ip[i]) & 0xff);
            }
            System.out.println();
            System.out.println(add.getHostName());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
