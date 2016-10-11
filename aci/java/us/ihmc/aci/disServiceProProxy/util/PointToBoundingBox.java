package us.ihmc.aci.disServiceProProxy.util;

/**
 * PointToBoundingBox.java
 *
 * @author Giacomo Benincasa (gbenincasa@ihmc.us), Enrico Casini (ecasini@ihmc.us)
 */
public class PointToBoundingBox
{
    private static  double _padding = 0.00001;
    private double _lat;
    private double _lon;

    public PointToBoundingBox (double lat, double lon)
    {
        _lat = lat;
        _lon = lon;
    }

    public PointToBoundingBox (double lat, double lon, double padding)
    {
        _lat = lat;
        _lon = lon;
        _padding = padding;
    }

    public float getLefUpperLatitude ()
    {
        return new Float (_lat + _padding);
    }

    public float getLefUpperLongitude ()
    {
        return new Float (_lon - _padding);
    }

    public float getRightLowerLatitude ()
    {
        return new Float (_lat - _padding);
    }

    public float getRightLowerLongitude ()
    {
        return new Float (_lon + _padding);
    }

    public float getRightUpperLatitude ()
    {
        return new Float(_lat + _padding);
    }

    public float getRightUpperLongitude()
    {
        return new Float(_lon + _padding);
    }

    public float getLeftLowerLatitude()
    {
        return new Float(_lat - _padding);
    }

    public float getLeftLowerLongitude()
    {
        return new Float(_lon - _padding);
    }
}
