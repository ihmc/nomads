package us.ihmc.gps.convert;

/**
 * NorthAmericanInfo.java
 * <p>Description: 
 * </p>
 * Created on Aug 31, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version $Revision$
 * $Date$
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */

import us.ihmc.gps.position.Coordinate;
import us.ihmc.gps.position.Degrees;
import us.ihmc.gps.position.GPSPosition;


public class GPSConverter 
{    
    /**
     * If setDatum () has not been previousily used the GPSPosition returned 
     * will contain null latitude and longitudes.
     * <p>
     * @return
     */
    public static GPSPosition getDatum ()
    {
        return ( new GPSPosition (_originY, _originX) );
    }
    
    /**
     * Converts Latitude and Longitude into UTM reference position with meters
     * as the unit of measure.
     * <p>
     * @param latitude
     * @param longitude
     */
    public static void setDatum (Degrees latitude, Degrees longitude)
    {
        _originX = longitude;
        _originY = latitude;
        
        double lat = latitude.getDecimalDegrees();
        double lon = longitude.getDecimalDegrees();        
        double ret[] = Converter.LLtoUTM (_ellipsoidRef, lat, lon);
                
        _originUTMEasting = ret[0]; 
        _originUTMNorthing = ret[1];
    }
    
    /**
     * Returns GPSPosition determined using GPS datum previously set.
     * <p>
     * @param pos
     * @param hemisphere
     * @return
     */
    public static GPSPosition getGPSFromXY (Coordinate pos, int hemisphere)
    {
        GPSPosition datum = new GPSPosition(_originY, _originX);
        
        return getGPSFromXY (datum, pos, hemisphere);
    }

    /**
     * Returns GPSPosition given datum, coordinate position and N or S 
     * hemisphere (ex. Converter.NORTHERN_HEMISPHERE).
     * <p>
     * @param datum
     * @param pos
     * @param hemisphere    ex. Converter.NORTHERN_HEMISPHERE
     * @return
     */
    public static GPSPosition getGPSFromXY (GPSPosition datum, Coordinate pos,
        int hemisphere)
    {
        double lat0 = datum.getLatitude().getDecimalDegrees();
        double lon0 = datum.getLongitude().getDecimalDegrees();        
        double ret[] = Converter.LLtoUTM (_ellipsoidRef, lat0, lon0);
                
        double originUTMEasting = ret[0]; 
        double originUTMNorthing = ret[1];  
            
        double Northing = pos.getY() + originUTMNorthing;
        double Easting = pos.getX() + originUTMEasting;
        int zone = Converter.getZoneNumber (datum);
                
        return ( Converter.UTMtoLL(_ellipsoidRef, Northing, Easting, zone, hemisphere) );
    }
    
    /**
     * 
     * <p>
     * @param datum
     * @param location
     * @return
     */
    public static Coordinate getXYFromGPS (GPSPosition datum, GPSPosition location)
    {
        //set datum
        double lat0 = datum.getLatitude().getDecimalDegrees();
        double lon0 = datum.getLongitude().getDecimalDegrees();        
        double ret[] = Converter.LLtoUTM (_ellipsoidRef, lat0, lon0);
                
        double originUTMEasting = ret[0]; 
        double originUTMNorthing = ret[1];		
		
		//convert location to xy based on datum
        double lat = location.getLatitude().getDecimalDegrees();
        double lon = location.getLongitude().getDecimalDegrees();        		
        ret = Converter.LLtoUTM (_ellipsoidRef, lat, lon);
		
		double x = (ret[0] - originUTMEasting);
        double y = (ret[1] - originUTMNorthing);               

        return ( new Coordinate (x, y, Coordinate.METER) );
    }
	

    /**
     * UTM uses meters from reference points, so the positions are already 
     * metric.  
     * <p>
     * Subtracting northings gives distance in meters north-south.
     * Subtracting eastings gives distance in meters east-west.
     * </p><p>
     * @param latitude
     * @param longitude
     * @return PosMonitorData
     */
    public static Coordinate PosConverter (Degrees latitude, Degrees longitude)
    {
        double lat = latitude.getDecimalDegrees();
        double lon = longitude.getDecimalDegrees();             
        double ret[] = Converter.LLtoUTM (_ellipsoidRef, lat, lon);
        
        double x = (ret[0] - _originUTMEasting);
        double y = (ret[1] - _originUTMNorthing);               

        return ( new Coordinate (x, y, Coordinate.METER) );
    }
	
    /**
     * Pythagorean used to determine distance between two coordinates
     * returns distance as a float.
     *
	public float Pythagorean (double N2, double E2) 
    {
        double N1 = _originUTMNorthing;
        double E1 = _originUTMEasting;

        double distanceN = Math.pow((N1 - N2), 2);
        double distanceW = Math.pow((E1 - E2), 2);
        double pDistance = Math.sqrt(distanceN + distanceW);

        return (float) pDistance;
    }
*/    
    
    private static int _ellipsoidRef = 23; //WGS-84 -- hardcoded for now.
    private static Degrees _originX = null;
    private static Degrees _originY = null;
    private static double _originUTMEasting; 
    private static double _originUTMNorthing; 
    
}//end of class NorthAmericanInfo
