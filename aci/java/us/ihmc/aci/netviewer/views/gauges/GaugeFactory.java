package us.ihmc.aci.netviewer.views.gauges;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Factory that allowes to create specific <code>Gauge</code> instances
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class GaugeFactory
{
    /**
     * Creates a specific <code>Gauge</code> instance
     * @param type identifier for the <code>Gauge</code> instance type
     * @return the specific <code>Gauge</code> instance corresponding the type
     */
    public static Gauge create (GaugeType type)
    {
        switch (type) {
            case digital_radial:
                return new DigitalRadialGauge();

            case display_rectangular:
                return new DisplayRectangularGauge();

            case linear:
                return new LinearGauge();

            case linear_bar_graph:
                return new LinearBarGraphGauge();

            case radial:
                return new RadialGauge();

            case radial_one_square:
                return new Radial1SquareGauge();

            case radial_one_vertical:
                return new Radial1VerticalGauge();

            case radial_two_top:
                return new RadialTwoTopGauge();

            case radial_bar_graph:
                return new RadialBarGraphGauge();

            case radial_counter:
                return new RadialCounterGauge();

            default:
                log.error ("Unsupported gauge type - Using default value");
                return new RadialGauge();
        }
    }

    private static final Logger log = LoggerFactory.getLogger (GaugeFactory.class);
}
