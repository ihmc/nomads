package us.ihmc.charts;

import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.layout.AnchorPane;
import us.ihmc.charts.controller.Area2DXDateController;
import us.ihmc.charts.controller.Area2DXNumberController;
import us.ihmc.charts.model.XYData;

import java.io.IOException;

/**
 * Creates a <code>Parent</code> containing the Area 2D chart
 * @author Rita Lenzi (rlenzi@ihmc.us) - 1/12/2016
 */
public class Area2DChart
{
    public static Parent createXNumberChart (String title, String xLabel, String yLabel, XYData data) throws IOException
    {
        FXMLLoader loader = new FXMLLoader (Line2DChart.class.getResource ("view/Area2DXNumberView.fxml"));
        AnchorPane pane = loader.load();

        Area2DXNumberController controller = loader.getController();
        controller.setData (title, xLabel, yLabel, data);

        return pane;
    }

    public static Parent createXDateChart (String title, String xLabel, String yLabel, XYData data) throws IOException
    {
        FXMLLoader loader = new FXMLLoader (Line2DChart.class.getResource ("view/Area2DXDateView.fxml"));
        AnchorPane pane = loader.load();

        Area2DXDateController controller = loader.getController();
        controller.setData (title, xLabel, yLabel, data);

        return pane;
    }
}
