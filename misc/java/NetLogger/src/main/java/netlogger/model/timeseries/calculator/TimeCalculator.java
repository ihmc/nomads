package netlogger.model.timeseries.calculator;

import netlogger.model.timeseries.TimeStatistic;
import netlogger.model.timeseries.TrackingMeasureTimeSeriesItem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class TimeCalculator
{

    public TimeCalculator (List<TrackingMeasureTimeSeriesItem> data) {
        _data = data;
    }

    public TimeStatistic getMaxTimeBetweenItemsWithFilters (Map<String, String> filterValues) {
        return calculateMaxTime(getFilteredList(filterValues));
    }

    public TimeStatistic getMinTimeBetweenItemsWithFilters (Map<String, String> filterValues) {
        return calculateMinTime(getFilteredList(filterValues));
    }

    private List<TrackingMeasureTimeSeriesItem> getFilteredList (Map<String, String> filterValues) {
        List<TrackingMeasureTimeSeriesItem> items = new ArrayList<>(_data);
        for (String key : filterValues.keySet()) {
            String filterValue = filterValues.get(key);
            items = items
                    .stream()
                    .filter(item -> item.getColumnValue(key).equals(filterValue))
                    .collect(Collectors.toList());
        }

        return items;
    }

    public Long getTimeSinceLastItemWithFilter (Map<String, String> filterValues) {
        List<TrackingMeasureTimeSeriesItem> items = getFilteredList(filterValues);
        if (items.size() != 0) {
            return System.currentTimeMillis() - items.get(items.size() - 1).getStoredTime();
        }

        return null;
    }

    private TimeStatistic calculateMaxTime (List<TrackingMeasureTimeSeriesItem> items) {
        long max = 0;
        TimeStatistic statistic = null;

        for (int i = 0; i < items.size() - 1; i++) {
            TrackingMeasureTimeSeriesItem currItem = items.get(i);
            TrackingMeasureTimeSeriesItem nextItem = items.get(i + 1);
            long difference = nextItem.getStoredTime() - currItem.getStoredTime();
            if (difference >= max) {
                max = difference;
                statistic = new TimeStatistic(currItem, nextItem, difference);
            }
        }
        return statistic;
    }

    private TimeStatistic calculateMinTime (List<TrackingMeasureTimeSeriesItem> items) {
        long min = Long.MAX_VALUE;
        TimeStatistic statistic = null;

        for (int i = 0; i < items.size() - 1; i++) {
            TrackingMeasureTimeSeriesItem currItem = items.get(i);
            TrackingMeasureTimeSeriesItem nextItem = items.get(i + 1);
            long difference = nextItem.getStoredTime() - currItem.getStoredTime();
            if (difference <= min) {
                min = difference;
                statistic = new TimeStatistic(currItem, nextItem, difference);
            }
        }
        return statistic;
    }

    private List<TrackingMeasureTimeSeriesItem> _data;

    private static final Logger _logger = LoggerFactory.getLogger(TimeCalculator.class);
}
