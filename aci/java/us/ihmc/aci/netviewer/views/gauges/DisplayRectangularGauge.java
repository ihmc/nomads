package us.ihmc.aci.netviewer.views.gauges;

import eu.hansolo.steelseries.gauges.DisplayRectangular;
import eu.hansolo.steelseries.tools.BackgroundColor;
import eu.hansolo.steelseries.tools.ColorDef;

import java.awt.*;

/**
 * Creates an instance of the <code>DisplayRectangular</code> gauge
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class DisplayRectangularGauge implements Gauge
{
    @Override
    public Component getComponent()
    {
        return _gauge;
    }

    @Override
    public void setTitle (String title)
    {
        if (title == null) {
            return;
        }

        _gauge.setTitle (title);
    }

    @Override
    public void setUnitString (String unit)
    {
        if (unit == null) {
            return;
        }

        _gauge.setUnitString (unit);
    }

    @Override
    public void setBackgroundColor (BackgroundColor color)
    {
        if (color == null) {
            return;
        }

        _gauge.setBackgroundColor (color);
    }

    @Override
    public void setTrackStart (double value)
    {
        _gauge.setTrackStart(value);
    }

    @Override
    public void setTrackStop (double value)
    {
        _gauge.setTrackStop(value);
    }

    @Override
    public void setTrackSection (double value)
    {
        _gauge.setTrackSection(value);
    }

    @Override
    public void setTrackStartColor (Color value)
    {
        if (value == null) {
            return;
        }

        _gauge.setTrackStartColor(value);
    }

    @Override
    public void setTrackStopColor (Color value)
    {
        if (value == null) {
            return;
        }

        _gauge.setTrackStopColor(value);
    }

    @Override
    public void setTrackSectionColor (Color value)
    {
        if (value == null) {
            return;
        }

        _gauge.setTrackSectionColor(value);
    }

    @Override
    public void setTrackVisible (boolean value)
    {
        _gauge.setTrackVisible(value);
    }

    @Override
    public void setThresholdVisible (boolean value)
    {
        _gauge.setThresholdVisible(value);
    }

    @Override
    public void setThreshold (double value)
    {
        _gauge.setThreshold(value);
    }

    @Override
    public void setThresholdColor (ColorDef value)
    {
        if (value == null) {
            return;
        }

        _gauge.setThresholdColor(value);
    }

    @Override
    public void setMinValue (double value)
    {
        _gauge.setMinValue(value);
    }

    @Override
    public void setMaxValue (double value)
    {
        _gauge.setMaxValue(value);
    }

    @Override
    public double getMinValue()
    {
        return _gauge.getMinValue();
    }

    @Override
    public double getMaxValue()
    {
        return _gauge.getMaxValue();
    }

    @Override
    public double getThreshold()
    {
        return _gauge.getThreshold();
    }

    @Override
    public void setValueAnimated (double value)
    {
        _gauge.setValueAnimated (value);
    }

    @Override
    public double getMinMeasuredValue()
    {
        return _gauge.getMinMeasuredValue();
    }

    @Override
    public double getMaxMeasuredValue()
    {
        return _gauge.getMaxMeasuredValue();
    }

    private final DisplayRectangular _gauge = new DisplayRectangular();
}
