package us.ihmc.gps.position;

/*
 * GPSPosition.java
 * <p>Description: A GPS Position maintains a latitude - longitude pairing.
 * </p>
 * Created on Sep 1, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version Revision: 1.0
 * Date: Sep 1, 2004
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)</p>
 */

public class GPSPosition 
{
    public GPSPosition(double Lat, double Lon)    
    {
        _lat = new Degrees (Lat);
        _long = new Degrees (Lon);
    }
        
    public GPSPosition(Degrees Lat, Degrees Lon)    
    {
        _lat = Lat;
        _long = Lon;
    }
    
    public Degrees getLatitude()
    {
        return _lat;
    }
        
    public void setLatitude(Degrees lat)
    {
        _lat = lat;
    }
        
    public Degrees getLongitude()
    {
        return _long;
    }
    
    public void setLongitude(Degrees lon)
    {
        _long = lon;
    }	

	public String toString() 
    {
		return "Lat:" + getLatitude()+ "\tLong:" + getLongitude();
	}

    
    //Class Variables.
	private Degrees _lat = null;
    private Degrees _long = null;
    
}// end of class GPSPosition
