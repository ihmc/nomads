package us.ihmc.aci.disServiceProProxy;

import us.ihmc.aci.disServiceProxy.DisseminationServiceProxyListener;
import us.ihmc.aci.util.dspro.NodePath;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public interface DisServiceProProxyListener extends DisseminationServiceProxyListener
{
    public boolean pathRegistered (NodePath path, String nodeId, String teamId, String mission);
    public boolean positionUpdated (float latitude, float longitude, float altitude, String nodeId);
}
