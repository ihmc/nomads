/*
 * PeerInfo.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.security.PublicKey;
import java.util.List;

/**
 * PeerInfo.java
 * <p/>
 * Class <code>PeerInfo</code> aims to hold all the information relative to a single peer like UUID, name,
 * last contact time,
 * the list of the groups within which the peer is member etc.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class PeerInfo
{
    public PeerInfo (String nodeUUID, String nodeName)
    {
        _nodeUUID = nodeUUID;
        _nodeName = nodeName;
    }

    private String _nodeUUID;
    private String _nodeName;
    private int _stateSeqNo;
    private int _grpDataStateSeqNo;
    private int _pingInterval;
    private InetAddress _address; // used by manager groups
    private int _port; // used by manager groups
    private long _lastContactTime;
    private PublicKey _pubKey;
    private String[] _groups;       // infos from RemoteGroupInfo
    private int _pingCount;             // Counts the number of ping packets that have been received since the last
    // reset
    private long _pingCountResetTime;   // The time when the ping count was last reset

    public String getNodeUUID ()
    {
        //_nodeUUID = getNodeUUIDNative();
        return _nodeUUID;
    }

    private native String getNodeUUIDNative ();

    public String getNodeName ()
    {
        _nodeName = getNodeNameNative();
        return _nodeName;
    }

    private native String getNodeNameNative ();

    public int getStateSeqNo ()
    {
        _stateSeqNo = getStateSeqNoNative();

        return _stateSeqNo;
    }

    private native int getStateSeqNoNative ();

    public int getGrpDataStateSeqNo ()
    {
        _grpDataStateSeqNo = getGrpDataStateSeqNoNative();

        return _grpDataStateSeqNo;
    }

    private native int getGrpDataStateSeqNoNative ();

    public int getPingInterval ()
    {
        _pingInterval = getPingIntervalNative();

        return _pingInterval;
    }

    private native int getPingIntervalNative ();

    public InetAddress getAddress ()
    {
        String address = getAddressNative();

        InetAddress addr = null;
        try {
            addr = InetAddress.getByName(address);
        }
        catch (UnknownHostException e) {
           e.printStackTrace();
        }

        return _address = addr;
    }

    private native String getAddressNative();

    public int getPort ()
    {
        _port = getPortNative();

        return _port;
    }

    private native int getPortNative();

    public long getLastContactTime ()
    {
        _lastContactTime = getLastContactTimeNative();

        return _lastContactTime;
    }

    private native long getLastContactTimeNative();

    public PublicKey getPubKey ()
    {
        return _pubKey;
    }

    private native String getPubKeyNative();

    public String[] getGroups ()
    {
        _groups = getGroupsNative();

        return _groups;
    }

    private native String[] getGroupsNative();

    public int getPingCount ()
    {
        _pingCount = getPingCountNative();

        return _pingCount;
    }

    private native int getPingCountNative();

    public long getPingCountResetTime ()
    {
        _pingCountResetTime = getPingCountResetTimeNative();

        return _pingCountResetTime;
    }

    private native long getPingCountResetTimeNative();

    private long _peerInfo;

    static {
        System.loadLibrary("grpmgrjavawrapper");
    }
}
