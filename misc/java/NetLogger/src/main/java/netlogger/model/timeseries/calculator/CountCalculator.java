package netlogger.model.timeseries.calculator;

import netlogger.model.timeseries.TrackingMeasureTimeSeriesItem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class CountCalculator
{
    public CountCalculator (List<TrackingMeasureTimeSeriesItem> data) {
        _data = data;
    }

    public int calculateTotalWithFilters (Map<String, String> filterValues) {
        return getFilteredList(filterValues).size();
    }

    private List<TrackingMeasureTimeSeriesItem> getFilteredList (Map<String, String> filterValues) {
        List<TrackingMeasureTimeSeriesItem> items = _data;
        for (String key : filterValues.keySet()) {
            String filterValue = filterValues.get(key);
            items = items
                    .stream()
                    .filter(item -> item.getColumnValue(key).equals(filterValue))
                    .collect(Collectors.toList());
        }

        return items;
    }

    private List<TrackingMeasureTimeSeriesItem> _data;
    private static final Logger _logger = LoggerFactory.getLogger(CountCalculator.class);
}
