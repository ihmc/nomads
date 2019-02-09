package us.ihmc.aci.netviewer.tabs;

import javafx.embed.swing.JFXPanel;

import javax.swing.*;

/**
 * Wrapper for <code>JTabbedPane</code> containing <code>SingleTab</code> instances
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class TabsPanel extends JTabbedPane
{
    /**
     * Adds a new text tab
     * @param id id associated to the tab
     * @param title tab title
     * @param text text to be displayed in the tab
     */
    public synchronized void addTab (String id, String title, String text)
    {
        addTab (title, new SingleTextTab (id, text));
    }

    /**
     * Adds a new chart tab
     * @param id id associated to the tab
     * @param title tab title
     * @param fxPanel javafx panel containing the charts
     */
    public synchronized void addTab (String id, String title, JFXPanel fxPanel)
    {
        addTab (title, new SingleChartTab (id, fxPanel));
    }

    /**
     * Gets the id associated to the tab at the specific index
     * @param i tab index
     * @return the id associated to the tab at the specific index
     */
    public synchronized String getIdAt (int i)
    {
        SingleTab singleTab = (SingleTab) getComponentAt (i);

        if (singleTab == null) {
            return null;
        }

        return singleTab.getId();
    }

    /**
     * Updates the text for a given tab
     * @param id tab id
     * @param text text to be displayed in the tab
     */
    public synchronized void updateText (String id, String text)
    {
        for (int i=0; i<getTabCount(); i++) {
            if (getComponentAt (i) instanceof SingleTextTab) {
                SingleTextTab singleTab = (SingleTextTab) getComponentAt (i);
                if (singleTab.getId().equals (id)) {
                    singleTab.updateText (text);
                    return;
                }
            }
        }
    }

    /**
     * Updates the charts for a given tab
     * @param id tab id
     * @param fxPanel javafx panel containing the charts
     */
    public synchronized void updateCharts (String id, JFXPanel fxPanel)
    {
        for (int i=0; i<getTabCount(); i++) {
            if (getComponentAt (i) instanceof SingleChartTab) {
                SingleChartTab singleTab = (SingleChartTab) getComponentAt (i);
                if (singleTab.getId().equals (id)) {
                    singleTab.repaint (fxPanel);
                    return;
                }
            }
        }
    }
}
