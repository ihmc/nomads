package elasticsearch;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

public class ResistantTimer<T>
{
    public ResistantTimer ()
    {
        _itemsList = new ArrayList<>();
        _maxWaitTime = 5000;
        _maxItemCount = new AtomicLong();

        _maxItemCount.set(11);
    }

    /**
     * Set the wait time for returning 1 item
     * @param wait
     */
    public void setMaxWaitTime(long wait)
    {
        _maxWaitTime = wait;
    }

    public synchronized void setMaxItemCount(long maxItemCount)
    {
        _maxItemCount.set(maxItemCount);
    }

    public synchronized long calculateWaitTime()
    {
        return calculateWaitTime(_itemsList.size());
    }

    private long calculateWaitTime(int itemCount)
    {
        // Formula was calculated for when max time = 5, max count = 11
        // The 6.6 is a resistance factor that may be changed as long as the 13.4665 is changed as well
        //        // Original formula: y = -19 + (5 + 19)/(1+(x/13.6)^6.6) where y is time, x is count

        return (Math.round((-19 + ((24) /
                (1 + (Math.pow(itemCount/(13.4665 * (_maxItemCount.get() / ORIGINAL_PLOTTED_VALUES)), 6.6))))) * 1000))
                * (_maxWaitTime / ORIGINAL_MAX_WAIT_TIME);
    }

    public synchronized void addItem(T item)
    {
        _itemsList.add(item);
    }

    public synchronized List<T> getAndClearItems ()
    {
        ArrayList<T> _items = new ArrayList<>(_itemsList);
        _itemsList.clear();
        return _items;
    }

    public void clearList()
    {
        _itemsList.clear();
    }

    private long _maxWaitTime;
    private AtomicLong _maxItemCount;
    private List<T> _itemsList;

    private static final int ORIGINAL_PLOTTED_VALUES = 11;
    private static final int ORIGINAL_MAX_WAIT_TIME = 5000;


    private static final Logger _logger = LoggerFactory.getLogger(ResistantTimer.class);
}
