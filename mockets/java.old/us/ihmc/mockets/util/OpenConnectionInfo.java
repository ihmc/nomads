package us.ihmc.mockets.util;

public class OpenConnectionInfo
{
    public OpenConnectionInfo (String localAddr, int localPort, String remoteAddr, int remotePort)
    {
        _localAddr = localAddr;
        _localPort = localPort;
        _remoteAddr = remoteAddr;
        _remotePort = remotePort;
        String hashCodeContent = localAddr + localPort + remoteAddr + remotePort;
        _hashCode = hashCodeContent.hashCode();
    }

    public int hashCode()
    {
        return _hashCode;
    }

    public boolean equals (Object obj)
    {
        if (obj instanceof OpenConnectionInfo) {
            OpenConnectionInfo rhsInfo = (OpenConnectionInfo) obj;
            if ((_localAddr.equals(rhsInfo._localAddr)) && (_localPort == rhsInfo._localPort) &&
                (_remoteAddr.equals(rhsInfo._remoteAddr)) && (_remotePort == rhsInfo._remotePort)) {
                    return true;
            }
        }
        return false;
    }

    public String getLocalAddr()
    {
        return _localAddr;
    }

    public int getLocalPort()
    {
        return _localPort;
    }

    public String getRemoteAddr()
    {
        return _remoteAddr;
    }

    public int getRemotePort()
    {
        return _remotePort;
    }

    private String _localAddr;
    private int _localPort;
    private String _remoteAddr;
    private int _remotePort;

    public long connectionStartTime;
    public long connectionCloseTime;
    public long timeSinceLastContact;
    public long lastUpdateTime;
    public boolean connectionLost;
    public int bytesSent;
    public int packetsSent;
    public int packetsRetransmitted;
    public int bytesReceived;
    public int packetsReceived;
    public int packetsDiscarded;

    private int _hashCode;
}

