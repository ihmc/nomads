package us.ihmc.aci.util.dspro;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import java.util.Calendar;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.StringUtil;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class NodePath {
    
    // Constants
    public static final short MAIN_PATH_TO_OBJECTIVE = 1;
    public static final short ALTERNATIVE_PATH_TO_OBJECTIVE = 2;
    public static final short MAIN_PATH_TO_BASE = 3;
    public static final short ALTERNATIVE_PATH_TO_BASE = 4;
    public static final short FIXED_LOCATION = 5;

    private static final org.slf4j.Logger _LOGGER = LogUtils.getLogger (NodePath.class);

    private String pathId;
    private short pathType;
    private float pathProbability;
    private List<Waypoint> waypoints = new ArrayList<>();

    public NodePath()
    {
        this(null, (short) FIXED_LOCATION);
    }

    public NodePath (String pathId, short pathType)
    {
        this(pathId, pathType, 1.0f);
    }

    public NodePath (String pathId, short pathType, float pathProbability)
    {
        this.pathId = pathId;
        this.pathType = pathType;
        this.pathProbability = pathProbability;
    }

    public int getLength()
    {
        return waypoints.size();
    }

    public String getPathId()
    {
        return pathId;
    }

    public short getPathType()
    {
        return pathType;
    }

    public float getPathProbability()
    {
        return pathProbability;
    }

    public List<Waypoint> getWaypoints()
    {
       return waypoints;
    }

    public float getLatitude (int wayPointIndex)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        return waypoints.get(wayPointIndex).getLat();
    }

    public float getLongitude (int wayPointIndex)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        return waypoints.get(wayPointIndex).getLon();
    }

    public float getAltitude (int wayPointIndex)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        return waypoints.get(wayPointIndex).getAlt();
    }

    public long getTimestamp (int wayPointIndex)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        return waypoints.get(wayPointIndex).getTimestmp();
    }
    
    public int getTime (int wayPointIndex, int calendarField)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(waypoints.get(wayPointIndex).getTimestmp());
        return cal.get (calendarField);
    }
    
    public String getLocation (int wayPointIndex)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        return waypoints.get(wayPointIndex).getLoc();
    }

    public String getNote (int wayPointIndex)
    {
        if (wayPointIndex < 0 || wayPointIndex > waypoints.size()) {
            throw new ArrayIndexOutOfBoundsException ("Way Point Index is out of bound.");
        }
        return waypoints.get(wayPointIndex).getNote();
    }

    public void setPathId(String pathId)
    {
        this.pathId = pathId;
    }

    public void setpathType(short pathType)
    {
        this.pathType = pathType;
    }

    public void setPathProbability (float pathProbability)
    {
        this.pathProbability = pathProbability;
    }

    public void appendWayPoint (Waypoint waypoint)
    {
       waypoints.add (waypoint);
    }

    public void appendWayPoint (float lat, float lon, float alt, long timestmp, String loc, String note)
    {
       waypoints.add (new Waypoint (lat, lon, alt, timestmp, loc, note));
    }

    public void setWaypoints (List<Waypoint> newWaypoints)
    {
       waypoints.addAll(newWaypoints);
    }

    @Override
    public String toString ()
    {
        StringBuilder sb = new StringBuilder(pathId).append(' ')
                .append(this.pathType).append(' ')
                .append(this.pathProbability).append(System.lineSeparator());

        for (Waypoint w : waypoints)  {
            sb = sb.append(w.getLat()).append(", ")
                    .append(w.getLon()).append(", ")
                    .append(w.getAlt());
        }

        return sb.toString();
    }

    public static NodePath read (CommHelper commHelper)
    {
        try {
            // read Path Name
            short len = commHelper.read16();
            String pathID = (len > 0 ? new String (commHelper.receiveBlob(len), StandardCharsets.UTF_8) : null);
            if (pathID.startsWith("NOPATH")) {
                return null;
            }

            // Read Path Probability
            float fProbability = Float.intBitsToFloat(commHelper.readI32());

            // Read Path Type
            int pathType =  commHelper.read32();
            NodePath newPath = new NodePath (pathID, (short)pathType, fProbability);

            // Read Way Points
            int nWayPoints = commHelper.readI32();
            for (int i = 0; i < nWayPoints; i++) {
                // Read Coordinates
                float fAltitude =  Float.intBitsToFloat(commHelper.readI32());
                float fLatitude =  Float.intBitsToFloat(commHelper.readI32());
                float fLongitude = Float.intBitsToFloat(commHelper.readI32());

                // Read Location
                len = commHelper.read16();
                String location = (len > 0 ? new String (commHelper.receiveBlob(len), StandardCharsets.UTF_8) : null);

                // Read Note
                // len = commHelper.read16();
                // note = (len > 0 ? new String (commHelper.receiveBlob(len)) : null);

                // Read Timestamp (it is in seconds!!!)
                long timestamp = commHelper.readI64();

                newPath.appendWayPoint (fLatitude, fLongitude, fAltitude,
                                        timestamp, location, null);
            }
            return newPath;
        }
        catch (Exception e) {
            _LOGGER.error (StringUtil.getStackTraceAsString (e));
            return null;
        }
    }

    public boolean write (CommHelper commHelper)
    {
        try {
            commHelper.write16((short)pathId.length());
            if (pathId.length() > 0) {
                commHelper.sendBlob (pathId.getBytes(StandardCharsets.UTF_8), 0, pathId.length());
            }
            commHelper.write32 (pathProbability);
            commHelper.write32 (pathType);
            commHelper.write32(waypoints.size());

            // write all the point in the path
            for (Waypoint w : waypoints) {
                commHelper.write32(w.getAlt());
                commHelper.write32(w.getLat());
                commHelper.write32(w.getLon());
                String location = w.getLoc();
                int locationLen = (location == null ? 0 : location.length());
                commHelper.write16((short) locationLen);
                if (locationLen > 0) {
                    commHelper.sendBlob (location.getBytes(StandardCharsets.UTF_8), 0, locationLen);
                }
                commHelper.write64(w.getTimestmp());
            }
            return true;
        }
        catch (Exception e) {
            _LOGGER.error (StringUtil.getStackTraceAsString (e));
            return false;
        }
    }
}

