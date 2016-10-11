package us.ihmc.charts.controller;

import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.fxml.FXML;
import javafx.scene.chart.CategoryAxis;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedBarChart;
import us.ihmc.charts.StackedBar2DChart;
import us.ihmc.charts.model.Bar2DSeriesData;
import us.ihmc.charts.model.XYSeries;
import us.ihmc.charts.model.XYValue;

/**
 * Controller class for the Stacked Bar 2D chart
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/12/2016
 */
public class StackedBar2DController
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

        _stackedBar2DChart.setTitle (title);
        _stackedBar2DChart.getStylesheets().add (StackedBar2DChart.class.getResource ("view/StackedBar2DView.css").toExternalForm());

        ObservableList<StackedBarChart.Series> stackedBarChartData = FXCollections.observableArrayList();
        for (XYSeries xySeries : data.getData()) {
            String name = xySeries.getName();
            if (xySeries.getData() == null) {
                continue;
            }
            ObservableList<StackedBarChart.Data<String, Double>> seriesValues = FXCollections.observableArrayList();
            for (XYValue value : xySeries.getData().getValues()) {
                seriesValues.add (new StackedBarChart.Data<>((String) value.x, (Double) value.y));
            }
            stackedBarChartData.add (new StackedBarChart.Series<>(name, seriesValues));
        }

        _stackedBar2DChart.setData (stackedBarChartData);
    }

    @FXML private StackedBarChart _stackedBar2DChart;
    @FXML private CategoryAxis _xAxis;
    @FXML private NumberAxis _yAxis;
}
