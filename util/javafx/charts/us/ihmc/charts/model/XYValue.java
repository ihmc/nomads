package us.ihmc.charts.model;

/**
 * Container for xy pairs
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class XYValue
{
    /**
     * Constructor
     * @param x x value
     * @param y y value
     */
    public XYValue (Object x, Object y)
    {
        this.x = x;
        this.y = y;
    }

    public final Object x;
    public final Object y;
}
