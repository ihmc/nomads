package us.ihmc.aci.netviewer.tabs;

import javafx.embed.swing.JFXPanel;

import javax.swing.*;

/**
 * Wrapper class for single chart tabs to include an unique id
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class SingleChartTab extends JPanel implements SingleTab
{
    /**
     * Constructor
     * @param id id associated to the tab
     * @param fxPanel javafx panel containing the charts
     */
    SingleChartTab (String id, JFXPanel fxPanel)
    {
        super();
        _id = id;

        repaint (fxPanel);
    }

    @Override
    public String getId()
    {
        return _id;
    }

    /**
     * Repaints the tab
     * @param fxPanel javafx panel containing the charts
     */
    void repaint (JFXPanel fxPanel)
    {
        removeAll();

        JScrollPane scrollPane = new JScrollPane (fxPanel);
        scrollPane.setVerticalScrollBarPolicy (JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        scrollPane.setHorizontalScrollBarPolicy (JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);

        setLayout (new BoxLayout (this, BoxLayout.PAGE_AXIS));
        add (scrollPane);
    }


    private final String _id;
}
