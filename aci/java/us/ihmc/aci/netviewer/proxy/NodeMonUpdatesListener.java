package us.ihmc.aci.netviewer.proxy;

import java.util.List;
import java.util.Map;

/**
 * Used to get notification coming from the node monitor
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface NodeMonUpdatesListener
{
    /**
     * Notifies about a new node being added
     * @param updateContainer container for updated data
     */
    public void newNode (UpdateContainer updateContainer);

    /**
     * Notifies about a node info being updated
     * @param updateContainer container for updated data
     */
    public void updateNode (UpdateContainer updateContainer);

    /**
     * Notifies about a dead node
     * @param id node id
     */
    public void deadNode (String id);

    /**
     * Notifies about the connection with the node monitor being closed
     */
    public void connectionClosed();
}
