package us.ihmc.charts;

import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.layout.AnchorPane;
import us.ihmc.charts.controller.Bar2DController;
import us.ihmc.charts.model.Bar2DData;
import us.ihmc.charts.model.Bar2DSeriesData;

import java.io.IOException;

/**
 * Creates a <code>Parent</code> containing the Bar 2D chart
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Bar2DChart
{
    /**
     * Creates the <code>Parent</code> component containing the chart
     * @param title chart title
     * @param xLabel chart x label
     * @param yLabel chart y label
     * @param data chart data
     * @return the <code>Parent</code> component containing the chart
     * @throws IOException if any problem occur
     */
    public static Parent createChart (String title, String xLabel, String yLabel, Bar2DData data) throws IOException
    {
        FXMLLoader loader = new FXMLLoader (Bar2DChart.class.getResource ("view/Bar2DView.fxml"));
        AnchorPane pane = loader.load();

        Bar2DController controller = loader.getController();
        controller.setData (title, xLabel, yLabel, data);

        return pane;
    }

    /**
     * Creates the <code>Parent</code> component containing the chart
     * @param title chart title
     * @param xLabel chart x label
     * @param yLabel chart y label
     * @param seriesData chart data series
     * @return the <code>Parent</code> component containing the chart
     * @throws IOException if any problem occur
     */
    public static Parent createChart (String title, String xLabel, String yLabel, Bar2DSeriesData seriesData) throws IOException
    {
        FXMLLoader loader = new FXMLLoader (Bar2DChart.class.getResource ("view/Bar2DView.fxml"));
        AnchorPane pane = loader.load();

        Bar2DController controller = loader.getController();
        controller.setData (title, xLabel, yLabel, seriesData);

        return pane;
    }
}
