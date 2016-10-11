package us.ihmc.gst.util;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class BoundingBox
{
    private final double _minLat;
    private final double _minLong;
    private final double _maxLat;
    private final double _maxLong;

    public BoundingBox (double minLat, double minLong, double maxLat, double maxLong)
    {
        _minLat = minLat;
        _minLong = minLong;
        _maxLat = maxLat;
        _maxLong = maxLong;
    }

    public double getLeftUpperLatitude()
    {
        return getLeftUpperLatitude (0);
    }

    public double getLeftUpperLongitude()
    {
        return getLeftUpperLongitude (0);
    }

    public double getRightLowerLatitude()
    {
        return getRightLowerLatitude (0);
    }

    public double getRightLowerLongitude()
    {
        return getRightLowerLongitude (0);
    }

    public double getLeftUpperLatitude (double padding)
    {
        return _maxLat + padding;
    }

    public double getLeftUpperLongitude (double padding)
    {
        return _minLong - padding;
    }

    public double getRightLowerLatitude (double padding)
    {
        return _minLat - padding;
    }

    public double getRightLowerLongitude (double padding)
    {
        return _maxLong + padding;
    }

    public boolean isValid()
    {
        if (!GeoCoordinates.isValidLatitude (_minLat) ||
            !GeoCoordinates.isValidLongitude (_minLong) ||
            !GeoCoordinates.isValidLatitude (_maxLat) ||
            !GeoCoordinates.isValidLongitude (_maxLong)) {
            return false;
        }

        if (((new Double (_minLat)).compareTo(_maxLat) == 0) &&
            ((new Double (_minLong)).compareTo(_maxLong) == 0)) {
            return true;
        }

        if (_minLat > _maxLat || _minLong > _maxLong) {
            return false;
        }

        return true;
    }

    public boolean equalLatitudeValues()
    {
        return ((new Double (_minLat)).compareTo (_maxLat) == 0);
    }

    public boolean equalLongitudeValues()
    {
        return ((new Double (_minLong)).compareTo (_maxLong) == 0);
    }
}
