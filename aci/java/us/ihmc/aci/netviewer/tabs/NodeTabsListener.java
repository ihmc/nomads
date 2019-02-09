package us.ihmc.aci.netviewer.tabs;

/**
 * Notifies about changes in the amount of nodes to show in the nodes tabs
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface NodeTabsListener
{
    /**
     * Notifies that a given node needs to be added to the nodes tabs
     * @param id node id
     * @param name node name
     */
    void addNodeTab (String id, String name);

    /**
     * Notifies that a given node needs to be deleted from the nodes tabs
     * @param id node id
     */
    void removeNodeTab (String id);
}
