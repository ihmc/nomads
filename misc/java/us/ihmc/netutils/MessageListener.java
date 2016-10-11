package us.ihmc.netutils;

import us.ihmc.netutils.protocol.Protocol;

/**
 * MessageListener.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface MessageListener
{
    public void onMessage (short clientId, byte[] message);

    public void onProgress (int bytesSent, int total);

    public void onTestStream (short clientId, int streamSize, final Stats stats);

    public void onStats (short clientId, final Stats stats);

    public void onBoundError (Protocol protocol, int port);
}
