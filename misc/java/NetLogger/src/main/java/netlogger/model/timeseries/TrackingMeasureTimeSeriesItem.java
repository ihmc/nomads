package netlogger.model.timeseries;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;
import java.util.TreeMap;

public class TrackingMeasureTimeSeriesItem extends TimeSeriesItem
{

    public TrackingMeasureTimeSeriesItem (Map<String, String> rowData, long millis) {

        super(millis);

        _row = new TreeMap<>(rowData);
    }

    @Override
    public boolean equals (Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof TrackingMeasureTimeSeriesItem)) {
            return false;
        }
        TrackingMeasureTimeSeriesItem other = (TrackingMeasureTimeSeriesItem) obj;

        return other._row.equals(this._row);
    }

    @Override
    public TrackingMeasureTimeSeriesItem clone () {
        return new TrackingMeasureTimeSeriesItem(_row,
                super.getStoredTime());
    }

    public String getColumnValue (String columnName) {
        return _row.get(columnName);
    }

    private Map<String, String> _row;

    private static final Logger _logger = LoggerFactory.getLogger(TrackingMeasureTimeSeriesItem.class);
}
