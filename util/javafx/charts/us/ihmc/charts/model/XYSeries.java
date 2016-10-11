package us.ihmc.charts.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Container for series
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class XYSeries
{
    /**
     * Constructor
     * @param name series name
     * @param data series data
     */
    XYSeries (String name, Bar2DData data)
    {
        _name = name;
        _data = data;
    }

    /**
     * Gets the series data size
     * @return the series data size
     */
    int size()
    {
        if (_data == null) {
            return 0;
        }

        return _data.size();
    }

    /**
     * Gets the data x categories
     * @return the data x categories
     */
    List<String> getXCategories()
    {
        if (_data == null) {
            return new ArrayList<>();
        }

        return _data.getXCategories();
    }

    /**
     * Gets the series name
     * @return the series name
     */
    public String getName()
    {
        return _name;
    }

    /**
     * Gets the series data
     * @return the series data
     */
    public Bar2DData getData()
    {
        return _data;
    }

    private final String _name;
    private final Bar2DData _data;
}
