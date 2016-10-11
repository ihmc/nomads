package us.ihmc.gst.util;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class GeoCoordinates
{
    public static final double MAX_LAT = 90.0d;
    public static final double MIN_LAT = -MAX_LAT;
    public static final double MAX_LONG = 180.0d;
    public static final double MIN_LONG = -MAX_LONG;

    public static final double UNSET_LAT = MAX_LAT + 1;
    public static final double UNSET_LONG = MAX_LONG + 1;

    public static boolean isValidLatitude (double lat)
    {
        return !(lat < MIN_LAT || lat > MAX_LAT);
    }

    public static boolean isValidLongitude (double lon)
    {
        return !(lon < MIN_LONG || lon > MAX_LONG);
    }
}
