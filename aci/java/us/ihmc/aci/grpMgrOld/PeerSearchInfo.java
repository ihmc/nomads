package us.ihmc.aci.grpMgrOld;

import java.security.PublicKey;
import java.util.Vector;

/**
 * PeerSearchInfo is the base class that represents a peer search
 *
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class PeerSearchInfo
{
    public PeerSearchInfo()
    {
        _timestamp = lastRxTime = lastTxTime = System.currentTimeMillis();
    }

    public void setMessage (byte[] message, int messageLen)
    {
        this.message = (byte[]) message.clone();
        this.messageLen = messageLen;
    }

    /**
     * Get the elapsed time for the peer search packet
     */
    public int getUpdatedTTL()
    {
        if (ttl > 0) {
            ttl -= (int) (System.currentTimeMillis() - _timestamp);
            _timestamp = System.currentTimeMillis();
            return (ttl > 0) ? ttl : 0;
        }
        else {
            // indefinitive persistent peer search
            return ttl;
        }

    }

    public int getTimeSinceLastTx()
    {
        return (int) (System.currentTimeMillis() - lastTxTime);
    }

    public int getTimeSinceLastRx()
    {
        return (int) (System.currentTimeMillis() - lastRxTime);
    }

    // Class variables
    public byte[] message;
    public Vector nicsInfo; // Vector<NICInfo>
    public int hopCount;
    public int floodProb;
    public int messageLen;
    public int ttl;
    public long lastTxTime;
    public long lastRxTime;
    public PublicKey publicKey;
    public String groupName;
    public String nodeUUID;
    public String searchUUID;
    public boolean bPrivate;
    private long _timestamp;
}
