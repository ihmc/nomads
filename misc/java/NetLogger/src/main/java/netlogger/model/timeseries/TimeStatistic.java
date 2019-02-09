package netlogger.model.timeseries;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TimeStatistic
{
    public TimeStatistic (TrackingMeasureTimeSeriesItem first, TrackingMeasureTimeSeriesItem second, long time) {
        _first = first;
        _second = second;

        _timeBetween = time;
    }

    public TrackingMeasureTimeSeriesItem getFirst () {
        return _first;
    }

    public TrackingMeasureTimeSeriesItem getSecond () {
        return _second;
    }

    public long getTimeBetween () {
        return _timeBetween;
    }


    private long _timeBetween;
    private TrackingMeasureTimeSeriesItem _first;
    private TrackingMeasureTimeSeriesItem _second;

    private static final Logger _logger = LoggerFactory.getLogger(TimeStatistic.class);
}
