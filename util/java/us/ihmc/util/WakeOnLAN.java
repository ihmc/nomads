/*
 * WakeOnLAN.java
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
import java.net.DatagramPacket;
import java.net.DatagramSocket;

/**
 * Date: Oct 2, 2009
 * Time: 9:36:33 AM
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision: 1.6 $
 */

public class WakeOnLAN
{
    public WakeOnLAN()
    {
    }

    public static void main (String[] args) 
    {
        if (args.length != 1) {
            System.out.println("Usage: <MACaddress>");
            System.exit (1);
        }
        String macAddr = args[0];
        WakeOnLAN wakeonLan = new WakeOnLAN();
        wakeonLan.wakeup (macAddr);
    }

    //Makes the wake up call
    public void wakeup (String macAddress)
    {
        try {
            byte[] macByteArray = getMACAddressBytes (macAddress);
            byte[] bytes = new byte [6 + 16 * macByteArray.length];
            for (int i = 0; i < 6; i++) {
                bytes[i] = (byte) 0xff;
            }
            for (int i = 6; i < bytes.length; i += macByteArray.length) {
                System.arraycopy (macByteArray, 0, bytes, i, macByteArray.length);
            }

            System.out.println("Wake-on-LAN: Creating the datagram socket...");

            InetAddress inetAddress = InetAddress.getByName ("255.255.255.255");
            DatagramPacket dgPacket = new DatagramPacket(bytes, bytes.length, inetAddress, PORT);
            DatagramSocket dgSocket = new DatagramSocket();

            System.out.println("Wake-on-LAN: Sending the datagram packet...");
            dgSocket.send (dgPacket);
            dgSocket.close();

            System.out.println ("Wake-on-LAN packet sent.");
        }
        catch (Exception e) {
            System.out.println("Failed to send Wake-on-LAN packet: + e");
            e.printStackTrace();
            System.exit(1);
        }
    }

    //Gets the byte array in hex of the MAC Address 
    protected byte[] getMACAddressBytes (String macAddrStr)
    {
        byte[] byteArray = new byte [6];
        String[] hex = macAddrStr.split ("(\\:|\\-)");

        if (hex.length != 6) {
            System.out.println ("Invalid MAC address.");
        }
        try {
            for (int i = 0; i < 6; i++) {
                byteArray[i] = (byte) Integer.parseInt (hex[i], 16);
            }
        }
        catch (NumberFormatException nfe) {
            nfe.printStackTrace();
        }
        return byteArray;
    }

    //Variables
    public static final int PORT = 5;
}
