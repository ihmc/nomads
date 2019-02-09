package us.ihmc.aci.util.chunking;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class Interval extends us.ihmc.chunking.Interval {
    
    public Interval()
    {
        super();
    }
 
    public void read (CommHelper ch) throws CommException, ProtocolException
    {
    	setDimension(Dimension.read (ch));
        setStart(ch.read64());
        setEnd(ch.read64());
    }
    
    @Override
    public Dimension getDimension()
    {
    	return (Dimension) super.getDimension();
    }
    
    public void write (CommHelper ch) throws CommException, ProtocolException
    {
        getDimension().write(ch);
        ch.write64(getStart());
        ch.write64(getEnd());
    }
}

