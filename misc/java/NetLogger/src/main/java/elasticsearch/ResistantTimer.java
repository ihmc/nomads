package elasticsearch;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

public class ResistantTimer<T>
{
    public ResistantTimer () {
        _itemsList = new ArrayList<>();
        _maxWaitTime = 5000;
        _maxItemCount = new AtomicLong();
        setStartWaitTimeToNow();

        _maxItemCount.set(100);
    }

    /**
     * Set the wait time for returning items
     *
     * @param wait
     */
    public void setMaxWaitTime (long wait) {
        _maxWaitTime = wait;
    }

    private void setStartWaitTimeToNow () {
        _startWaitTime = System.currentTimeMillis();
    }

    public synchronized void setMaxItemCount (long maxItemCount) {
        _maxItemCount.set(maxItemCount);
    }

    public synchronized boolean itemsAreAccessible () {
        return itemsAreAccessible(_itemsList.size());
    }

    private boolean itemsAreAccessible (int itemCount) {
        boolean overItemCount = itemCount >= _maxItemCount.get();
        long timeBetween = (System.currentTimeMillis() - _startWaitTime);
        boolean pastWaitTime = _maxWaitTime < timeBetween;

        return overItemCount || pastWaitTime;
    }

    public synchronized void addItem (T item) {
        _itemsList.add(item);
    }

    public synchronized List<T> getAndClearItems () {
        ArrayList<T> _items = new ArrayList<>(_itemsList);
        _itemsList.clear();
        setStartWaitTimeToNow();
        return _items;
    }

    public void clearList () {
        _itemsList.clear();
    }

    private long _startWaitTime;
    private long _maxWaitTime;
    private AtomicLong _maxItemCount;
    private List<T> _itemsList;


    private static final Logger _logger = LoggerFactory.getLogger(ResistantTimer.class);
}
