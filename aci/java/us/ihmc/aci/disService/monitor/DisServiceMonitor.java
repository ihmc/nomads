/*
 * DisServiceMonitor.java
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

package us.ihmc.aci.disService.monitor;

import org.msgpack.MessagePack;
import org.msgpack.unpacker.Unpacker;

import java.io.ByteArrayInputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;

/**
 * Date: 5/22/13
 * @author  Maggie Breedy <mbreedy@ihmc.us>
 * $Revision$
 */
public class DisServiceMonitor extends Thread
{
    public DisServiceMonitor()
          throws SocketException
    {
        this (DisServiceStatus.DEFAULT_DIS_SERVICE_STATUS_PORT);
    }

    public DisServiceMonitor (int port)
        throws SocketException
    {
        // Create a socket to listen on a port
        _dgSocket = new DatagramSocket (port);
    }

    public void setListener (DisServiceStatusListener dssl)
    {
        _listener = dssl;
    }

    @Override
    public void run()
    {
        try {
            // Create a buffer to read datagrams into.
            byte[] buffer = new byte [2048];

            // Create a packet to receive data into the buffer
            DatagramPacket packet = new DatagramPacket (buffer, buffer.length);

            while (!_terminate) {
                // Wait to receive a datagram
                _dgSocket.receive (packet);
                // Convert the packet contents to a msgpack to display it in the gui
                MessagePack msgpack = new MessagePack();

                // Deserialize the packet
                ByteArrayInputStream in = new ByteArrayInputStream (packet.getData());
                Unpacker unpacker = msgpack.createUnpacker (in);

                if (unpacker != null) {
                    //System.out.println ("Calling update");
                    processSummaryStatsMessage (unpacker);
                }
            }
        }
        catch (Exception e) {
            System.err.println(e);
        }
    }

    //Process the packet
    protected void processSummaryStatsMessage (Unpacker unpacker)
    {
        try {
            short header = unpacker.readShort();
            //System.out.println ("****header: " + header);

            String peerId = unpacker.readString();  //nodeID

            DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi = new DisServiceStatus.DisServiceBasicStatisticsInfo (peerId);
            //System.out.println ("****dsbsip.peerId: " + dsbsi.peerId);

            short type = unpacker.readShort();
            //System.out.println ("****type: " + type);

            dsbsi.dataMessagesReceived = unpacker.readLong();
            dsbsi.dataBytesReceived = unpacker.readLong();
            dsbsi.dataFragmentsReceived = unpacker.readLong();
            dsbsi.dataFragmentBytesReceived = unpacker.readLong();
            dsbsi.missingFragmentRequestMessagesSent = unpacker.readLong();
            dsbsi.missingFragmentRequestBytesSent = unpacker.readLong();
            dsbsi.missingFragmentRequestMessagesReceived = unpacker.readLong();
            dsbsi.missingFragmentRequestBytesReceived = unpacker.readLong();
            dsbsi.dataCacheQueryMessagesSent = unpacker.readLong();
            dsbsi.dataCacheQueryBytesSent = unpacker.readLong();
            dsbsi.dataCacheQueryMessagesReceived = unpacker.readLong();
            dsbsi.dataCacheQueryBytesReceived = unpacker.readLong();
            dsbsi.topologyStateMessagesSent = unpacker.readLong();
            dsbsi.topologyStateBytesSent = unpacker.readLong();
            dsbsi.topologyStateMessagesReceived = unpacker.readLong();
            dsbsi.topologyStateBytesReceived = unpacker.readLong();
            dsbsi.keepAliveMessagesSent = unpacker.readLong();
            dsbsi.keepAliveMessagesReceived = unpacker.readLong();
            dsbsi.queryMessagesSent = unpacker.readLong();
            dsbsi.queryMessagesReceived = unpacker.readLong();
            dsbsi.queryHitsMessagesSent = unpacker.readLong();
            dsbsi.queryHitsMessagesReceived = unpacker.readLong();
            //System.out.println ("****dsbsi.queryHitsMessagesReceived: " + dsbsi.queryHitsMessagesReceived);

            if (_listener != null) {
                _listener.basicStatisticsInfoUpdateArrived (dsbsi);
            }            
            if (unpacker.readShort() == DisServiceStatus.DisServiceStatusFlags.DSSF_OverallStats.ordinal()) {
                DisServiceStatus.DisServiceStatsInfo dssi = new DisServiceStatus.DisServiceStatsInfo (peerId);
                dssi.clientMessagesPushed = unpacker.readLong();
                dssi.clientBytesPushed = unpacker.readLong();
                dssi.clientMessagesMadeAvailable = unpacker.readLong();
                dssi.clientBytesMadeAvailable = unpacker.readLong();
                dssi.fragmentsPushed = unpacker.readLong();
                dssi.fragmentBytesPushed = unpacker.readLong();
                dssi.onDemandFragmentsSent = unpacker.readLong();
                dssi.onDemandFragmentBytesSent = unpacker.readLong();
                //System.out.println ("****dssi.onDemandFragmentBytesSent: " + dssi.onDemandFragmentBytesSent);

                if (_listener != null) {
                    _listener.overallStatsInfoUpdateArrived (dssi);
                }
            }

            if (unpacker.readShort() == DisServiceStatus.DisServiceStatusFlags.DSSF_DuplicateTrafficInfo .ordinal()) {
                DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti = new DisServiceStatus.DisServiceDuplicateTrafficInfo (peerId);
                dsdti.overheardDuplicateTraffic = unpacker.readLong();
                dsdti.targetedDuplicateTraffic = unpacker.readLong();
                //System.out.println ("****dsdti.targetedDuplicateTraffic: " + dsdti.targetedDuplicateTraffic);
                if (_listener != null) {
                    _listener.duplicateTrafficStatisticInfoUpdateArrived (dsdti);
                }
            }

            short flag = unpacker.readShort();
            for (; flag == DisServiceStatus.DisServiceStatusFlags.DSSF_PerClientGroupTagStats.ordinal(); flag = unpacker.readShort()) {
                DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsibp = new DisServiceStatus.DisServiceBasicStatisticsInfoByPeer (peerId);
                dsbsibp.remotePeerId = unpacker.readString();
                //System.out.println ("****dsbsibp.peerId: " + dsbsibp.peerId);
                dsbsibp.dataMessagesReceived = unpacker.readLong();
                dsbsibp.dataBytesReceived = unpacker.readLong();
                dsbsibp.dataFragmentsReceived = unpacker.readLong();
                dsbsibp.dataFragmentBytesReceived = unpacker.readLong();
                dsbsibp.missingFragmentRequestMessagesReceived = unpacker.readLong();
                dsbsibp.missingFragmentRequestBytesReceived = unpacker.readLong();
                dsbsibp.keepAliveMessagesReceived = unpacker.readLong();

                if (_listener != null) {
                    _listener.peerGroupStatisticInfoUpdateArrived (dsbsibp);
                }
            }

            int flagend = unpacker.readInt();
            //System.out.println ("****endFlag: " + flagend);
        }
        catch (Exception exp) {
            //exp.printStackTrace();
        }
    }

    public void requestTermination()
    {
        _terminate = true;
    }

    private DatagramSocket _dgSocket;
    private DisServiceStatusListener _listener;
    private boolean _terminate = false;
}
