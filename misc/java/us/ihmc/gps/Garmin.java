package us.ihmc.gps;

public class Garmin
{
    public static class PVTData
    {
        public static final int FIX_TYPE_UNUSABLE = 0;
        public static final int FIX_TYPE_INVALID = 1;
        public static final int FIX_TYPE_2D = 2;
        public static final int FIX_TYPE_3D = 3;
        public static final int FIX_TYPE_2D_DIFF = 4;
        public static final int FIX_TYPE_3D_DIFF = 5;

        public short  fixType;                            // Type of fix (see constants)
        public double lat;                                // Latitute in radians
        public double lon;                                // Longitude in radians
        public float  altitude;                           // Altitude above WGS 84 ellipsoid (meters)
        public float  mslHeight;                          // Height of WGS 84 ellipsoid above MSL (meters)
        public float  estimatedPositionError;             // Estimated error (meters)
        public float  estimatedHorizontalPositionError;   // Estimated horizontal positioning error (meters)
        public float  estimatedVerticalPositionError;     // Estinated vertical positioning error (meters)
        public float  eastVelocity;                       // Easterly velocity (meters/sec)
        public float  northVelocity;                      // Northerly velocity (meters/sec)
        public float  upVelocity;                         // Vertical velocity (meters/sec)
        public double timeOfWeek;                         // Number of seconds since the beginning of the current week
        public short  leapSeconds;                        // Number of leap seconds
        public int    weekNumberDays;                     // Number of days that have occurred from UTC December 31st, 1989 to the beginning of the current week
    }
    
    public static native boolean initialize();
    public static native void startReceivingUpdates();
    public static native void close();
    public static native void getData (PVTData data);
    
    static {
        System.loadLibrary ("garmin");
    }
}
