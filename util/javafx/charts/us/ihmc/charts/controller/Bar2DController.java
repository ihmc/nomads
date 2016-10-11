package us.ihmc.charts.controller;

import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.chart.BarChart;
import javafx.scene.chart.CategoryAxis;
import javafx.scene.chart.NumberAxis;
import us.ihmc.charts.Bar2DChart;
import us.ihmc.charts.model.Bar2DData;
import us.ihmc.charts.model.Bar2DSeriesData;
import us.ihmc.charts.model.XYSeries;
import us.ihmc.charts.model.XYValue;

/**
 * Controller class for the Bar 2D chart
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Bar2DController
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
    public void setData (String title, String xLabel, String yLabel, Bar2DData data)
    {
        if (data == null) {
            return;
        }

        _yAxis.setLabel (yLabel);

        _xAxis.setLabel (xLabel);
        _xAxis.setCategories (FXCollections.observableArrayList (data.getXCategories()));

        _bar2DChart.setLegendVisible (false);
        _bar2DChart.setTitle (title);

        if (data.getValues() == null) {
            return;
        }

//        XYChart.Series<String, Double> series = new XYChart.Series<>();
        BarChart.Series<String, Double> series = new BarChart.Series<>();
        for (XYValue xyValue : data.getValues()) {
//            series.getData().add (new XYChart.Data<>(xyValue.x, (Double) xyValue.y));
            series.getData().add (new BarChart.Data<>((String) xyValue.x, (Double) xyValue.y));
        }

        _bar2DChart.getData().add (series);
    }

    /**
     * Sets the data in the chart when there are more data series
     * @param title chart title
     * @param xLabel chart x label
     * @param yLabel chart y label
     * @param data chart data
     */
    public void setData (String title, String xLabel, String yLabel, Bar2DSeriesData data)
    {
        if (data == null) {
            return;
        }

        _yAxis.setLabel(yLabel);

        _xAxis.setLabel (xLabel);
        _xAxis.setCategories (FXCollections.observableArrayList (data.getXCategories()));

        _bar2DChart.setTitle (title);
        _bar2DChart.getStylesheets().add (Bar2DChart.class.getResource ("view/Bar2DView.css").toExternalForm());

        ObservableList<BarChart.Series> barChartData = FXCollections.observableArrayList();
        for (XYSeries xySeries : data.getData()) {
            String name = xySeries.getName();
            if (xySeries.getData() == null) {
                continue;
            }
            ObservableList<BarChart.Data<String, Double>> seriesValues = FXCollections.observableArrayList();
            for (XYValue value : xySeries.getData().getValues()) {
                seriesValues.add (new BarChart.Data<>((String) value.x, (Double) value.y));
            }
            barChartData.add (new BarChart.Series<>(name, seriesValues));
        }

        _bar2DChart.setData (barChartData);
    }

    @FXML private BarChart _bar2DChart;
    @FXML private CategoryAxis _xAxis;
    @FXML private NumberAxis _yAxis;
}