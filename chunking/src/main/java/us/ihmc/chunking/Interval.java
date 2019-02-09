package us.ihmc.chunking;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class Interval {
    
    private Dimension _dimension;
    private long _start;
    private long _end;

    public Interval()
    {
        this (0, 0);
    }
 
    public Interval (long start, long end)
    {
        _start = start;
        _end = end;
    }


    public long getEnd()
    {
        return _end;
    }

    public void setEnd(long end) 
    {
    	_end = end;
    }
    
    public long getStart()
    {
        return _start;
    }
    
    public void setStart (long start) 
    {
    	_start = start;
    }
    
    public Dimension getDimension() 
    {
    	return _dimension;
    }
    
    public void setDimension(Dimension dimension) 
    {
    	_dimension = dimension;
    }
}

