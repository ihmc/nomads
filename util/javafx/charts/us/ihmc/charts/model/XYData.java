package us.ihmc.charts.model;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * Container for 2D data
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class XYData implements Data
{
    /**
     * Constructor
     * @param chartType chart type
     */
    public XYData (ChartType chartType)
    {
        _chartType = chartType;
        _values = new ArrayList<>();
    }

    /**
     * Adds a new data where the x value is a <code>Number</code>
     * @param x x value
     * @param y y value
     */
    public void add (Number x, Number y)
    {
        _values.add (new XYValue (x, y));
    }

    /**
     * Adds a new data where the x value is a <code>Date</code>
     * @param x x value
     * @param y y value
     */
    public void add (Date x, Number y)
    {
        _values.add (new XYValue (x, y));
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

    /**
     * Gets the chart type
     * @return the chart type
     */
    public ChartType getChartType()
    {
        return _chartType;
    }


    private final ChartType _chartType;
    private final List<XYValue> _values;

    public enum ChartType
    {
        line_number,
        line_date,
        area_number,
        area_date
    }
}
