package us.ihmc.aci.envMonitor.provider.arl;

/**
 * GPSInfo
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 2, 2004 at 4:04:31 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class GPSInfo implements Cloneable
{
    public GPSInfo ()
    {
        reset();
    }

    public void setLatitude (double lat)
    {
        _latitude = lat;
    }

    public double getLatitude ()
    {
        return (_latitude);
    }

    public void setLongitude (double longitude)
    {
        _longitude = longitude;
    }

    public double getLongitude ()
    {
        return (_longitude);
    }

    public void setAltitude (double altitude)
    {
        _altitude = altitude;
    }

    public double getAltitude ()
    {
        return (_altitude);
    }

    public void setRoll (double roll)
    {
        _roll = roll;
    }

    public double getRoll ()
    {
        return (_roll);
    }

    public void setPitch (double pitch)
    {
        _pitch = pitch;
    }

    public double getPitch ()
    {
        return (_pitch);
    }

    public void setYaw (double yaw)
    {
        _yaw = yaw;
    }

    public double getYaw ()
    {
        return (_yaw);
    }

    public String toString()
    {
        String sinfo = "\n\tLat:" + _latitude + ", Long:" + _longitude + ", Alt:" + _altitude + "\n";
        sinfo = sinfo + "\tRoll:" + _roll + ", Pitch:" + _pitch + ", Yaw:" +  _yaw;
        return (sinfo);
    }

    public boolean equals (Object obj)
    {
        GPSInfo gpsInfo = (GPSInfo) obj;
        if (gpsInfo.getAltitude() != _altitude) {
            return false;
        }
        if (gpsInfo.getLongitude() != _longitude) {
            return false;
        }
        if (gpsInfo.getLatitude() != _latitude) {
            return false;
        }
        if (gpsInfo.getPitch() != _pitch) {
            return false;
        }
        if (gpsInfo.getRoll() != _roll) {
            return false;
        }
        if (gpsInfo.getYaw() != _yaw) {
            return false;
        }
        return true;
    }

    public boolean equalsCartesian (GPSInfo gpsInfo, double threshold)
    {
        if ((Math.abs(gpsInfo.getLatitude() - _latitude)) > threshold) {
            return false;
        }
        if ((Math.abs(gpsInfo.getLongitude() - _longitude)) > threshold) {
            return false;
        }
        return true;
    }

    public Object clone()
    {
        GPSInfo gpsInfo = new GPSInfo();
        gpsInfo.setAltitude(_altitude);
        gpsInfo.setLatitude(_latitude);
        gpsInfo.setLongitude(_longitude);
        gpsInfo.setPitch(_pitch);
        gpsInfo.setRoll(_roll);
        gpsInfo.setYaw(_yaw);
        return gpsInfo;
    }

    public void reset()
    {
        _latitude = 0;
        _longitude = 0;
        _altitude = 0;
        _roll = 0;
        _pitch = 0;
        _yaw = 0;
    }

    private double _latitude;
    private double _longitude;
    private double _altitude;
    private double _roll;
    private double _pitch;
    private double _yaw;
}
