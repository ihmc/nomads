package us.ihmc.cot.parser;

import java.io.IOException;
import java.text.DecimalFormat;

/**
 * A Cursor on Target point.
 * 
 * @author Joe Bergeron
 */
public class CotPoint {

    public static final String TAG = "CotPoint";

    public static final double COT_CE90_UNKNOWN = 9999999;
    public static final double COT_LE90_UNKNOWN = 9999999;
    public static final double COT_ALTITUDE_UNKNOWN = 9999999;
    public static final String COT_ALTITUDE_UNKNOWN_SOURCE = "???";
    public static final String COT_GEOPOINT_UNKNOWN_SOURCE = "???";

    /**
     * Point where latitude, longitude, and height above ellipsoid equal UNKNOWN_ALTITUDE. Linear
     * error and circular error are UNKNOWN_CE90, UNKNOWN_LE90.
     */
    public static final CotPoint ZERO = new CotPoint(0d, 0d,
            COT_ALTITUDE_UNKNOWN,
            COT_CE90_UNKNOWN,
            COT_LE90_UNKNOWN);

    private double _lat;
    private double _lon;
    private double _hae = COT_ALTITUDE_UNKNOWN;
    private double _ce = COT_CE90_UNKNOWN;
    private double _le = COT_LE90_UNKNOWN;
    private String _altsrc = COT_ALTITUDE_UNKNOWN_SOURCE;
    private String _geopointsrc = COT_GEOPOINT_UNKNOWN_SOURCE;

    /**
     * Create a CoT point given all point attributes
     * 
     * @param lat latitude
     * @param lon longitude
     * @param hae height above ellipsoid in meters
     * @param ce circular radius error in meters (set to CotPoint.COT_CE90_UNKNOWN if unknown)
     * @param le linear error in meters (set to CotPoint.COT_LE90_UNKNOWN if unknown)
     * @param altsrc descriptor for the source of the altitude data.
     * @param geopointsrc a comma deliminated descritor of the geospatial source either one of
     * GeoPointSource abbreviated source is abreviated,imagex,imagey,filename
     */
    protected CotPoint(final double lat,
            final double lon,
            final double hae,
            final double ce,
            final double le,
            final String altsrc,
            final String geopointsrc) {
        _lat = lat;
        _lon = lon;
        _hae = hae;
        _ce = ce;
        _le = le;
        _altsrc = altsrc;
        _geopointsrc = geopointsrc;
    }

    /**
     * Create a CoT point given all point attributes
     * 
     * @param lat latitude
     * @param lon longitude
     * @param hae height above ellipsoid in meters
     * @param ce circular radius error in meters (set to CotPoint.COT_CE90_UNKNOWN if unknown)
     * @param le linear error in meters (set to CotPoint.COT_LE90_UNKNOWN if unknown)
     */
    public CotPoint(final double lat,
            final double lon,
            final double hae,
            final double ce,
            final double le) {
        this(lat, lon, hae, ce, le, COT_ALTITUDE_UNKNOWN_SOURCE,
                COT_GEOPOINT_UNKNOWN_SOURCE);
    }

    /**
     * Copy Constructor
     * 
     * @param point
     */
    public CotPoint(CotPoint point) {
        _lat = point._lat;
        _lon = point._lon;
        _hae = point._hae;
        _ce = point._ce;
        _le = point._le;
        _altsrc = point._altsrc;
        _geopointsrc = point._geopointsrc;
    }

    /**
     * Build upon a StringBuilder for the XML representation of this point.
     * 
     * @param b destination string builder
     */
    public void buildXml(Appendable b) throws IOException {
        b.append("<point lat='");
        b.append(String.valueOf(_lat));
        b.append("' lon='");
        b.append(String.valueOf(_lon));
        b.append("' hae='");
        b.append(String.valueOf(_hae));
        b.append("' ce='");
        b.append(numFormat.format(_ce));
        b.append("' le='");
        b.append(numFormat.format(_le));
        b.append("' />");
    }

    /**
     * Get the latitude
     * 
     * @return
     */
    public double getLat() {
        return _lat;
    }

    /**
     * Get the longitude
     * 
     * @return
     */
    public double getLon() {
        return _lon;
    }

    /**
     * Get the circular radius error in meters
     * 
     * @return NULL_VALUE if the error is unknown
     */
    public double getCe() {
        return _ce;
    }

    /**
     * Get the linear error in meters
     * 
     * @return NULL_VALUE if the error is unknown
     */
    public double getLe() {
        return _le;
    }

    /**
     * Get the height above ellipsoid in meters
     * 
     * @return
     */
    public double getHae() {
        return _hae;
    }

    /**
     * Get the source of the altitude data.
     * 
     * @return
     */
    public String getAltitudeSource() {
        return _altsrc;
    }

    /**
     * Get the source of the geospatial data stringified
     * 
     * @return
     */
    public String getGeoPointSource() {
        return _geopointsrc;
    }

    /**
     * Package protected fix up code for adding in the geospatial source after the details have been
     * read from a cot event. Only used by CotEvent. THIS METHOD SHOULD NEVER BE PUBLIC
     */
    void setGeoPointSource(final String geopointsrc) {
        if (geopointsrc != null)
            _geopointsrc = geopointsrc;
    }

    /**
     * Package protected fix up code for adding in the altitude source after the details have been
     * read from a cot event. Only used by CotEvent. THIS METHOD SHOULD NEVER BE PUBLIC
     */
    void setAltitudeSource(final String altsrc) {
        if (altsrc != null)
            _altsrc = altsrc;
    }

    static final DecimalFormat numFormat = new DecimalFormat("#.#");
}
