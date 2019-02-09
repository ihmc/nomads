package us.ihmc.chunking;

import java.util.Collection;
import java.util.LinkedList;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class AnnotationWrapper {

    public final Collection<Interval> _intervals = new LinkedList<>();
    private byte[] _data;

    public AnnotationWrapper()
    {
        super();
    }

    public AnnotationWrapper(byte[] data, Collection<Interval> intervals) {
        _data = data;
        _intervals.addAll(intervals);
    }

    public byte[] getData()
    {
        return _data;
    }

    public void setData(byte[] data)
    {
    	_data = data;
    }
    
    
    public Collection<Interval> getIntervals()
    {
        return _intervals;
    }

}
