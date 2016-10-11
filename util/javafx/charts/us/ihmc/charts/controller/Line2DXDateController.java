package us.ihmc.charts.controller;

import javafx.fxml.FXML;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import us.ihmc.charts.Line2DChart;
import us.ihmc.charts.model.XYData;
import us.ihmc.charts.model.XYValue;
import us.ihmc.charts.util.DateAxis;

import java.util.Date;

/**
 * Controller class for the Line 2D chart where the x axis has date values
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Line2DXDateController
{
    /**
     * Initializes the controller class. This method is automatically called
     * after the fxml file has been loaded.
     */
    @FXML
    private void initialize()
    {
    }

    /**
     * Sets the data in the chart when there is only one data series
     * @param title chart title
     * @param xLabel chart x label
     * @param yLabel chart y label
     * @param data chart data
     */
    public void setData (String title, String xLabel, String yLabel, XYData data)
    {
        if (data == null) {
            return;
        }

        _yAxis.setLabel (yLabel);
        _xAxis.setLabel (xLabel);

        _line2DChart.setLegendVisible (false);
        _line2DChart.setCreateSymbols (false);
        _line2DChart.setTitle (title);
        _line2DChart.getStylesheets().add (Line2DChart.class.getResource ("view/Line2DView.css").toExternalForm());

        if (data.getValues() == null) {
            return;
        }

        XYChart.Series<Date, Number> series = new XYChart.Series<>();
        for (XYValue xyValue : data.getValues()) {
            series.getData().add (new XYChart.Data<>((Date) xyValue.x, (Number) xyValue.y));
        }

        _line2DChart.getData().add (series);
    }

    @FXML private LineChart<Date, Number> _line2DChart;
    @FXML private DateAxis _xAxis;
    @FXML private NumberAxis _yAxis;
}
