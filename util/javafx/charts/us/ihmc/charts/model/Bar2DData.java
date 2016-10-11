package us.ihmc.charts.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Container for Bar2D data
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Bar2DData implements Data
{
    /**
     * Constructor
     */
    public Bar2DData()
    {
        _values = new ArrayList<>();
    }

    /**
     * Adds a new data
     * @param x x value
     * @param y y value
     */
    public void add (String x, Object y)
    {
        _values.add (new XYValue (x, y));
    }

    /**
     * Gets the data x categories
     * @return the data x categories
     */
    public List<String> getXCategories()
    {
        List<String> xValues = new ArrayList<>();
        for (XYValue value : _values) {
            xValues.add ((String) value.x);
        }

        return xValues;
    }

    /**
     * Gets the data size
     * @return the data size
     */
    public int size()
    {
        return _values.size();
    }

    /**
     * Gets the data values
     * @return the data values
     */
    public List<XYValue> getValues()
    {
        return _values;
    }


    private final List<XYValue> _values;
}
