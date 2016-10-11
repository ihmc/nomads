package us.ihmc.charts.controller;

import javafx.fxml.FXML;
import javafx.scene.chart.AreaChart;
import javafx.scene.chart.NumberAxis;
import us.ihmc.charts.Line2DChart;
import us.ihmc.charts.model.XYData;
import us.ihmc.charts.model.XYValue;
import us.ihmc.charts.util.DateAxis;

import java.util.Date;

/**
 * Controller class for the Area 2D chart where the x axis has data values
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/12/2016
 */
public class Area2DXDateController
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

        _area2DChart.setLegendVisible (false);
        _area2DChart.setCreateSymbols (false);
        _area2DChart.setTitle (title);
        _area2DChart.getStylesheets().add (Line2DChart.class.getResource ("view/Area2DView.css").toExternalForm());

        if (data.getValues() == null) {
            return;
        }

        AreaChart.Series<Date, Number> series = new AreaChart.Series<>();
        for (XYValue xyValue : data.getValues()) {
            series.getData().add (new AreaChart.Data<>((Date) xyValue.x, (Number) xyValue.y));
        }

        _area2DChart.getData().add (series);
    }

    @FXML private AreaChart<Date, Number> _area2DChart;
    @FXML private DateAxis _xAxis;
    @FXML private NumberAxis _yAxis;
}
