package us.ihmc.aci.dspro2.legacy;

import us.ihmc.aci.disServiceProxy.PeerStatusListener;
import us.ihmc.aci.dspro2.DSProProxyListener;
import us.ihmc.aci.util.dspro.NodePath;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class LegacyPeerStatusListener implements DSProProxyListener
{
    private final PeerStatusListener _listener;

    LegacyPeerStatusListener (PeerStatusListener listener)
    {
        _listener = listener;
    }

    public void newNeighbor(String peerID)
    {
        _listener.newPeer (peerID);
    }

    public void deadNeighbor(String peerID)
    {
        _listener.deadPeer (peerID);
    }

    public boolean pathRegistered(NodePath path, String nodeId, String teamId, String mission)
    {
        return true;
    }

    public boolean positionUpdated(float latitude, float longitude, float altitude, String nodeId)
    {
        return true;
    }

    public boolean dataArrived (String id, String groupName, String objectId,
                                String instanceId, String annotatedObjMsgId,
                                String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String queryId)
    {
        return true;
    }

    public boolean metadataArrived (String id, String groupName, String XMLMetadata,
                                    String objectId, String instanceId, String referredDataId,
                                    String queryId)
    {
        return true;
    }

    public boolean searchArrived (String queryId, String groupName, String queryType, String queryQualifiers, byte[] query)
    {
        return true;
    }
}
