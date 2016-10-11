package us.ihmc.charts;

import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import javafx.scene.layout.AnchorPane;
import us.ihmc.charts.controller.Line2DXDateController;
import us.ihmc.charts.controller.Line2DXNumberController;
import us.ihmc.charts.model.XYData;

import java.io.IOException;

/**
 * Creates a <code>Parent</code> containing the Line 2D chart
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/12/2016
 */
public class Line2DChart
{
    public static Parent createXNumberChart (String title, String xLabel, String yLabel, XYData data) throws IOException
    {
        FXMLLoader loader = new FXMLLoader (Line2DChart.class.getResource ("view/Line2DXNumberView.fxml"));
        AnchorPane pane = loader.load();

        Line2DXNumberController controller = loader.getController();
        controller.setData (title, xLabel, yLabel, data);

        return pane;
    }

    public static Parent createXDateChart (String title, String xLabel, String yLabel, XYData data) throws IOException
    {
        FXMLLoader loader = new FXMLLoader (Line2DChart.class.getResource ("view/Line2DXDateView.fxml"));
        AnchorPane pane = loader.load();

        Line2DXDateController controller = loader.getController();
        controller.setData (title, xLabel, yLabel, data);

        return pane;
    }

    public static Parent createChart (String title, String xLabel, String yLabel) throws IOException
    {
        double hours = 0;
        double minutes = 0;
        double timeInHours = 0;
        double prevY = 10;
        double y = 10;

        NumberAxis xAxis = new NumberAxis(0, 24, 3);
        final NumberAxis yAxis = new NumberAxis(0, 100, 10);
        LineChart<Number, Number> chart = new LineChart<>(xAxis, yAxis);
        // setup chart
        chart.getStylesheets().add(Line2DChart.class.getResource ("view/Line2DView.css").toExternalForm());
        chart.setCreateSymbols(false);
        chart.setAnimated(false);
        chart.setLegendVisible(false);
        chart.setTitle (title);
        xAxis.setLabel (xLabel);
        xAxis.setForceZeroInRange(false);
        yAxis.setLabel (yLabel);
        yAxis.setTickLabelFormatter(new NumberAxis.DefaultFormatter(yAxis, "$", null));

        // add starting data
        XYChart.Series<Number, Number> hourDataSeries = new XYChart.Series<>();
        hourDataSeries.setName("Hourly Data");
        XYChart.Series<Number, Number> minuteDataSeries = new XYChart.Series<>();
        minuteDataSeries.setName("Minute Data");

        // create some starting data
        hourDataSeries.getData().add(new XYChart.Data<Number, Number>(timeInHours, prevY));
        minuteDataSeries.getData().add(new XYChart.Data<Number, Number>(timeInHours, prevY));
        for (double m = 0; m < (60); m++) {
            if (minutes == 59) {
                hours++;
                minutes = 0;
            } else {
                minutes++;
            }
            timeInHours = hours + ((1d / 60d) * minutes);

            if ((timeInHours % 1) == 0) {
                // change of hour
                double oldY = y;
                y = prevY - 10 + (Math.random() * 20);
                prevY = oldY;
                while (y < 10 || y > 90) {
                    y = y - 10 + (Math.random() * 20);
                }
                hourDataSeries.getData()
                        .add(new XYChart.Data<Number, Number>(timeInHours, prevY));
                // after 25hours delete old data
                if (timeInHours > 25) {
                    hourDataSeries.getData().remove(0);
                }
                // every hour after 24 move range 1 hour
                if (timeInHours > 24) {
                    xAxis.setLowerBound(xAxis.getLowerBound() + 1);
                    xAxis.setUpperBound(xAxis.getUpperBound() + 1);
                }
            }
            double min = (timeInHours % 1);
            double randomPickVariance = Math.random();
            if (randomPickVariance < 0.3) {
                double minY = prevY + ((y - prevY) * min) - 4 + (Math.random() * 8);
                minuteDataSeries.getData().add(new XYChart.Data<Number, Number>(timeInHours, minY));
            } else if (randomPickVariance < 0.7) {
                double minY = prevY + ((y - prevY) * min) - 6 + (Math.random() * 12);
                minuteDataSeries.getData().add(new XYChart.Data<Number, Number>(timeInHours, minY));
            } else if (randomPickVariance < 0.95) {
                double minY = prevY + ((y - prevY) * min) - 10 + (Math.random() * 20);
                minuteDataSeries.getData().add(new XYChart.Data<Number, Number>(timeInHours, minY));
            } else {
                double minY = prevY + ((y - prevY) * min) - 15 + (Math.random() * 30);
                minuteDataSeries.getData().add(new XYChart.Data<Number, Number>(timeInHours, minY));
            }
            // after 25hours delete old data
            if (timeInHours > 25) {
                minuteDataSeries.getData().remove(0);
            }
        }
        chart.getData().add(minuteDataSeries);
        chart.getData().add(hourDataSeries);
        return chart;
    }
}
