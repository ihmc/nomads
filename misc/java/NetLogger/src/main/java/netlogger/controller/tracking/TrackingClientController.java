package netlogger.controller.tracking;

import javafx.scene.control.Tooltip;
import netlogger.model.timeseries.TrackingMeasureTimeSeries;
import netlogger.model.tracking.TrackingData;
import netlogger.view.DelayedTooltip;
import netlogger.view.tracking.ModuleView;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Controller for the client-level tracking. Will have a sub controller for the modules within the application
 */
public class TrackingClientController
{
    public TrackingClientController (String application, String clientID, TrackingMeasureTimeSeries timeSeries) {
        _trackingData = new TrackingData(application, clientID);
        _moduleView = new ModuleView(125, 125, clientID);

        final ClientStatisticsManager clientStatisticsManager = new ClientStatisticsManager(clientID);
        clientStatisticsManager.setSeries(timeSeries);

        final DelayedTooltip tooltip = new DelayedTooltip();
        tooltip.setDuration(Integer.MAX_VALUE);
        clientStatisticsManager.setTooltip(tooltip);

        Tooltip.install(_moduleView, tooltip);
        _moduleView.setOnMouseEntered(event -> {
            clientStatisticsManager.setCanRun(true);
            tooltip.setHoveringTargetPrimary(true);
        });
        _moduleView.setOnMouseExited(event -> {
            clientStatisticsManager.setCanRun(false);
            tooltip.setHoveringTargetPrimary(false);
            tooltip.hide();
        });

        tooltip.getScene().setOnMouseEntered(event -> {
            clientStatisticsManager.setCanRun(true);
            tooltip.setHoveringTargetSecondary(true);
        });

        tooltip.getScene().setOnMouseExited(event -> {
            clientStatisticsManager.setCanRun(false);
            tooltip.setHoveringTargetSecondary(false);
            tooltip.hide();
        });

        new Thread(clientStatisticsManager, "Statistics manager").start();
        _moduleView.setStyle("-fx-border-width:1; -fx-border-color:#f00");
    }


    public ModuleView getModuleView () {
        return _moduleView;
    }

    private TrackingData _trackingData;
    private ModuleView _moduleView;


    private static final Logger _logger = LoggerFactory.getLogger(TrackingClientController.class);
}
