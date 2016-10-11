/*
 * MonitorForwarder.java
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

import java.io.FileWriter;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;

import com.google.gson.Gson;

/**
 * MonitorForwarder.java
 * <p/>
 * Class <code>MonitorForward</code> listens for DisService UDP packets on the port
 * UDP_PORT through the <code>DisServiceMonitor</code> and forwards them to the port UDP_PORT+1.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public final class MonitorForwarder implements DisServiceStatusListener
{
    public final Map<String, StatWrapper> _statsByPeer; // peerId -> StatWrapper
    private final int _udpListenPort;
    private DatagramSocket _datagramSocket;

    class StatWrapper
    {
        StatWrapper ()
        {
            this(null, null, null, null);
        }

        StatWrapper (DisServiceStatus.DisServiceBasicStatisticsInfo basicStatsInfo,
                     DisServiceStatus.DisServiceStatsInfo statsInfo,
                     DisServiceStatus.DisServiceDuplicateTrafficInfo duplicateTrafficInfo,
                     DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsibp)
        {
            this.basicStatsInfo = basicStatsInfo;
            this.statsInfo = statsInfo;
            this.duplicateTrafficInfo = duplicateTrafficInfo;
            this.basicStatsInfoByPeer = new HashMap<String, DisServiceStatus.DisServiceBasicStatisticsInfoByPeer>();
        }

        DisServiceStatus.DisServiceBasicStatisticsInfo basicStatsInfo;
        DisServiceStatus.DisServiceStatsInfo statsInfo;
        DisServiceStatus.DisServiceDuplicateTrafficInfo duplicateTrafficInfo;
        Map<String, DisServiceStatus.DisServiceBasicStatisticsInfoByPeer> basicStatsInfoByPeer;
    }

    public MonitorForwarder (int udpPort)
    {
        _udpListenPort = udpPort;
        try {
            DisServiceMonitor m = new DisServiceMonitor(udpPort);
            m.setListener(this);
            m.start();
            _datagramSocket = new DatagramSocket();
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }
        _statsByPeer = new HashMap<String, StatWrapper>();
    }

    public void basicStatisticsInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi)
    {
        System.out.println("Basic statistics info update for node: " + dsbsi.peerId);
        StatWrapper wr = _statsByPeer.get(dsbsi.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dsbsi.peerId, wr);
        }
        wr.basicStatsInfo = dsbsi;
        toJSONDatagram(wr);
    }

    public void overallStatsInfoUpdateArrived (DisServiceStatus.DisServiceStatsInfo dssi)
    {
        System.out.println("Overall stats info update for node: " + dssi.peerId);
        StatWrapper wr = _statsByPeer.get(dssi.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dssi.peerId, wr);
        }
        wr.statsInfo = dssi;
        //toJSONDatagram(wr);
    }

    public void duplicateTrafficStatisticInfoUpdateArrived (DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti)
    {
        System.out.println("Duplicate traffic statistics info update for node: " + dsdti.peerId);
        StatWrapper wr = _statsByPeer.get(dsdti.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dsdti.peerId, wr);
        }
        wr.duplicateTrafficInfo = dsdti;
        //toJSONDatagram(wr);
    }

    public void peerGroupStatisticInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsip)
    {
        System.out.println("Peer group statistics update for node: " + dsbsip.peerId);
        StatWrapper wr = _statsByPeer.get(dsbsip.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dsbsip.peerId, wr);
        }
        wr.basicStatsInfoByPeer.put(dsbsip.remotePeerId, dsbsip);
        //toJSONDatagram(wr);
    }

    private void toJSONDatagram (StatWrapper wrapper)
    {
        Gson gson = new Gson();
        String json = gson.toJson(wrapper);
        byte[] jsonBuf = json.getBytes();
        try {
            DatagramPacket packet = new DatagramPacket(jsonBuf, jsonBuf.length, InetAddress.getLocalHost(),
                    _udpListenPort + 1);
            _datagramSocket.send(packet);
        }
        catch (UnknownHostException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void toJSONFile (String peerId, StatWrapper wrapper)
    {
        Gson gson = new Gson();
        String json = gson.toJson(wrapper);

        try {
            //write converted json data to a file named "file.json"
            FileWriter writer = new FileWriter(peerId + ".json");
            writer.write(json);
            writer.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}
