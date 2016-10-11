package us.ihmc.charts.model;

/**
 * Container for chart info
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ChartInfo 
{
    /**
     * Constructor
     * @param title chart title
     * @param data chart data
     */
    public ChartInfo (String title, Data data)
    {
        this.title = title;
        this.xLabel = null;
        this.yLabel = null;
        this.zLabel = null;
        this.data = data;
    }

    /**
     * Constructor
     * @param title chart title
     * @param xLabel chart x label
     * @param yLabel chart y label
     * @param data chart data
     */
    public ChartInfo (String title, String xLabel, String yLabel, Data data)
    {
        this.title = title;
        this.xLabel = xLabel;
        this.yLabel = yLabel;
        this.zLabel = null;
        this.data = data;    
    }

    /**
     * Constructor
     * @param title chart title
     * @param xLabel chart x label
     * @param yLabel chart y label
     * @param zLabel chart z label
     * @param data chart data
     */
    public ChartInfo (String title, String xLabel, String yLabel, String zLabel, Data data)
    {
        this.title = title;
        this.xLabel = xLabel;
        this.yLabel = yLabel;
        this.zLabel = zLabel;
        this.data = data;
    }
    
    public final String title;
    public final String xLabel;
    public final String yLabel;
    public final String zLabel;
    public final Data data;
}
