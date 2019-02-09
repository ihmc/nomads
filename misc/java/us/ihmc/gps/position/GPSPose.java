package us.ihmc.gps.position;

/**
 * GPSPose.java
 * <p>
 * Description: 
 * </p><p>
 * Created on Sep 1, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * - Mat Johnson needed this for robots.
 * @version $Revision$
 * $Date$
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */

public class GPSPose extends GPSPosition
{
    public GPSPose (double Lat, double Lon)    
    {
        super (Lat, Lon);
    }	
        
    public GPSPose (Degrees Lat, Degrees Lon)    
    {
		super (Lat, Lon);
    }
    
	public void setCourse (double course)
	{
		_course = course;
	}
	
	public double getCourse()
	{
		return _course;	
	}

	public String toString() 
    {
		return "Lat:" + getLatitude()+ "\tLong:" + getLongitude() + 
			   "\tHDG:" + getCourse();
	}


    private double _course = 0.0;    
}