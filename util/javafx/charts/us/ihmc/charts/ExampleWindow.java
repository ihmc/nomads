package us.ihmc.charts;

import eu.hansolo.enzo.common.Section;
import eu.hansolo.enzo.gauge.SimpleGauge;
import javafx.application.Application;
import javafx.geometry.Orientation;
import javafx.scene.Scene;
import javafx.scene.layout.FlowPane;
import javafx.scene.paint.Paint;
import javafx.stage.Stage;
import us.ihmc.charts.model.Bar2DData;
import us.ihmc.charts.model.Bar2DSeriesData;
import us.ihmc.charts.model.XYData;

import java.io.IOException;
import java.util.GregorianCalendar;

/**
 * Creates the javafx application main window
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ExampleWindow extends Application
{
    @Override
    public void start (Stage stage)
    {
        _primaryStage = stage;

        try {
            FlowPane root = new FlowPane (Orientation.HORIZONTAL);
            root.setPrefWrapLength(100);

            // Bar 2D chart
            Bar2DData data = new Bar2DData();
            data.add ("1.2.3.4", 100d);
            data.add ("11.22.33.44", 200d);
            data.add ("111.222.333.444", 300d);
            data.add("1.2.3.5", 50d);
            root.getChildren().add (Bar2DChart.createChart ("Traffic by IP", "IP", "y axis", data));

            // Bar 2D chart with series and bars next to each other
            Bar2DSeriesData seriesData = new Bar2DSeriesData (Bar2DSeriesData.ChartType.linear);
            data = new Bar2DData();
            data.add ("1.2.3.4", 567d);
            data.add ("11.22.33.44", 1292d);
            data.add ("111.222.333.444", 1292d);
            seriesData.add ("TCP", data);
            data = new Bar2DData();
            data.add ("1.2.3.4", 956d);
            data.add ("11.22.33.44", 1665d);
            data.add ("111.222.333.444", 2559d);
            seriesData.add ("UDP", data);
            data = new Bar2DData();
            data.add ("1.2.3.4", 1154d);
            data.add ("11.22.33.44", 1927d);
            data.add ("111.222.333.444", 2774d);
            seriesData.add("ICMP", data);
            root.getChildren().add (Bar2DChart.createChart ("Traffic", "years", "y axis", seriesData));

            // Bar 2D chart with series and stacked bars
            seriesData = new Bar2DSeriesData (Bar2DSeriesData.ChartType.stack);
            data = new Bar2DData();
            data.add ("214.15.3.77", 567d);
            data.add ("214.15.3.161", 1292d);
            data.add ("214.15.3.165", 1292d);
            data.add ("214.15.3.191", 2250d);
            seriesData.add ("TCP", data);
            data = new Bar2DData();
            data.add ("214.15.3.77", 956d);
            data.add ("214.15.3.161", 1665d);
            data.add ("214.15.3.165", 2559d);
            data.add ("214.15.3.191", 870d);
            seriesData.add ("UDP", data);
            data = new Bar2DData();
            data.add ("214.15.3.77", 1154d);
            data.add ("214.15.3.161", 1927d);
            data.add ("214.15.3.165", 2774d);
            data.add ("214.15.3.191", 890d);
            seriesData.add ("ICMP", data);
            root.getChildren().add (StackedBar2DChart.createChart ("Incoming Traffic Rate By IP and Protocol",
                    "IP and Protocol", "Bandwidth (B/s)", seriesData));

            // XY area chart with dates on the x axis
            XYData xyData = new XYData (XYData.ChartType.area_date);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 25).getTime(), 1);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 30).getTime(), 19);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 35).getTime(), 18);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 40).getTime(), 25);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 45).getTime(), 6);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 50).getTime(), 30);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 11, 55).getTime(), 35);
            xyData.add (new GregorianCalendar (2016, 0, 12, 14, 12, 0).getTime(), 30);
            root.getChildren().add (Line2DChart.createXDateChart ("Incoming Traffic Rate (5 seconds stats update)",
                    "Time", "Bandwidth (B/s)", xyData));

            // XY area chart with numbers on the x axis
            xyData = new XYData (XYData.ChartType.area_number);
            xyData.add (1, 10);
            xyData.add (2, 5);
            xyData.add (3, 8);
            xyData.add (4, 1);
            xyData.add (5, 19);
            xyData.add (6, 18);
            xyData.add (7, 25);
            xyData.add (8, 6);
            xyData.add (9, 30);
            root.getChildren().add (Area2DChart.createXNumberChart ("ACME Company Stock", "Time", "Share Price", xyData));

            // XY line chart with numbers on the x axis
            xyData = new XYData (XYData.ChartType.line_number);
            xyData.add (1, 10);
            xyData.add (2, 5);
            xyData.add (3, 8);
            xyData.add (4, 1);
            xyData.add (5, 19);
            xyData.add (6, 18);
            xyData.add (7, 25);
            xyData.add (8, 6);
            xyData.add (9, 30);
            root.getChildren().add (Line2DChart.createXNumberChart ("ACME Company Stock", "Time", "Share Price", xyData));

            // Gauge chart
            SimpleGauge gauge = new SimpleGauge();
            gauge.setStyle (SimpleGauge.STYLE_CLASS_GREEN_TO_RED_10);
            Section[] sections = new Section[3];
            sections[0] = new Section (0, 30);
            sections[1] = new Section (30, 70);
            sections[2] = new Section (70, 100);
            gauge.setSections (sections);
            gauge.setSectionFill0 (Paint.valueOf ("#0cd943"));
            gauge.setSectionFill1 (Paint.valueOf ("#ffa500"));
            gauge.setSectionFill2 (Paint.valueOf ("#d90c3b"));
            gauge.setValue (35);

            root.getChildren().add (gauge);

            Scene scene = new Scene (root);
            _primaryStage.setScene (scene);
            _primaryStage.show();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }

    /**
     * Used when the window is created from some other class
     */
    public void initWindow()
    {
        launch();
    }

    private Stage _primaryStage;
    boolean isInit = false;
}
