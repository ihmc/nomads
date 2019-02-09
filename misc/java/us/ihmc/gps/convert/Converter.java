package us.ihmc.gps.convert;

/**
 * Converter.java
 * <p>Description: </p>
 * Created on Sep 1, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version Revision: 1.0
 * Date: Sep 1, 2004
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */

import us.ihmc.gps.position.GPSPosition;


public class Converter 
{
    /**
     * Converts lat/long to UTM coordinates.  East Longitudes are positive, West
     * are negative.  North latitudes are positive, South are negative.  UTM is
     * limited to 84N to 80S.
     * <p>
     * @param ellipsoidRef
     * @param Lat -- decimal degrees
     * @param Long -- decimal degrees
     */
    public static double[] LLtoUTM (int ellipsoidRef, double Lat, double Long)
    {        
        if (Long < -180 || Long > 179.9) {
            System.err.println("LLtoUTM incorrect longitude parameter."); 
        }
        
        double N, T, C, A, M;
        double LatRad = Math.toRadians (Lat);
        double LongRad = Math.toRadians (Long);
        double radius = elliposid[ellipsoidRef].getRadius();
        double eccSquared = elliposid[ellipsoidRef].getEccentricity();
        
        int zoneNumber = getZoneNumber (Lat, Long);
                
        //+3 puts origin in middle of zone
        double LongOrigin = (zoneNumber - 1) * 6 - 180 + 3;
        double LongOriginRad = Math.toRadians (LongOrigin);
        double eccPrimeSquared = eccSquared / (1 - eccSquared);
        
        N = radius / Math.sqrt(1 - eccSquared * Math.sin(LatRad) * Math.sin(LatRad));
        T = Math.tan(LatRad) * Math.tan(LatRad);
        C = eccPrimeSquared * Math.cos(LatRad) * Math.cos(LatRad);
        A = Math.cos(LatRad) * (LongRad - LongOriginRad);
        M = radius * ((1 - eccSquared/4 - 3 * eccSquared * eccSquared / 64    
            - 5 * eccSquared * eccSquared * eccSquared / 256) * LatRad
            - (3 * eccSquared / 8 + 3 * eccSquared * eccSquared / 32
            + 45 * eccSquared * eccSquared * eccSquared / 1024) * Math.sin(2*LatRad)
            + (15 * eccSquared * eccSquared / 256 
            + 45 * eccSquared * eccSquared * eccSquared / 1024) * Math.sin(4*LatRad)
            - (35 * eccSquared * eccSquared * eccSquared / 3072) * Math.sin(6*LatRad));
    
        double UTMEasting = (k0*N*(A+(1-T+C)*A*A*A/6
            + (5-18*T+T*T+72*C-58*eccPrimeSquared)*A*A*A*A*A/120) + 500000.0);

        double UTMNorthing = (k0*(M+N*Math.tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)
            *A*A*A*A/24 + (61-58*T+T*T+600*C-330*eccPrimeSquared)*A*A*A*A*A*A/720)));
            
        // 10000000 meter offset for southern hemisphere
        if(Lat < 0) {
            UTMNorthing += 10000000.0;         
        }
        
        double ret[] = new double[2];
        ret[0] = UTMEasting;
        ret[1] = UTMNorthing;
        
        return ret;
    }
    
    /**
     * Converts UTM coords to lat/long.  East Longitudes are positive, West 
     * longitudes are negative.  North latitudes are positive, South latitudes 
     * are negative.  Lat and Long are in decimal degrees. 
     * <p>
     * @param ellipsoidRef
     * @param UTMNorthing
     * @param UTMEasting
     * @param ZoneNumber
     * @param Hemisphere
     * @return
     */
    public static GPSPosition UTMtoLL (int ellipsoidRef, double UTMNorthing,
        double UTMEasting, int ZoneNumber, int Hemisphere)
    {
        double x, y;
        double LongOrigin;
        double eccPrimeSquared;
        double mu, phi1Rad;
        double N1, T1, C1, R1, D, M;
        double radius = elliposid[ellipsoidRef].getRadius();
        double eccSquared = elliposid[ellipsoidRef].getEccentricity();
        double e1 = ( 1 - Math.sqrt(1 - eccSquared) ) /
            ( 1 + Math.sqrt(1 - eccSquared) );
            
        // remove 500,000 meter offset for longitude.
        x = UTMEasting - 500000.0; 
        y = UTMNorthing;

        // remove 10,000,000 meter offset used for southern hemisphere.
        if( Hemisphere == SOUTHERN_HEMISPHERE) {
            y -= 10000000.0;
        }

        //+3 puts origin in middle of zone
        LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  
        eccPrimeSquared = (eccSquared)/(1-eccSquared);

        M = y / k0;
        mu = M /( radius * (1-eccSquared/4 - 3 * eccSquared * eccSquared/64 -
            5 * eccSquared * eccSquared * eccSquared/256) );

        phi1Rad = mu + ( 3 * e1/2 - 27 * e1 * e1 * e1/32 ) * Math.sin(2*mu) + 
            (21 * e1 * e1/16 - 55 * e1 * e1 * e1 * e1/32) * Math.sin(4*mu) + 
            (151 * e1 * e1 * e1/96) * Math.sin(6*mu);
            

        N1 = radius/Math.sqrt(1-eccSquared * Math.sin(phi1Rad) * Math.sin(phi1Rad));
        T1 = Math.tan(phi1Rad) * Math.tan(phi1Rad);
        C1 = eccPrimeSquared* Math.cos(phi1Rad) * Math.cos(phi1Rad);
        R1 = radius * (1-eccSquared) / Math.pow( 1-eccSquared * Math.sin(phi1Rad) 
            * Math.sin(phi1Rad), 1.5 );
        D = x /(N1*k0);

        double Lat = phi1Rad - (N1 * Math.tan(phi1Rad)/R1) * (D * D/2 - (5 + 3 *
            T1 + 10 * C1 - 4 * C1 * C1 - 9 * eccPrimeSquared) * D * D * D * D/24
            +(61 + 90 * T1 + 298 * C1 + 45 * T1 * T1 - 252 * eccPrimeSquared - 3
            * C1 * C1) * D * D * D * D * D * D/720);
        Lat = Math.toDegrees (Lat);

        double Long = (D - (1 + 2 * T1 + C1) * D * D * D/6 + (5 - 2 * C1 + 28 * 
            T1 - 3 * C1 * C1 + 8 * eccPrimeSquared + 24 * T1 * T1) * D * D * D * 
            D * D/120) / Math.cos(phi1Rad);
        Long = LongOrigin + Math.toDegrees (Long);
        
        return ( new GPSPosition (Lat, Long) );
    }
    
    
    /**
     * Returns the zone number for a given GPSPosition parameter.
     * <p>
     * @param pos
     * @return UTM zone number.
     */
    public static int getZoneNumber (GPSPosition pos)
    {
        double lat = pos.getLatitude().getDecimalDegrees();
        double lon = pos.getLongitude().getDecimalDegrees();
        
        return getZoneNumber (lat, lon);
    }
    
    /**
     * Returns the zone number for a given longitude.
     * <p>
     * @param Lat -- decimal degrees
     * @param Long -- decimal degrees.
     * @return int of UTM zone number.
     */
    public static int getZoneNumber (double Lat, double Long)
    {
        int zoneNumber = ((int) (Long + 180)/6) + 1; 
        
        // Special Zones for Svalbard.
        if (Lat >= 56.0 && Lat < 64.0 && Long >= 3.0 && Long < 12.0) {
            zoneNumber = 32;
        }
        else if (Lat > 72.0 && Lat < 84){
            if (Long >= 0.0 && Long < 9.0) zoneNumber = 31;            
            else if (Long >= 9.0 && Long < 21.0)  zoneNumber = 33;
            else if (Long >= 21.0 && Long < 33.0) zoneNumber = 35;
            else if (Long >= 33.0 && Long < 42.0) zoneNumber = 37;
        }
        
        return zoneNumber;
    }
    
    
    // Class Variables.
    private static double k0 = 0.9996;    
    public static final int SOUTHERN_HEMISPHERE = 0; 
    public static final int NORTHERN_HEMISPHERE = 1; 
    
    // id, Ellipsoid name, Equatorial radius, square of eccentricity
    private static Ellipsoid elliposid[] = {
        new Ellipsoid (-1, "Placeholder", 0, 0),
        new Ellipsoid (1, "Airy", 6377563, 0.00667054),
        new Ellipsoid (2, "Australian National", 6378160, 0.006694542),
        new Ellipsoid (3, "Bessel 1841", 6377397, 0.006674372),
        new Ellipsoid (4, "Bessel 1841 (Nambia)", 6377484, 0.006674372),
        new Ellipsoid (5, "Clarke 1866", 6378206, 0.006768658),
        new Ellipsoid (6, "Clarke 1880", 6378249, 0.006803511),
        new Ellipsoid (7, "Everest", 6377276, 0.006637847),
        new Ellipsoid (8, "Fischer 1960 (Mercury)", 6378166, 0.006693422),
        new Ellipsoid (9, "Fischer 1968", 6378150, 0.006694322),
        new Ellipsoid (10, "GRS 1967", 6378160, 0.006694605),
        new Ellipsoid (11, "GRS 1980", 6378137, 0.00669438),
        new Ellipsoid (12, "Helmert 1906", 6378200, 0.006693422),
        new Ellipsoid (13, "Hough", 6378270, 0.00672267),
        new Ellipsoid (14, "International", 6378388, 0.00672267),
        new Ellipsoid (15, "Krassovsky", 6378245, 0.006693422),
        new Ellipsoid (16, "Modified Airy", 6377340, 0.00667054),
        new Ellipsoid (17, "Modified Everest", 6377304, 0.006637847),
        new Ellipsoid (18, "Modified Fischer 1960", 6378155, 0.006693422),
        new Ellipsoid (19, "South American 1969", 6378160, 0.006694542),
        new Ellipsoid (20, "WGS 60", 6378165, 0.006693422),
        new Ellipsoid (21, "WGS 66", 6378145, 0.006694542),
        new Ellipsoid (22, "WGS-72", 6378135, 0.006694318),
        new Ellipsoid (23, "WGS-84", 6378137, 0.00669438)
    };
    
}//end of class Converter
