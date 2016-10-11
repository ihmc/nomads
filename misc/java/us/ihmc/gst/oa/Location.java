package us.ihmc.gst.oa;

import us.ihmc.gst.util.GeoCoordinates;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public class Location
{
    Location()
    {
        _latitude = (new Double (GeoCoordinates.UNSET_LAT)).floatValue();
        _longitude = (new Double (GeoCoordinates.UNSET_LONG)).floatValue();
    }

    public float _latitude;
    public float _longitude;
}
