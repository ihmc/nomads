package us.ihmc.aci.netviewer.views.gauges;

import eu.hansolo.steelseries.tools.BackgroundColor;

import java.awt.*;

/**
 * Interface to expose methods commons to all the gauge instances
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface Gauge
{
    /**
     * Returns the <code>Component</code> that can be added to a panel
     * @return the <code>Component</code> that can be added to a panel
     */
    Component getComponent();

    /**
     * Sets the title of the gauge
     * @param title gauge title
     */
    void setTitle (String title);

    /**
     * Sets the unit string of the gauge
     * @param unit the unit string of the gauge
     */
    void setUnitString (String unit);

    /**
     * Sets the background color of the gauge.
     * The background color is not a standard color but more a color scheme with colors and a gradient.
     * The typical background color is DARK_GRAY.
     * @param color background color of the gauge
     */
    void setBackgroundColor (BackgroundColor color);

    /**
     * Sets the value where the track will start.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors
     * @param value track start value
     */
    void setTrackStart (double value);

    /**
     * Sets the value of the end of the track.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors
     * @param value track stop value
     */
    void setTrackStop (double value);

    /**
     * Sets the value of the point between trackStart and trackStop.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors
     * @param value track section value
     */
    void setTrackSection (double value);

    /**
     * Sets the color of the point where the track will start.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors
     * @param value track start color
     */
    void setTrackStartColor (java.awt.Color value);

    /**
     * Sets the color of the point where the track will stop.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors
     * @param value track stop color
     */
    void setTrackStopColor (java.awt.Color value);

    /**
     * Sets the color of the value between trackStart and trackStop.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors
     * @param value color of the value between trackStart and trackStop
     */
    void setTrackSectionColor (java.awt.Color value);

    /**
     * Sets the visibility of the track.
     * The track is a area that could be defined by a start value, a section stop value. This area will be painted with a
     * gradient that uses two or three given colors.
     * @param value visibility of the track
     */
    void setTrackVisible (boolean value);

    /**
     * Sets the visibility of the threshold indicator.
     * The value of the threshold will be visualized by a small red triangle that points on the threshold value.
     * @param value visibility of the threshold indicator
     */
    void setThresholdVisible (boolean value);

    /**
     * Sets the given value as the threshold.
     * If the current value of the gauge exceeds this threshold, a event will be fired and the led will
     * start blinking (if the led is visible).
     * @param value threshold
     */
    void setThreshold (double value);

    /**
     * Sets the color of the threshold indicator
     * @param value threshold indicator color
     */
    void setThresholdColor (eu.hansolo.steelseries.tools.ColorDef value);

    /**
     * Sets the minimum value of the measurement
     * range of this gauge. This value defines the
     * minimum value the gauge could display.
     * @param value min value
     */
    void setMinValue (double value);

    /**
     * Sets the maximum value of the measurement range of this gauge. This value defines the
     * maximum value the gauge could display.
     * @param value max value
     */
    void setMaxValue (double value);

    /**
     * Returns the minimum value of the measurement range of this gauge
     * @return a double representing the min value the gauge could visualize
     */
    double getMinValue();

    /**
     * Returns the maximum value of the measurement range of this gauge
     * @return a double representing the max value the gauge could visualize
     */
    double getMaxValue();

    /**
     * Returns the value that is defined as a threshold
     * @return the threshold value where the led starts blinking
     */
    double getThreshold();

    /**
     * Sets the value in the gauge by animating it
     * @param value new value
     */
    void setValueAnimated (double value);

    /**
     * Returns the lowest measured value.
     * On every move of the bar/pointer the lowest value will be stored in the minMeasuredValue variable.
     * @return a double representing the min measure value
     */
    double getMinMeasuredValue();

    /**
     * Returns the biggest measured value.
     * On every move of the bar/pointer the biggest value will be stored in the maxMeasuredValue variable.
     * @return a double representing the max measured value
     */
    double getMaxMeasuredValue();
}
