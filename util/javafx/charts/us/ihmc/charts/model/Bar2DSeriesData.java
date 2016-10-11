package us.ihmc.charts.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Container for Bar2D series data
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Bar2DSeriesData implements Data
{
    /**
     * Constructor
     * @param chartType chart type
     */
    public Bar2DSeriesData (ChartType chartType)
    {
        _chartType = chartType;
        _values = new ArrayList<>();
    }

    /**
     * Adds a new data series
     * @param series series name
     * @param data series data
     */
    public void add (String series, Bar2DData data)
    {
        _values.add (new XYSeries (series, data));
    }

    /**
     * Gets the data x categories
     * @return the data x categories
     */
    public List<String> getXCategories()
    {
        if (_values.size() == 0) {
            return new ArrayList<>();
        }

        return _values.get (0).getXCategories();   // TODO: we are assuming the data are correct and this method would give the same results for all the series
    }

    /**
     * Gets the series data
     * @return the series data
     */
    public List<XYSeries> getData()
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
    private final List<XYSeries> _values;

    public enum ChartType
    {
        linear,
        stack
    }
}
