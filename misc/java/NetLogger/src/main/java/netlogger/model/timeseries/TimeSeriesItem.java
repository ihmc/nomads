package netlogger.model.timeseries;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TimeSeriesItem implements Comparable<TimeSeriesItem>, Cloneable
{
    public TimeSeriesItem (long time) {
        _storedTime = time;
    }

    public long getStoredTime () {
        return _storedTime;
    }

    /**
     * Returns an integer indicating the order of this object
     * relative to another object.
     * before == negative, same == zero , after == positive.
     *
     * @param o1 The object being compared to.
     * @return An integer indicating the order of the data item object
     * relative to another object.
     */
    @Override
    public int compareTo (TimeSeriesItem o1) {

        long ourTime = _storedTime;
        long theirTime = ((TimeSeriesItem) o1)._storedTime;
        if (ourTime > theirTime) {
            return 1;
        }
        else if (ourTime == theirTime) {
            return 0;
        }
        else {
            return -1;

        }
    }

    @Override
    public TimeSeriesItem clone () {
        return new TimeSeriesItem(_storedTime);
    }


    /**
     * The time period.
     */
    private long _storedTime;
    private static final Logger _logger = LoggerFactory.getLogger(TimeSeriesItem.class);
}
