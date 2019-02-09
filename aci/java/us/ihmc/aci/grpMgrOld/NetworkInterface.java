package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;

import us.ihmc.net.NICInfo;

/**
 *
 * @author Matteo Rebeschini
 */
public abstract class NetworkInterface extends Thread
{
    protected abstract void init (NICInfo nicInfo, NetworkAccessLayer nal) throws NetworkException;
    public abstract void run();
    protected abstract void terminate();
    protected abstract void broadcastPacket (byte[] packet, int packetLen, int hopCount) throws NetworkException;
    protected abstract void sendPacket (InetAddress destAddr, byte[] packet, int packetLen) throws NetworkException;
    protected abstract void receivedPacket (InetAddress destAddr, byte[] packet, int packetLen); // call back

    protected NetworkAccessLayer _nal;
    protected NICInfo _nicInfo;
}

