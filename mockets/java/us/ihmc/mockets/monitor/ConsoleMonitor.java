/*
 * ConsoleMonitor.java
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

import us.ihmc.mockets.monitor.MocketStatus.MocketStatisticsInfo;

public class ConsoleMonitor implements MocketStatusListener
{
    ConsoleMonitor()
    {
    }

    public static void main (String[] args)
    {
        try {
            Monitor m = new Monitor();
            m.setListener (new ConsoleMonitor());
            m.start();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public void statusUpdateArrived (MocketStatus.EndPointsInfo epi, byte msgType)
    {
        System.out.println ("\nStatus Update <" + MocketStatus.MocketStatusNoticeType.values()[msgType] + "> received from " + epi.PID + " " + epi.identifier);
    }

    public void statisticsInfoUpdateArrived (MocketStatus.EndPointsInfo epi, MocketStatisticsInfo msi)
    {
        _updateCounter++;
        try {
            System.out.println ("\nUpdate " + _updateCounter + " received from " + epi.PID + " " + epi.identifier);
            System.out.println ("    Last contact time = " + msi.lastContactTime);
            System.out.println ("    Sent bytes = " + msi.sentBytes);
            System.out.println ("    Sent packets = " + msi.sentPackets);
            System.out.println ("    Retransmits = " + msi.retransmits);
            System.out.println ("    Received bytes = " + msi.receivedBytes);
            System.out.println ("    Received packets = " + msi.receivedPackets);
            System.out.println ("    DuplicatedDiscardedPackets = " + msi.duplicatedDiscardedPackets);
            System.out.println ("    NoRoomDiscardedPackets = " +msi.noRoomDiscardedPackets);
            System.out.println ("    ReassemblySkippedDiscardedPackets = " + msi.reassemblySkippedDiscardedPackets);
            System.out.println ("    EstimatedRTT = " + msi.estimatedRTT);
            System.out.println ("    UnacknowledgedDataSize = " + msi.unacknowledgedDataSize);
            System.out.println ("    PendingDataSize = " + msi.pendingDataSize);
            System.out.println ("    PendingPacketQueueSize = " + msi.pendingPacketQueueSize);
            System.out.println ("    ReliableSequencedDataSize = " + msi.reliableSequencedDataSize);
            System.out.println ("    ReliableSequencedPacketQueueSize = " + msi.reliableSequencedPacketQueueSize);
            System.out.println ("    ReliableUnsequencedDataSize = " + msi.reliableUnsequencedDataSize);
            System.out.println ("    ReliableUnsequencedPacketQueueSize = " + msi.reliableUnsequencedPacketQueueSize);
        }
        catch (Exception ex) {
            System.out.println ("An error has occurred: "+ex);
        }
    }
    
    public void statisticsInfoUpdateArrived (MocketStatus.EndPointsInfo epi, MocketStatus.MessageStatisticsInfo messSi)
    {
        System.out.println ("Message statistics Update");
        System.out.println ("    MsgType = " + messSi.msgType);
        System.out.println ("    SentReliableSequencedMsgs = " + messSi.sentReliableSequencedMsgs);
        System.out.println ("    SentReliableUnsequencedMsgs = " + messSi.sentReliableUnsequencedMsgs);
        System.out.println ("    SentUnreliableSequencedMsgs = " + messSi.sentUnreliableSequencedMsgs);
        System.out.println ("    SentUnreliableUnsequencedMsgs = " + messSi.sentUnreliableUnsequencedMsgs);
        System.out.println ("    ReceivedReliableSequencedMsgs = " + messSi.receivedReliableSequencedMsgs);
        System.out.println ("    ReceivedReliableUnsequencedMsgs = " + messSi.receivedReliableUnsequencedMsgs);
        System.out.println ("    ReceivedUnreliableSequencedMsgs = " + messSi.receivedUnreliableSequencedMsgs);
        System.out.println ("    ReceivedUnreliableUnsequencedMsgs = " + messSi.receivedUnreliableUnsequencedMsgs);
        System.out.println ("    CancelledPackets = " + messSi.cancelledPackets);
    }
    
    private int _updateCounter = 0;
    
}
