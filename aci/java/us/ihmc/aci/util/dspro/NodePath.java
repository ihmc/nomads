package us.ihmc.aci.util.dspro;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.logging.Logger;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.StringUtil;

public class NodePath
{
    public NodePath()
    {
        this (null, (short)-1, 0);
    }

    /**
     *
     * @param pathID
     * @param pathType
     * @param fProbability 
     */
    public NodePath (String pathID, short pathType, float fProbability)
    {
        _pathID = pathID;
        _pathType = pathType;
        _fProbability = fProbability;
        _fLatitude = new float[50];
        _fLongitude = new float[50];
        _fAltitude = new float[50];
        _location = new String[50];
        _note = new String[50];
        _hours = new int[50];
        _minutes = new int[50];
        _seconds = new int[50];
        _days = new int[50];
        _months = new int[50];
        _years = new int[50];
        _count = 0;
    }

    /**
     * 
     * @param fLatitude
     * @param fLongitude
     * @param fAltitude
     * @param location
     * @param note
     * @param time
     * @return 
     */
    public boolean appendWayPoint (float fLatitude, float fLongitude, float fAltitude,
                                   String location, String note, long time)
    {
        Calendar c = GregorianCalendar.getInstance();
        c.setTimeInMillis (time);

        return appendWayPoint (fLatitude, fLongitude, fAltitude, location, note,
                               c.get (Calendar.HOUR_OF_DAY),
                               c.get (Calendar.MINUTE),
                               c.get (Calendar.SECOND),
                               c.get (Calendar.MONTH)+1,    // MONTH ranges from 0 to 11
                               c.get (Calendar.DAY_OF_MONTH),
                               c.get (Calendar.YEAR));
    }

    /**
     * @param fLatitude
     * @param fLongitude
     * @param fAltitude
     * @param location
     * @param note
     * @param hours [0, 23]
     * @param minutes [0, 59]
     * @param seconds [0, 59]
     * @param months [1, 12]
     * @param days [1, 31]
     * @param years [1900, ... ]
     * @return
     */
    public boolean appendWayPoint (float fLatitude, float fLongitude, float fAltitude,
                                   String location, String note,
                                   int hours, int minutes, int seconds, int months, int days, int years)
    {
        _fLatitude[_count] = fLatitude;
        _fLongitude[_count] = fLongitude;
        _fAltitude[_count] = fAltitude;
        if (location == null) {
            _location[_count] = "Default";
        }
        else {
            _location[_count] = location;
        }
        if (note == null) {
            _note[_count] = "Default";
        }
        else {
            _note[_count] = note;
        }
        _hours[_count] = hours;
        _minutes[_count] = minutes;
        _seconds[_count] = seconds;
        _days[_count] = days;
        _months[_count] = months;
        _years[_count] = years;

        if ((_count +1) == _fLatitude.length) {
            int iNewSize = _fLatitude.length + 50;
            float[] fTmp = new float[iNewSize];
            String[] tmp = new String[iNewSize];
            int[] iTmp = new int[iNewSize];

            System.arraycopy (_fLatitude, 0, fTmp, 0, _fLatitude.length);
            _fLatitude = fTmp;

            fTmp = new float[iNewSize];
            System.arraycopy (_fLongitude, 0, fTmp, 0, _fLongitude.length);
            _fLongitude = fTmp;

            fTmp = new float[iNewSize];
            System.arraycopy (_fAltitude, 0, fTmp, 0, _fAltitude.length);
            _fAltitude = fTmp;

            System.arraycopy (_location, 0, tmp, 0, _location.length);
            _location = tmp;

            tmp = new String[iNewSize];
            System.arraycopy (_note, 0, tmp, 0, _note.length);
            _note = tmp;

            System.arraycopy (_hours, 0, iTmp, 0, _hours.length);
            _hours = iTmp;

            iTmp = new int[iNewSize];
            System.arraycopy (_minutes, 0, iTmp, 0, _minutes.length);
            _minutes = iTmp;

            iTmp = new int[iNewSize];
            System.arraycopy (_seconds, 0, iTmp, 0, _seconds.length);
            _seconds = iTmp;

            iTmp = new int[iNewSize];
            System.arraycopy (_days, 0, iTmp, 0, _days.length);
            _days = iTmp;

            iTmp = new int[iNewSize];
            System.arraycopy (_months, 0, iTmp, 0, _months.length);
            _months = iTmp;

            iTmp = new int[iNewSize];
            System.arraycopy (_years, 0, iTmp, 0, _years.length);
            _years = iTmp;
        }
        _count++;
        return true;
    }

    public float getLatitude (int wayPointIndex) throws Exception
    {
        if (wayPointIndex >= 0 && wayPointIndex <= _count) {
            return _fLatitude[wayPointIndex];
        }
        throw new Exception ("Way Point Index is out of bound.");
    }

    public float getLongitude (int wayPointIndex) throws Exception
    {
        if (wayPointIndex >= 0 && wayPointIndex <= _count) {
             return _fLongitude[wayPointIndex];
        }
        throw new Exception ("Way Point Index is out of bound.");
    }

    public float getAltitude (int wayPointIndex) throws Exception
    {
        if (wayPointIndex >= 0 && wayPointIndex <= _count) {
             return _fAltitude[wayPointIndex];
        }
        throw new Exception ("Way Point Index is out of bound.");
    }

    public String getLocation (int wayPointIndex) throws Exception
    {
        if (wayPointIndex >= 0 && wayPointIndex <= _count) {
             return _location[wayPointIndex];
        }
        throw new Exception ("Way Point Index is out of bound.");
    }

    public String getNote (int wayPointIndex) throws Exception
    {
        if (wayPointIndex >= 0 && wayPointIndex <= _count) {
             return _note[wayPointIndex];
        }
        throw new Exception ("Way Point Index is out of bound.");
    }

    /**
     *
     * @param wayPointIndex
     * @param field
     * @return the value of field.
     * The returned values are in the ranges listed as follow:
     * hour [0, 23]
     * min [0, 59]
     * sec [0, 59]
     * day [1, 31]
     * mon [1, 12]
     * year [1900, ... ]
     * @throws Exception
     */
    public int getTime (int wayPointIndex, int field) throws Exception
    {
        if (wayPointIndex >= 0 && wayPointIndex <= _count) {
            switch (field) {
                case Calendar.YEAR: return _years[wayPointIndex];
                case Calendar.MONTH: return _months[wayPointIndex];
                case Calendar.DAY_OF_MONTH: return _days[wayPointIndex];
                case Calendar.HOUR_OF_DAY: return _hours[wayPointIndex];
                case Calendar.MINUTE: return _minutes[wayPointIndex];
                case Calendar.SECOND: return _seconds[wayPointIndex];
                default: throw new Exception ("Time Graunlarity" + field + " not supported.");
            }
        }
        throw new Exception ("Way Point Index is out of bound.");
    }

    public long getTimeInMillis (int wayPointIndex)
    {
        return (new GregorianCalendar (_years[wayPointIndex],
                                       _months[wayPointIndex],
                                       _days[wayPointIndex],
                                       _hours[wayPointIndex],
                                       _minutes[wayPointIndex],
                                       _seconds[wayPointIndex])).getTimeInMillis();
    }

    public short getType ()
    {
        return _pathType;
    }

    public String getPathID()
    {
        return _pathID;
    }

    public boolean write (CommHelper commHelper)
    {
        try {
            commHelper.write16((short)_pathID.length());
            if (_pathID.length() > 0) {
                commHelper.sendBlob (_pathID.getBytes(), 0, _pathID.length());
            }
            commHelper.write32 (_fProbability);
            commHelper.write32 (_pathType);
            commHelper.write32(_count);

            // write all the point in the path
            for (int i = 0 ; i < _count ; i++) {
                commHelper.write32(_fAltitude[i]);
                commHelper.write32(_fLatitude[i]);
                commHelper.write32(_fLongitude[i]);
                commHelper.write16((short)_location[i].length());
                if (_location[i].length() > 0) {
                    commHelper.sendBlob (_location[i].getBytes(), 0, _location[i].length());
                }
                commHelper.write64((new GregorianCalendar(_years[i], _months[i]-1, _days[i], _hours[i], _minutes[i], _seconds[i])).getTimeInMillis()/1000);
            }
            return true;
        }
        catch (Exception e) {
            _LOGGER.severe (StringUtil.getStackTraceAsString (e));
            return false;
        }
    }

    public static NodePath read (InputStream in)
    {
        CommHelper commHelper = new CommHelper();
        commHelper.init (in, new ByteArrayOutputStream());
        return read (commHelper);
    }

    public static NodePath read (CommHelper commHelper)
    {
        String pathID;
        int pathType;
        float fProbability;
        NodePath newPath;

        try {
            // read Path Name
            short len = commHelper.read16();
            pathID = (len > 0 ? new String (commHelper.receiveBlob(len)) : null);
            if (pathID.startsWith("NOPATH")) {
                return null;
            }

            // Read Path Probability
            fProbability = Float.intBitsToFloat(commHelper.readI32());

            // Read Path Type
            pathType =  commHelper.read32();

            newPath = new NodePath (pathID, (short)pathType, fProbability);

            // Read Way Points
            int nWayPoints = commHelper.readI32();
            String location, note;
            float fAltitude, fLatitude, fLongitude;
            Calendar cal = new GregorianCalendar();
            for (int i = 0; i < nWayPoints; i++) {
                // Read Coordinates
                fAltitude =  Float.intBitsToFloat(commHelper.readI32());
                fLatitude =  Float.intBitsToFloat(commHelper.readI32());
                fLongitude = Float.intBitsToFloat(commHelper.readI32());

                // Read Location
                len = commHelper.read16();
                location = (len > 0 ? new String (commHelper.receiveBlob(len)) : null);

                // Read Note
                //len = commHelper.read16();
                //note = (len > 0 ? new String (commHelper.receiveBlob(len)) : null);

                // Read Timestamp (it is in seconds!!!)
                cal.setTimeInMillis (commHelper.readI64()*1000);

                newPath.appendWayPoint (fLatitude, fLongitude, fAltitude,
                                        location, null,
                                        cal.get (Calendar.HOUR),
                                        cal.get (Calendar.MINUTE),
                                        cal.get (Calendar.SECOND),
                                        cal.get (Calendar.MONTH),
                                        cal.get (Calendar.DAY_OF_MONTH),
                                        cal.get (Calendar.YEAR));
            }
            return newPath;
        }
        catch (Exception e) {
            _LOGGER.severe (StringUtil.getStackTraceAsString (e));
            return null;
        }
    }

    @Override
    public String toString ()
    {
        String ret = "NodePath: <" + _pathID + ":" + _pathType + ":" + _fProbability + ">\n";
        for (int i = 0; i < _count; i++) {
            ret += _months[i] + "/" + _days[i] + "/" + _years[i] + " "
                 + _hours[i] + ":" + _minutes[i] + ":" + _seconds[i] + "\n";
            ret += _fLatitude[i] + "\n";
            ret += _fLongitude[i] + "\n";
        }
        return ret;
    }

    public int getLength()
    {
        return _count;
    }

    // Variables
    private String _pathID;
    private short _pathType;
    private float _fProbability;
    private float[] _fLatitude;
    private float[] _fLongitude;
    private float[] _fAltitude;
    private String[] _location;
    private String[] _note;
    private int[] _hours;
    private int[] _minutes;
    private int[] _seconds;
    private int[] _days;
    private int[] _months;
    private int[] _years;
    private int _count;

    // Constants
    public static final short MAIN_PATH_TO_OBJECTIVE = 1;
    public static final short ALTERNATIVE_PATH_TO_OBJECTIVE = 2;
    public static final short MAIN_PATH_TO_BASE = 3;
    public static final short ALTERNATIVE_PATH_TO_BASE = 4;
    public static final short FIXED_LOCATION = 5;

    private static final Logger _LOGGER = Logger.getLogger (NodePath.class.getName());
}