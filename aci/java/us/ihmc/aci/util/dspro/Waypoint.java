package us.ihmc.aci.util.dspro;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class Waypoint {

    private float lat;
    private float lon;
    private float  alt;
    private long timestmp;
    private String loc;
    private String note;

    public Waypoint()
    {
        this(0.0f, 0.0f, (long) 0);
    }

    public Waypoint (float lat, float lon, long timestmp)
    {
        this (lat, lon, 0.0f, timestmp);
    }

    public Waypoint (float lat, float lon, float alt, long timestmp)
    {
        this (lat, lon, alt, timestmp, null, null);
    }
    
    public Waypoint (float lat, float lon, float alt, long timestmp, String loc, String note)
    {
        this.lat = lat;
        this.lon = lon;
        this.alt = alt;
        this.timestmp = timestmp;
        this.loc = loc;
        this.note = note;
    }

    public float getLat()
    {
        return lat;
    }

    public float getLon()
    {
        return lon;
    }

    public float getAlt()
    {
        return alt;
    }

    public long getTimestmp()
    {
        return timestmp;
    }

    public String getLoc()
    {
        return loc;
    }

    public String getNote()
    {
        return this.note;
    }

    public void setLat (float lat)
    {
        this.lat = lat;
    }

    public void setLon (float lon)
    {
        this.lon = lon;
    }

    public void setAlt (float alt)
    {
        this.alt = alt;
    }

    public void setTimestmp (long timestmp)
    {
        this.timestmp = timestmp;
    }

    public void setLoc (String loc)
    {
        this.loc = loc;
    }

    public void setNote (String note)
    {
        this.note = note;
    }
}

