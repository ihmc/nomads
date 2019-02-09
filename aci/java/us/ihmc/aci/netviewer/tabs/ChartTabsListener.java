package us.ihmc.aci.netviewer.tabs;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface ChartTabsListener
{
    /**
     * Notifies that a given chart needs to be added to the charts tabs
     * @param id link id
     * @param name link name
     */
    void addChartTab (String id, String name);

    /**
     * Notifies that a given chart needs to be deleted from the charts tabs
     * @param id link id
     */
    void removeChartTab (String id);
}
