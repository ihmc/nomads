/*
 * Monitor.java
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
 * @author ebenvegnu
 */

package us.ihmc.mockets.monitor;

import us.ihmc.util.ByteConverter;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;

public class Monitor extends Thread
{
    public Monitor()
        throws SocketException
    {
        this (MocketStatus.DEFAULT_MOCKET_STATUS_PORT);
    }

    public Monitor (int port)
        throws SocketException
    {
        _dgSocket = new DatagramSocket (port);
    }

    public void setListener (MocketStatusListener msl)
    {
        _listener = msl;
    }

    public void run()
    {
        //DatagramPacket dgPacket = new java.net.DatagramPacket(new byte[2048], 2048);
        DatagramPacket dgPacket = new java.net.DatagramPacket(new byte[65535], 65535);
        while (true) {
            try {
                _dgSocket.receive (dgPacket);
            }
            catch (IOException e) {
                e.printStackTrace();
                return;
            }
            //System.out.println ("Received a new packet of size " + dgPacket.getLength());
            ByteArrayInputStream bais = new ByteArrayInputStream (dgPacket.getData());
            DataInputStream dis = new DataInputStream (bais);
            
            try {
                processMessage (dis);
            }
            catch (IOException e) {
                e.printStackTrace();
                // This is a parsing / packet format error - so continue
                continue;
            }
        }
    }
    
    private void processMessage (DataInputStream dis)
        throws java.io.IOException
    {
        MocketStatus.EndPointsInfo epi = new MocketStatus.EndPointsInfo();
        // Message type, process ID, identifier
        byte msgType = dis.readByte();

        byte buf[] = new byte[4];
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        epi.PID = ByteConverter.from4BytesToUnsignedInt (buf, 0);

        byte buf2[] = new byte[2];
        dis.readFully (buf2);
        ByteConverter.swapBytes (buf2, 0, 2);
        int idLength = ByteConverter.from2BytesToUnsignedShortInt (buf2, 0);
        if (idLength > 0) {
            byte[] id = new byte[idLength];
            dis.readFully (id);
            epi.identifier = new String(id);
        }
        dis.readByte();   // String terminator

        //buf = new byte[4];
        dis.readFully (buf);
        epi.localAddress = InetAddress.getByAddress(buf);
        ByteConverter.swapBytes (buf, 0, 4);
        epi.localAddr = ByteConverter.from4BytesToUnsignedInt (buf, 0);

        byte buf3[] = new byte[2];
        dis.readFully (buf3);
        ByteConverter.swapBytes (buf3, 0, 2);
        epi.localPort = ByteConverter.from2BytesToUnsignedShortInt (buf3, 0);

        //buf = new byte[4];
        dis.readFully (buf);
        epi.remoteAddress = InetAddress.getByAddress (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        epi.remoteAddr = ByteConverter.from4BytesToUnsignedInt (buf, 0);

        byte buf4[] = new byte[2];
        dis.readFully (buf4);
        ByteConverter.swapBytes (buf4, 0, 2);
        epi.remotePort = ByteConverter.from2BytesToUnsignedShortInt (buf4, 0);
        
//        System.out.println ("***Update of type "+MocketStatus.MocketStatusNoticeType.values()[msgType]+" from "+epi.PID+", "+epi.identifier);
//        System.out.println ("loc Add, port: "+ epi.localAddr+" "+ epi.localPort);
//        System.out.println ("rem Add, port: "+ epi.remoteAddr+" "+ epi.remotePort);
        
        if (msgType == MocketStatus.MocketStatusNoticeType.MSNT_Stats.ordinal()) {
            processStatsMessage (dis, epi);
        }
        else {
            _listener.statusUpdateArrived (epi, msgType);
        }
    }

    private void processStatsMessage (DataInputStream dis, MocketStatus.EndPointsInfo epi)
        throws java.io.IOException
    {
        MocketStatus.MocketStatisticsInfo msi = new MocketStatus.MocketStatisticsInfo();
        byte buf8[] = new byte[8];
        dis.readFully (buf8);
        ByteConverter.swapBytes (buf8, 0, 8);
        msi.lastContactTime = ByteConverter.from8BytesToLong (buf8, 0);
        byte buf[] = new byte[4];
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.sentBytes = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.sentPackets = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.retransmits = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.receivedBytes = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.receivedPackets = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.duplicatedDiscardedPackets = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.noRoomDiscardedPackets = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.reassemblySkippedDiscardedPackets = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        DataInputStream di = new DataInputStream(new ByteArrayInputStream(buf));
        msi.estimatedRTT = di.readFloat();
        
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.unacknowledgedDataSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        //TODO: unack data size is 0, the correct value is not provided
        //TODO: unack queue size is missing!! No stat info is provided;
        
        
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.pendingDataSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.pendingPacketQueueSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.reliableSequencedDataSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.reliableSequencedPacketQueueSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.reliableUnsequencedDataSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        dis.readFully (buf);
        ByteConverter.swapBytes (buf, 0, 4);
        msi.reliableUnsequencedPacketQueueSize = ByteConverter.from4BytesToUnsignedInt (buf, 0);
        
        
        byte flags = dis.readByte();
        if (flags != MocketStatus.MocketStatusFlags.MSF_OverallMessageStatistics.ordinal()) {
            System.out.println ("Error: flags byte set to "+flags);
        }
        
        if (_listener != null) {
            _listener.statisticsInfoUpdateArrived (epi, msi);
        }
        
        MocketStatus.MessageStatisticsInfo messSi = new MocketStatus.MessageStatisticsInfo();
        byte buf2[] = new byte[2];
        dis.readFully (buf2);
        ByteConverter.swapBytes (buf2, 0, 2);
        messSi.msgType = ByteConverter.from2BytesToUnsignedShortInt (buf2, 0);
        //System.out.println ("Message type "+MocketStatus.MocketStatusNoticeType.values()[messSi.msgType]);
        while (messSi.msgType == MocketStatus.MocketStatusNoticeType.MSNT_Stats.ordinal()) {
            System.out.println ("Message statistics: msgType "+MocketStatus.MocketStatusNoticeType.values()[messSi.msgType]);
            //buf = new byte[4];
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.sentReliableSequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.sentReliableUnsequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.sentUnreliableSequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.sentUnreliableUnsequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.receivedReliableSequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.receivedReliableUnsequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.receivedUnreliableSequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.receivedUnreliableUnsequencedMsgs = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            dis.readFully (buf);
            ByteConverter.swapBytes (buf, 0, 4);
            messSi.cancelledPackets = ByteConverter.from4BytesToUnsignedInt (buf, 0);
            
            if (_listener != null) {
                _listener.statisticsInfoUpdateArrived (epi, messSi);
            }

            dis.readFully (buf2);
            ByteConverter.swapBytes (buf2, 0, 2);
            messSi.msgType = ByteConverter.from2BytesToUnsignedShortInt (buf2, 0);
        }
    }
    
    private DatagramSocket _dgSocket;
    private MocketStatusListener _listener;
    
}
