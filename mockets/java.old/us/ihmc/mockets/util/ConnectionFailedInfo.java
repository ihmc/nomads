package us.ihmc.mockets.util;

public class ConnectionFailedInfo
{
    public ConnectionFailedInfo()
    {
    }

    public ConnectionFailedInfo (long time, String localAddr, String remoteAddr, int remotePort)
    {
        this.time = time;
        this.localAddr = localAddr;
        this.remoteAddr = remoteAddr;
        this.remotePort = remotePort;
    }

    public long time;
    public String localAddr;
    public String remoteAddr;
    public int remotePort;
}
