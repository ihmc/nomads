package us.ihmc.aci.netviewer.views.gauges;

import eu.hansolo.steelseries.tools.BackgroundColor;
import eu.hansolo.steelseries.tools.ColorDef;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 * Creates a window with a gauge
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class GaugeWindow extends JFrame
{
    public GaugeWindow (final Gauge gauge, String title, String unit)
    {
        setDefaultCloseOperation (DISPOSE_ON_CLOSE);
        setLocationByPlatform (true);

        JPanel panel = new JPanel() {
            @Override
            public Dimension getPreferredSize() {
                return new Dimension (300, 400);
            }
        };

        gauge.setBackgroundColor (BackgroundColor.WHITE);

        double min = 0;
        double max = 100;
        double value = 30;
        double threshold = 80;

        gauge.setTrackStart (min);
        gauge.setTrackStop (max);
        gauge.setTrackSection (getSection (gauge.getMaxValue(), gauge.getMinValue()));
        gauge.setValueAnimated (value);

        gauge.setTrackStartColor (Color.GREEN);
        gauge.setTrackSectionColor (Color.YELLOW);
        gauge.setTrackStopColor (Color.RED);
        gauge.setTrackVisible (true);

        gauge.setThresholdVisible (true);
        gauge.setThreshold (threshold);
        gauge.setThresholdColor (ColorDef.GREEN);

        gauge.setTitle (title);
        gauge.setUnitString (unit);

        panel.setLayout (new BorderLayout());
        panel.add (gauge.getComponent(), BorderLayout.CENTER);
        add (panel);

        JPanel buttonsPanel = new JPanel();
        JLabel valueLabel = new JLabel ("Value:");

        final JTextField valueField = new JTextField (7);
        valueField.setText (String.valueOf ((int) value));
        JButton button = new JButton ("Set");
        button.addActionListener (new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                try {
                    double value = Double.valueOf (valueField.getText());
                    gauge.setValueAnimated (value);
                }
                catch (NumberFormatException ex) {
                    //TODO - handle invalid input
                    System.err.println ("invalid input");
                }
            }
        });

        JLabel maxValueLabel = new JLabel ("Max Value:");
        final JTextField maxValueField = new JTextField (7);
        maxValueField.setText (String.valueOf ((int) max));
        JButton maxValueButton = new JButton ("Set");
        maxValueButton.addActionListener (new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                try {
                    double value = Double.valueOf (maxValueField.getText());
                    gauge.setMaxValue (value);
                    gauge.setTrackStop (gauge.getMaxValue());
                    gauge.setTrackSection (getSection (gauge.getMaxValue(), gauge.getMinValue()));
                }
                catch (NumberFormatException ex) {
                    //TODO - handle invalid input
                    System.err.println ("invalid input");
                }
            }
        });

        buttonsPanel.add(valueLabel);
        buttonsPanel.add(valueField);
        buttonsPanel.add(button);
        buttonsPanel.add(maxValueLabel);
        buttonsPanel.add(maxValueField);
        buttonsPanel.add(maxValueButton);

        add(buttonsPanel, BorderLayout.NORTH);

        setTitle (gauge.getClass().getSimpleName() + " example");
        pack();
        setVisible(true);
    }

    /**
     * Gets the section value as the middle point between min and max
     * @param max max value
     * @param min min value
     * @return the section value
     */
    private double getSection (double max, double min)
    {
        return (max - min) / 2;
    }
}
