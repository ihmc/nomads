package us.ihmc.aci.envMonitor.provider;


import java.net.URI;
import java.util.Hashtable;

import us.ihmc.aci.AttributeList;
import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphException;
import us.ihmc.gps.MagellanReader;
import us.ihmc.gps.position.GPSPosition;
import us.ihmc.util.ConfigLoader;

/**
 * SerialGPSProvider.java
 * <p>Description: </p>
 * Created on Feb 15, 2005
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version Revision: 1.0
 * Date: Feb 15, 2005
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */
public class SerialGPSProvider extends AsyncProvider 
{
    public void init() throws Exception 
    {
        _magellan = new MagellanReader();
        //default read interval can be over-ridden with 
        //_magellan.setReadInterval(int interval);

        ConfigLoader cfgLoader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cfgLoader.getProperty("gss.server.default.host");
        int gssServerPort = cfgLoader.getPropertyAsInt("gss.server.default.port");

        if (gssServerHost == null) {
            throw new Exception("Failed to query GSS Server Hostname from " 
                + "ConfigLoader " 
                + "(property gss.server.default.host in aci.properties)");
        }

        _fGraph = FGraph.getThinClient(new URI("tcp://" + gssServerHost + ":" + gssServerPort));
    }


    /**
     * Reads gps position from serial Magellan reader.  Delay for a given read 
     * interval, then get next position.  Blocking is providing if needed during
     * start up to ensure valid gps readings are achieved.
     */
    public void run() 
    {
        _magellan.start();
        
        while(true) 
        {
            try{
                Thread.sleep(_magellan.getReadInterval());
            }
            catch(Exception e) {}  
                 
            GPSPosition pos = _magellan.getGPSPosition();
            if (pos == null) {
                //GPSPosition == null is not unusual during Magellan start up.
                debugMsg("GPS position returned is null.");
                pos = magellanStartUp();
                if (pos == null){
                    debugMsg("Magellan GPS failed on start up.");
                    return;
                }
            }
            
            debugMsg("(lat/long) " + pos + "\t\tcourse = " 
                + _magellan.getCourse() + "\t\taltitude = " 
                + _magellan.getAltitude());
            
            double lat = pos.getLatitude().getDecimalDegrees();
            double lon = pos.getLongitude().getDecimalDegrees();
            if (discardUpdate (lat, lon)) {
                continue;
            }
            
            Hashtable attributes = new Hashtable();
            attributes.put(AttributeList.NODE_GPS_LATITUDE, new Double(lat));
            attributes.put(AttributeList.NODE_GPS_LONGITUDE, new Double(lon));
            attributes.put(AttributeList.NODE_GPS_ALTITUDE, new Double(_magellan.getAltitude()));
            try {
                _fGraph.setVertexAttributeList(_environmentalMonitor.getNodeName(), attributes);
            }
            catch (FGraphException e) {
                debugMsg("Exception: " + e.getMessage());
            }
            
            _environmentalMonitor.updateValues(attributes);
        }       
    }

    public double getThreshold ()
    {
        return _threshold;
    }

    public void setThreshold (double threshold)
    {
        _threshold = threshold;
    }
    
    /**
     * Check the threshold to see if this update should be discarded
     *
     * @return   true if the update should be discarded, false otherwise
     */
    private boolean discardUpdate (double lat, double lon)
    {
        if (((Math.abs (lat - _lastLat)) > _threshold) ||
            ((Math.abs (lon - _lastLon)) > _threshold)) {
            _lastLat = lat;
            _lastLon = lon;
            return false;
        }
        /*!!*/ // Not addressing altitude change here
        return true;
    }
    
    /**
     * Block for Magellan serial reader to get a valid position.  Null positions
     * during Magellan start on Linux systems is not unusual.
     * <p>
     * @param pos GPSPosition
     * @return GPSPosition
     */
    private GPSPosition magellanStartUp()
    {
        GPSPosition pos = null;
        long sendTimestamp = System.currentTimeMillis();
        
        while (pos == null && _blocking) 
        {
            pos = _magellan.getGPSPosition();
            if (pos != null) {
                _blocking = false;
                break;
            }
            else if ((System.currentTimeMillis() - _commTimeout) > sendTimestamp) {
                _blocking = false;
                return null;
            }
                 
            try{
                Thread.sleep(_magellan.getReadInterval());
            }
            catch(Exception e) {}       
        }
        
        return pos;
    }

    private void debugMsg (String msg) 
    {
        if (_debug) {
            System.out.println("[GPSProvider] " + msg);
        }
    }

    //Class variables.
    private FGraph _fGraph;
    private MagellanReader _magellan;
    private boolean _debug = false;
    private boolean _blocking = true;
    private double _lastLat = 0.0;
    private double _lastLon = 0.0;    
    private long _commTimeout = 30 * 1000;
    private double _threshold = 0.00000001;  //about 2 meter ?;
}
