package netlogger.model.timeseries;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class TrackingMeasureTimeSeries
{

    /**
     * Constructor
     */
    public TrackingMeasureTimeSeries () {
        _data = new ArrayList<>();
    }

    /**
     * Returns the number of items in the series.
     *
     * @return The item count
     */
    public int getItemCount () {
        return _data.size();
    }

    /**
     * Returns the list of data items for the series
     *
     * @return The list of data items
     */
    public List<TrackingMeasureTimeSeriesItem> getItems () {
        return Collections.unmodifiableList(_data);
    }


    /**
     * Returns a data item from the dataset. The returned object
     * is a clone of the item in the series.
     *
     * @param index the item index.
     * @return The data item.
     */
    public TrackingMeasureTimeSeriesItem getDataItem (int index) {
        return _data.get(index).clone();
    }

    /**
     * Returns the milliseconds at the specified index.
     *
     * @param index the index
     * @return The milliseconds
     */
    public long getTimeFor (int index) {
        return getDataItem(index).getStoredTime();
    }

    /**
     * Adds a data item to the series
     *
     * @param item the (timeperiod, value) pair (<code>null</code> not
     *             permitted).
     */
    public int add (TrackingMeasureTimeSeriesItem item) {
        item = item.clone();

        int count = getItemCount();
        if (count == 0) {
            _data.add(item);
            return _data.size() - 1;
        }
        else {
            // If this item's time is the same or > the most recent item, add it to the end of our list
            if (item.compareTo(getDataItem(getItemCount() - 1)) >= 0) {
                _data.add(item);
                return _data.size() - 1;
            }
            // Otherwise insert it into the correct spot
            // TODO: not sure if this finds the first value in the list or the last value in the list with this time, but it should insert after the last
            else {
                int index = Collections.binarySearch(_data, item);
                if (index < 0) {
                    index = -index - 1;
                }
                _data.add(index, item);
                return index;
            }
        }
    }


    /**
     * Removes all data items from the series
     */
    public void clear () {
        if (_data.size() > 0) {
            _data.clear();
        }
    }

    /**
     * Deletes the data items for the given value
     *
     * @param val the period of the item to delete (<code>null</code> not permitted).
     */
    public void delete (TrackingMeasureTimeSeriesItem val) {
        _data.remove(val);
    }

    /**
     * Deletes data from startTimeline until end index (end inclusive).
     *
     * @param start the index of the first period to delete.
     * @param end   the index of the last period to delete.
     */
    public void delete (int start, int end) {
        if (end < start) {
            throw new IllegalArgumentException("Requires startTimeline <= end.");
        }

        for (int i = 0; i <= (end - start); i++) {
            _data.remove(start);
        }
    }


    private List<TrackingMeasureTimeSeriesItem> _data;
    private static final Logger _logger = LoggerFactory.getLogger(TrackingMeasureTimeSeries.class);
}
