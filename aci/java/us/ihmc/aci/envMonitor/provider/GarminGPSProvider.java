package us.ihmc.aci.envMonitor.provider;

import java.net.URI;
import java.util.Hashtable;

import us.ihmc.aci.AttributeList;

import us.ihmc.aci.envMonitor.AsyncProvider;

import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphException;

import us.ihmc.gps.Garmin;

import us.ihmc.util.ConfigLoader;

/**
 *
 * @author  nsuri
 */
public class GarminGPSProvider extends AsyncProvider
{
    public void init() throws Exception
    {
        if (!Garmin.initialize()) {
            throw new Exception ("Failed to initialize GPS device");
        }
        ConfigLoader cfgLoader = ConfigLoader.getDefaultConfigLoader();
        String gssServerHost = cfgLoader.getProperty("gss.server.default.host");
        int gssServerPort = cfgLoader.getPropertyAsInt("gss.server.default.port");

        if (gssServerHost == null) {
            throw new Exception ("Failed to query GSS Server Hostname from ConfigLoader (property gss.server.default.host in aci.properties)");
        }

        _fGraph = FGraph.getThinClient (new URI("tcp://" + gssServerHost + ":" + gssServerPort));
    }

    public double getThreshold ()
    {
        return _threshold;
    }

    public void setThreshold (double threshold)
    {
        _threshold = threshold;
    }

    public void run()
    {
        Garmin.startReceivingUpdates();
        Garmin.PVTData pvtData = new Garmin.PVTData();

        while (true) {
            Garmin.getData (pvtData);
            if (!discardUpdate (pvtData)) {
                Hashtable attributes = new Hashtable();
                attributes.put (AttributeList.NODE_GPS_LATITUDE, new Double (Math.toDegrees (pvtData.lat)));
                attributes.put (AttributeList.NODE_GPS_LONGITUDE, new Double (Math.toDegrees (pvtData.lon)));
                attributes.put (AttributeList.NODE_GPS_ALTITUDE, new Double (pvtData.altitude));

                try {
                    _fGraph.setVertexAttributeList (_environmentalMonitor.getNodeName(), attributes);
                }
                catch (FGraphException e) {
                    debugMsg ("Exception: " + e.getMessage());
                }
                _environmentalMonitor.updateValues (attributes);
            }
        }
    }

    /**
     * Check the threshold to see if this update should be discarded
     *
     * @return   true if the update should be discarded, false otherwise
     */
    private boolean discardUpdate (Garmin.PVTData pvtData)
    {
        if (((Math.abs (pvtData.lat - _lastLat)) > _threshold) ||
            ((Math.abs (pvtData.lon - _lastLon)) > _threshold)) {
            _lastLat = pvtData.lat;
            _lastLon = pvtData.lon;
            return false;
        }
        /*!!*/ // Not addressing altitude change here
        return true;
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[GPSProvider] " + msg);
        }
    }

    private FGraph _fGraph;
    private double _threshold = 0.00000001;  //about 2 meter ?;
    private double _lastLat = 0.0;
    private double _lastLon = 0.0;

    private boolean _debug = true;
}
