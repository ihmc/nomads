package us.ihmc.aci.netviewer.tabs;

/**
 * Notifies about changes in the amount of links to show in the links tabs
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface LinkTabsListener
{
    /**
     * Notifies that a given link needs to be added to the links tabs
     * @param id link id
     * @param name link name
     */
    void addLinkTab (String id, String name);

    /**
     * Notifies that a given link needs to be deleted from the links tabs
     * @param id link id
     */
    void removeLinkTab (String id);
}
