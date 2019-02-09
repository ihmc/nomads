package us.ihmc.gps;

public class GarminTest
{
    public static void main (String[] args)
    {
        if (!Garmin.initialize()) {
            System.out.println ("Failed to initialize Garmin GPS");
            System.exit (-1);
        }
        Garmin.startReceivingUpdates();
        while (true) {
            Garmin.PVTData data = new Garmin.PVTData();
            Garmin.getData (data);
            System.out.println ("fix type = " + data.fixType + "; position = " + Math.toDegrees (data.lat) + ", " + Math.toDegrees(data.lon));
        }
    }    
}
