package netlogger.controller.tracking;

import netlogger.model.timeseries.TrackingMeasureTimeSeries;
import netlogger.model.tracking.statistics.GeneralStatistics;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class StatisticsManager
{
    public void setSeries (TrackingMeasureTimeSeries series) {
        _series = series;
    }

    public abstract GeneralStatistics calculateGeneralStatistics ();

    protected TrackingMeasureTimeSeries _series;
    private static final Logger _logger = LoggerFactory.getLogger(StatisticsManager.class);
}
