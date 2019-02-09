package us.ihmc.aci.netviewer.views.charts;

import javafx.embed.swing.JFXPanel;
import us.ihmc.charts.ChartsPanel;
import us.ihmc.charts.model.*;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.GregorianCalendar;
import java.util.List;

/**
 * Listener for events to create charts
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class DemoChartsActionListener implements ActionListener
{
    /**
     * Constructor
     * @param title chart window title
     */
    public DemoChartsActionListener (String title)
    {
        _title = title;
    }

    @Override
    public void actionPerformed (ActionEvent e)
    {
        SwingUtilities.invokeLater (new Runnable() {
            public void run() {
                JFrame frame = new JFrame (_title);

                List<ChartInfo> chartInfoList = new ArrayList<>();

                XYData xyData = new XYData (XYData.ChartType.area_date);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 10).getTime(), 10);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 15).getTime(), 5);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 20).getTime(), 8);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 25).getTime(), 1);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 30).getTime(), 19);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 35).getTime(), 18);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 40).getTime(), 25);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 45).getTime(), 6);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 50).getTime(), 30);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 55).getTime(), 35);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 0).getTime(), 30);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 5).getTime(), 24);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 10).getTime(), 10);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 15).getTime(), 5);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 20).getTime(), 8);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 25).getTime(), 1);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 30).getTime(), 19);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 35).getTime(), 18);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 40).getTime(), 25);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 45).getTime(), 6);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 50).getTime(), 30);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 55).getTime(), 35);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 12, 0).getTime(), 30);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate (5 seconds stats update) for eMIP", "Time",
                        "Bandwidth (B/s)", xyData));

                xyData = new XYData (XYData.ChartType.area_date);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 10).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 15).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 20).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 25).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 30).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 35).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 40).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 45).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 50).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 10, 55).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 0).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 5).getTime(), 0);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 10).getTime(), 20);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 15).getTime(), 15);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 20).getTime(), 10);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 25).getTime(), 18);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 30).getTime(), 22);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 35).getTime(), 25);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 40).getTime(), 28);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 45).getTime(), 6);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 50).getTime(), 30);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 55).getTime(), 35);
                xyData.add (new GregorianCalendar (2016, 0, 12, 14, 12, 0).getTime(), 30);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate (1 minute stats update) for eMIP", "Time",
                        "Bandwidth (B/s)", xyData));

                Bar2DData bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 10d);
                bar2DData.add ("214.15.3.161", 20d);
                bar2DData.add ("214.15.3.165", 30d);
                bar2DData.add ("214.15.3.191", 5d);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate By IP (5 seconds stats update) for eMIP",
                        "IP", "Bandwidth (B/s)", bar2DData));

                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 120d);
                bar2DData.add ("214.15.3.161", 350d);
                bar2DData.add ("214.15.3.165", 460d);
                bar2DData.add ("214.15.3.191", 50d);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate By IP (1 minute stats update) for eMIP",
                        "IP", "Bandwidth (B/s)", bar2DData));

                Bar2DSeriesData seriesData = new Bar2DSeriesData (Bar2DSeriesData.ChartType.stack);
                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 5d);
                bar2DData.add ("214.15.3.161", 8d);
                bar2DData.add ("214.15.3.165", 5d);
                bar2DData.add ("214.15.3.191", 1d);
                seriesData.add ("TCP", bar2DData);
                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 3d);
                bar2DData.add ("214.15.3.161", 7d);
                bar2DData.add ("214.15.3.165", 15d);
                bar2DData.add ("214.15.3.191", 1d);
                seriesData.add ("UDP", bar2DData);
                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 2d);
                bar2DData.add ("214.15.3.161", 5d);
                bar2DData.add ("214.15.3.165", 10d);
                bar2DData.add ("214.15.3.191", 3d);
                seriesData.add ("ICMP", bar2DData);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate By IP and Protocol (5 seconds stats update) for eMIP",
                        "IP and Protocol", "Bandwidth (B/s)", seriesData));

                seriesData = new Bar2DSeriesData (Bar2DSeriesData.ChartType.stack);
                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 60d);
                bar2DData.add ("214.15.3.161", 180d);
                bar2DData.add ("214.15.3.165", 185d);
                bar2DData.add ("214.15.3.191", 10d);
                seriesData.add ("TCP", bar2DData);
                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 10d);
                bar2DData.add ("214.15.3.161", 90d);
                bar2DData.add ("214.15.3.165", 170d);
                bar2DData.add ("214.15.3.191", 25d);
                seriesData.add ("UDP", bar2DData);
                bar2DData = new Bar2DData();
                bar2DData.add ("214.15.3.77", 50d);
                bar2DData.add ("214.15.3.161", 8d);
                bar2DData.add ("214.15.3.165", 105d);
                bar2DData.add ("214.15.3.191", 15d);
                seriesData.add ("ICMP", bar2DData);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate By IP and Protocol (1 minute stats update) for eMIP",
                        "IP and Protocol", "Bandwidth (B/s)", seriesData));

                bar2DData = new Bar2DData();
                bar2DData.add ("TCP", 5d);
                bar2DData.add ("UDP", 3d);
                bar2DData.add ("ICMP", 2d);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate By Protocol (5 seconds stats update)\nfor eMIP from 214.15.3.77",
                        "Protocol", "Bandwidth (B/s)", bar2DData));

                bar2DData = new Bar2DData();
                bar2DData.add ("TCP", 60d);
                bar2DData.add ("UDP", 10d);
                bar2DData.add ("ICMP", 50d);
                chartInfoList.add (new ChartInfo ("Incoming Traffic Rate By Protocol (1 minute stats update)\nfor eMIP from 214.15.3.77",
                        "Protocol", "Bandwidth (B/s)", bar2DData));

                bar2DData = new Bar2DData();
                bar2DData.add ("54915", 305d);
                bar2DData.add ("1947", 0d);
                bar2DData.add ("17500", 185d);
                bar2DData.add ("137", 276d);
                bar2DData.add ("138", 178.20d);
                bar2DData.add ("61117", 17.20d);
                chartInfoList.add (new ChartInfo ("Incoming UDP Traffic Rate By Port (5 seconds stats update)\nfor eMIP from 214.15.3.77",
                        "Port", "Bandwidth (B/s)", bar2DData));

                bar2DData = new Bar2DData();
                bar2DData.add ("54915", 76.25d);
                bar2DData.add ("1947", 1.37d);
                bar2DData.add ("17500", 26.17d);
                bar2DData.add ("137", 90.47d);
                bar2DData.add ("138", 58.05d);
                bar2DData.add ("61117", 4.30d);
                chartInfoList.add (new ChartInfo ("Incoming UDP Traffic Rate By Port (1 minute stats update)\nfor eMIP from 214.15.3.77",
                        "Port", "Bandwidth (B/s)", bar2DData));

                JFXPanel fxPanel = new ChartsPanel().initPanel (chartInfoList);
                JScrollPane pane = new JScrollPane (fxPanel);
                frame.add (pane);
                frame.setSize (600, 850);
                frame.setVisible (true);
                frame.setDefaultCloseOperation (JFrame.DISPOSE_ON_CLOSE);
            }
        });
    }

    private final String _title;
}
