package us.ihmc.aci.dspro2.legacy;

import us.ihmc.aci.disServiceProProxy.DisServiceProProxyListener;
import us.ihmc.aci.dspro2.DSProProxyListener;
import us.ihmc.aci.util.dspro.NodePath;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class LegacyDSProProxyListener implements DSProProxyListener
{
    private final DisServiceProProxyListener _listener;

    LegacyDSProProxyListener(DisServiceProProxyListener listener)
    {
        _listener = listener;
    }

    public boolean pathRegistered(NodePath path, String nodeId, String teamId, String mission)
    {
        return _listener.pathRegistered (path, nodeId, teamId, mission);
    }

    public boolean positionUpdated(float latitude, float longitude, float altitude, String nodeId)
    {
        return _listener.positionUpdated (latitude, longitude, altitude, nodeId);
    }

    public void newNeighbor(String peerID)
    {
    }

    public void deadNeighbor(String peerID)
    {
    }

    public boolean dataArrived (String id, String groupName, String objectId, String instanceId,
                                String annotatedObjMsgId, String mimeType, byte[] data, short chunkNumber,
                                short totChunksNumber, String queryId)
    {
        String[] tokens = id.split (":");
                         
        if (chunkNumber == 0 && totChunksNumber == 0) {
            _listener.dataArrived (id,
                                  tokens[1], // sender
                                  groupName,
                                  Integer.parseInt (tokens[2]),
                                  objectId,
                                  instanceId,
                                  mimeType,
                                  data,
                                  0,          // metadataLength
                                  (short) 0,  // tag
                                  (byte) 0,   // priority
                                  queryId);
        }
        else {
            _listener.chunkArrived (id,
                                   tokens[1], // sender
                                   groupName,
                                   Integer.parseInt (tokens[2]),
                                   objectId,
                                   instanceId,
                                   mimeType,
                                   data, chunkNumber, totChunksNumber,
                                   id,
                                   (short) 0,  // tag
                                   (byte) 0,   // priority
                                   queryId);
        }
        return true;                           
    }

    public boolean metadataArrived (String id, String groupName, String objectId,
                                    String instanceId, String XMLMetadata,
                                    String referredDataId, String queryId)
    {
        String[] tokens = id.split (":");
        _listener.metadataArrived (id,
                                   tokens[1],  // sender
                                   tokens[0],  // groupName
                                   Integer.parseInt (tokens[2]),
                                   objectId,
                                   instanceId,
                                   null,       // mimeType
                                   XMLMetadata.getBytes(),
                                   true,       // dataChunked
                                   (short) 0,  // tag
                                   (byte) 0,   // priority
                                   queryId);
        return true;
    }

    public void searchArrived (String groupName, String queryType, String queryQualifiers, byte[] query)
    {
    }
}
