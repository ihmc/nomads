package us.ihmc.gps;
    
/**
 * MagellanHelper.java
 * 
 * <p> Description:
 * The MagellanHelper class receives via SerialComm communitcation input in
 * NMEA0183-Format from a Magellan GPS device (Sports Trak models: Map).
 * This class is used to strip input into usable formats: 
 *  String 
 *  bearing, degrees and minutes encapsulated in a GPSData object.
 * </p>
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version $Revision$ 
 * $Date$
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)</p>
 */
    
import java.util.StringTokenizer;

import us.ihmc.gps.exception.LatLongFormatException;
import us.ihmc.gps.position.Degrees;
import us.ihmc.util.SerialComm;
    

public class MagellanHelper extends SerialComm
{
    public MagellanHelper (int commPort) throws Exception
    {
		this (commPort, 'W', 'N');
    }


    public MagellanHelper (int comPort, char meridian, char hemisphere) 
        throws Exception
    {
        super (comPort, 9600, 8, 1, "none");
        Meridian = meridian;
        Hemisphere = hemisphere;
    }


    /**
     * Strips the NMEA0183-Formats to Gps field and returns a String containing
     * degrees, minutes and bearing ( i.e. 3029.2173,N )
     *
     * $GPGLL,3029.2173,N,08718.0558,W,233331.301,A*26
     * $GPGGA,233331.30,3029.2173,N,08718.0558,W,2,05,2.0,00021,M,,,,*35
     * $GPRMC,233331.30,A,3029.2173,N,08718.0558,W,00.0,000.0,240703,01,W*44
     */
    public String findGpsField(String str, int which) 
    {
        int beginIndex = 0;
        int endIndex;

        if (which == GGA || which == RMC) {
            int commaIndex = str.indexOf(',');
            if (commaIndex == -1) {
                return null;
            }
            beginIndex = (which == GGA) ? commaIndex + 1 : commaIndex + 3;
        }        
        str = str.substring(beginIndex, str.length());
        
        endIndex = str.indexOf(Meridian);
        if (endIndex == -1) {
            return null;
        }
        str = str.substring(0, endIndex + 1);

        return str;
    }
    
    public String getAltitude(String str) 
    {
        StringTokenizer st = new StringTokenizer(str,",");
        int counter = 0;
        String altitude = null;
        while(st.hasMoreTokens()){
            counter++;
            altitude = st.nextToken();
            if(counter == 10){
                break;
            }
        }
        
        return altitude;
    }
	
	public String getCourse(String str) 
    {
        StringTokenizer st = new StringTokenizer(str,",");
		int counter = 0;
		String course = null;
		while(st.hasMoreTokens()){
			counter++;
			course = st.nextToken();
			if(counter == 8){
				break;
			}
		}
        
        return course;
    }

    /**
     * Separates the 3029.2173,N,08718.0558,W Format into a latitude field:
     * returns a String. ( example 3029.2173,N )
     */
    public String getLatitude(String str) 
    {
        int commaIndex = str.indexOf(',');
        if (commaIndex == -1) {
            return null;
        }
        String lat = str.substring(0, commaIndex + 2);

        return lat;
    }


    /**
     * Separates the 3029.2173,N,08718.0558,W Format into longitude field:
     * returns a String. ( example 08718.0558,W )
     */
    public String getLongitude(String str) 
    {
        int commaIndex = str.indexOf(',');
        if (commaIndex == -1) {
            return null;
        }
        String _long = str.substring(commaIndex + 3, str.length());

        return _long;
    }


    /**
     * The accepted Latitude/Longitude format is "DMM.m,d", with:
     * D = degrees ( 1 or more digits )
     * M = minutes ( 2 digits )
     * m = minutes ( 1 or more digits )
     * d = direction( 1 character )
     * 
     * String parameter should be: 3029.2173,N or 08718.0558,W
     * Returns GPSData object which encapsulates bearing, degrees and minutes.
     */
    public Object parseGpsField (String str) throws LatLongFormatException 
    {
		String bearing;
		double deg;
        double min;
        int pointIndex;

        pointIndex = str.indexOf('.');
        if (pointIndex == -1) {
            throw new LatLongFormatException (null);
        }
        
		try {
            deg = Double.parseDouble(str.substring(0, pointIndex - 2));
            min = Double.parseDouble(str.substring(pointIndex - 2, str.length() - 2));
            bearing = str.substring(str.length() - 1, str.length());
        }
        catch (IndexOutOfBoundsException e) {
            throw new LatLongFormatException (e);
        }
        catch (NumberFormatException e) {
            throw new LatLongFormatException (e);
        }
		
        return new Degrees (deg, min, bearing);
    }


    //Class Variables.
    private char Hemisphere;
    private char Meridian;
    
    public static final int GLL = 0;
    public static final int GGA = 1;
    public static final int RMC = 2;    
	
}// end of class MagellanHelper
