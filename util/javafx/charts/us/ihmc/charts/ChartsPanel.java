package us.ihmc.charts;

import javafx.application.Platform;
import javafx.embed.swing.JFXPanel;
import javafx.geometry.Orientation;
import javafx.scene.Scene;
import javafx.scene.layout.FlowPane;
import us.ihmc.charts.model.*;

import java.io.IOException;
import java.util.List;

/**
 * Creates the container for the charts
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ChartsPanel
{
    /**
     * Used when the application is created from some other class that wants to retrieve the <code>JFXPanel</code>
     * containing the charts
     * @param chartInfoList list of <code>ChartInfo</code> instance containing the info and value for the charts
     * @return the <code>JFXPanel</code> containing the charts
     */
    public JFXPanel initPanel (List<ChartInfo> chartInfoList)
    {
        if (chartInfoList == null) {
            return null;
        }

        final JFXPanel fxPanel = new JFXPanel();

        try {
            final FlowPane root = new FlowPane (Orientation.HORIZONTAL);
            root.setPrefWrapLength (100);

            for (ChartInfo chartInfo : chartInfoList) {
                if (chartInfo.data instanceof  Bar2DData) {
                    root.getChildren().add (Bar2DChart.createChart (chartInfo.title, chartInfo.xLabel, chartInfo.yLabel,
                            (Bar2DData) chartInfo.data));
                }
                else if (chartInfo.data instanceof Bar2DSeriesData) {
                    switch (((Bar2DSeriesData) chartInfo.data).getChartType()) {
                        case linear:
                            root.getChildren().add (Bar2DChart.createChart (chartInfo.title, chartInfo.xLabel, chartInfo.yLabel,
                                    (Bar2DSeriesData) chartInfo.data));
                            break;

                        case stack:
                            root.getChildren().add (StackedBar2DChart.createChart (chartInfo.title, chartInfo.xLabel,
                                    chartInfo.yLabel, (Bar2DSeriesData) chartInfo.data));
                            break;
                    }

                }
                else if (chartInfo.data instanceof XYData) {
                    switch (((XYData) chartInfo.data).getChartType()) {
                        case line_number:
                            root.getChildren().add (Line2DChart.createXNumberChart (chartInfo.title, chartInfo.xLabel,
                                    chartInfo.yLabel, (XYData) chartInfo.data));
                            break;

                        case line_date:
                            root.getChildren().add (Line2DChart.createXDateChart (chartInfo.title, chartInfo.xLabel,
                                    chartInfo.yLabel, (XYData) chartInfo.data));
                            break;

                        case area_number:
                            root.getChildren().add (Area2DChart.createXNumberChart (chartInfo.title, chartInfo.xLabel,
                                    chartInfo.yLabel, (XYData) chartInfo.data));
                            break;

                        case area_date:
                            root.getChildren().add (Area2DChart.createXDateChart (chartInfo.title, chartInfo.xLabel,
                                    chartInfo.yLabel, (XYData) chartInfo.data));
                            break;
                    }
                }
                else {
                    System.err.println ("Wrong data instance");
                }
            }

            Platform.runLater (new Runnable()
            {
                @Override
                public void run()
                {
                    Scene scene = new Scene (root);
                    fxPanel.setScene (scene);
                    isInit = true;
                }
            });
        }
        catch (IOException e) {
            e.printStackTrace();
            return null;
        }

        while (!isInit) {
            try {
                Thread.sleep (10);
            }
            catch (InterruptedException e) {
                e.printStackTrace();
                return null;
            }
        }

        return fxPanel;
    }

    boolean isInit = false;
}
