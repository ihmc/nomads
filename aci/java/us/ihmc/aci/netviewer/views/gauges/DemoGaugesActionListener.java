package us.ihmc.aci.netviewer.views.gauges;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 * Listener for events to create <code>GaugeWindow</code> instances
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class DemoGaugesActionListener implements ActionListener
{
    /**
     * Constructor
     * @param title gauge title
     * @param unit gauge measure unit
     */
    public DemoGaugesActionListener (String title, String unit)
    {
        _gaugeTitle = title;
        _gaugeUnit = unit;
    }

    @Override
    public void actionPerformed (final ActionEvent e)
    {
        SwingUtilities.invokeLater (new Runnable() {
            public void run() {
                new GaugeWindow (GaugeFactory.create (GaugeType.valueOf (e.getActionCommand())), _gaugeTitle, _gaugeUnit);
            }
        });
    }

    private final String _gaugeTitle;
    private final String _gaugeUnit;
}
