package us.ihmc.gps;

import us.ihmc.gps.position.GPSPosition;

/**
 * MagellanTest.java
 * <p>Description: </p>
 * Created on Feb 18, 2005
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version Revision: 1.0
 * Date: Feb 18, 2005
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */
public class MagellanTest 
{
    public static void main (String[] args)
    {
        MagellanReader magellan = null; 
        
        try {
            if (args.length == 0) {
                magellan = new MagellanReader();
            }
            else {
                magellan = new MagellanReader(Integer.parseInt(args[0]));
            }
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(-2);
        }
        
        magellan.start();

        while (true) {
            GPSPosition pos = magellan.getGPSPosition();
            //System.out.println("Driver (lat/long) " + pos );            
            if (pos == null) {
                System.out.println("GPS Position returned null.");
                continue;
            }
            
            double lat = pos.getLatitude().getDecimalDegrees();
            double lon = pos.getLongitude().getDecimalDegrees();
            System.out.println (" position = " + lat + ", " + lon);

            //sleep
            try{
                Thread.sleep(magellan.getReadInterval());
            }
            catch(Exception e) {}       
            
        }
    }        
}
