package us.ihmc.aci.proxyudp;

import us.ihmc.aci.envMonitor.AsyncProvider;
import us.ihmc.aci.envMonitor.provider.arl.GPSInfo;
import us.ihmc.aci.envMonitor.provider.arl.XMLMessageParser;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.util.ByteArrayHandler;
import us.ihmc.aci.util.SNMPWalker;
import us.ihmc.aci.AttributeList;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.ds.fgraph.FGraphException;

import java.util.Hashtable;
import java.util.StringTokenizer;
import java.net.URI;
import java.net.Socket;
import java.net.SocketException;
import java.io.*;

/**
 * ARLGPSProvider
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Jul 30, 2004 at 7:44:45 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class ARLGPSLinuxProviderUDP extends AsyncProvider
{
    public void init()
            throws Exception
    {
        _cloader = ConfigLoader.getDefaultConfigLoader();
        _pollInterval = _cloader.getPropertyAsInt("arl.provider.POS.pollInterval",_pollInterval);
        _debug = _cloader.hasProperty("arl.provider.POS.debug");
        _svccipPath = getSvcCipPath();
        _gpsInfo = new GPSInfo();
        _prevGPSInfo = (GPSInfo) _gpsInfo.clone();
        initialize();
    }

    //---------------------------------------------
    private void initialize ()
            throws Exception
    {
        try {
            _gpsInfo = new GPSInfo();
            debugMsg ("data-request (size: " + _bRequest.length + " bytes)");
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //----------------------------------------------------------------
    private boolean connect()
    {
        URI posURI = lookupPOS();
        if (posURI != null)  {
            try {
                debugMsg("Will connect to " +posURI.getHost() + ":" + posURI.getPort());
                Socket sock = new Socket (posURI.getHost(), posURI.getPort());
                if (sock != null) {
                    _os = sock.getOutputStream();
                    _is = sock.getInputStream();
                    _dis = new DataInputStream(_is);
                    return true;
                }
            }
            catch (Exception e) {
                debugMsg ("Failed to connect: " + e.getMessage());
            }
        }
        return false;
    }


    //----------------------------------------------------------------
    public void run()
    {
        _connected = false;
        while (true)
        {
            synchronized (this) {
                try {
                    if (_connected) {
                        //sendRequest ();
                        if (readPosData ()) {
                            updateCoordinates();
                            _lastUpdate = System.currentTimeMillis();
                        }
                        else {
                            long currTime = System.currentTimeMillis();
                            if ((currTime - _lastUpdate) > _maxUpdateInterval) {
                                updateCoordinates();
                                _lastUpdate = currTime;
                            }
                        }
                    }
                    else {
                        _connected = connect();
                        if (!_connected) {
                            try {
                                Thread.sleep(_posNotFoundInterval);
                            }
                            catch (Exception e2) { }
                        }
                    }
                }
                catch (Exception e) {
                    debugMsg (e.getMessage());
                }

                try {
                    Thread.sleep(_pollInterval);
                }
                catch (Exception e) {}
            }
        }
    }

    public void terminate()
    {
        _initialized = false;
        _running = false;
    }

    public double getThreshold ()
    {
        return _threshold;
    }

    public void setThreshold (double thresh)
    {
        _threshold = thresh;
    }

    private boolean readPosData ()
    {
        try {
            int time = _dis.readInt();
            double latitude = _dis.readDouble();
            double longitude = _dis.readDouble();
            float heading = _dis.readFloat();
            float tilt = _dis.readFloat();
            float elevation = _dis.readFloat();
            float cameraPan = _dis.readFloat();
            float cameratilt = _dis.readFloat();
            float cameraArm = _dis.readFloat();
            float batteryPower = _dis.readFloat();
            float batteryQuality = _dis.readFloat();
            float xmove = _dis.readFloat();
            float ymove = _dis.readFloat();
            char idiotLights = _dis.readChar();

            _gpsInfo.setLatitude(latitude);
            _gpsInfo.setLongitude(longitude);
            return true;
        }
        catch (Exception e) {
            debugMsg ("Got Exception on readPosData (" + e.getMessage() + ")");
        }
        return false;
    }

     /**
     * In order to contact the Agent Registry, we must first
     * find it's ip and port number. This information is available
     * in a file (C:\\temp\\svccip\\) in the local system.
     */
    private URI lookupPOS()
    {
        URI serverURI = null;
        try {
            String serverPOS = _svccipPath + _serverName;
            FileReader fr = new FileReader (serverPOS);
            BufferedReader br = new BufferedReader (fr);
            String sline = br.readLine();
            if (sline != null) {
                StringTokenizer st = new StringTokenizer (sline);
                if (st.countTokens() >= 2) {
                    String posIP = st.nextToken();
                    int posPort = Integer.parseInt (st.nextToken());
                    debugMsg (_serverName + " fount at: " + posIP + ":" + posPort);
                    serverURI = new URI("tcp://" + posIP + ":" + posPort);
                }
            }
            br.close();
            fr.close();
        }
        catch (Exception e) {
            debugMsg ("Failed to locate POS_Server.");
            //e.printStackTrace();
        }
        return serverURI;
    }

    public void setEnvAgentUDP (EnvAgentUDP envAgentUDP)
    {
        _envAgentUDP = envAgentUDP;
    }

    private synchronized void updateCoordinates ()
    {
        debugMsg ("Updating Coordinates....");
        if (!_gpsInfo.equalsCartesian(_prevGPSInfo, _threshold))
        {
            String gpsInfoString = _gpsInfo.getLatitude() + " ";
            gpsInfoString = gpsInfoString + _gpsInfo.getLongitude() + " ";
            gpsInfoString = gpsInfoString + _gpsInfo.getAltitude() + " ";
            gpsInfoString = gpsInfoString + _gpsInfo.getYaw() + " ";
            gpsInfoString = gpsInfoString + _gpsInfo.getPitch() + " ";
            gpsInfoString = gpsInfoString + _gpsInfo.getRoll();
            try {
                if (_envAgentUDP != null) {
                    debugMsg ("Notifying envAgentUDP (" + gpsInfoString + ")");
                    _envAgentUDP.setGPS (gpsInfoString);
                }
            }
            catch (Exception e) {
                debugMsg ("GotException: " + e.getMessage());
            }
            _prevGPSInfo = (GPSInfo) _gpsInfo.clone();
        }
        else {
            debugMsg ("Same as previous, skip.");
        }
    }

    private String getSvcCipPath()
    {
        String svcCipPath = _cloader.getProperty("arl.provider.SvcCip.path");
        if (svcCipPath == null) {
            String fs = System.getProperty("file.separator");
            if (fs.equals("\\")) {  //probably windows...
                svcCipPath = "C:\\temp\\SvcCip\\";
            }
            else {
                svcCipPath = "/temp/SvcCip/";   
            }
        }
        return svcCipPath;
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[GPSProvider] " + msg);
        }
    }

    //this must be changes to a java property
    private long _maxUpdateInterval = 15000;

    private URI _posURI;
    private long _lastUpdate = 0;
    private String _svccipPath = "C:\\temp\\SvcCip\\";
    private DataInputStream _dis;
    private InputStream _is;
    private OutputStream _os;
    private byte[] _bRequest;
    private String _initString = "posdata\0 \0";

    private EnvAgentUDP _envAgentUDP;
    private ConfigLoader _cloader;
    private int _pollInterval = 2000;
    private String _serverName = "SSPOSServer";
    private GPSInfo _gpsInfo;
    private GPSInfo _prevGPSInfo;
    private boolean _connected;
    private int _posNotFoundInterval = 10000;
    private double _threshold = 0.0000001;           //about 2 meter ?
    private boolean _initialized = false;
    private boolean _running = true;
    private boolean _debug = false;
}


/*
    time = ubf.getInteger() ;
    latitude = ubf.getDouble();
    longitude = ubf.getDouble();
    heading = ubf.getFloat();
    tilt = ubf.getFloat();
    elevation = ubf.getFloat();
    cameraPan = ubf.getFloat();
    cameraTilt = ubf.getFloat();
    cameraArm = ubf.getFloat();
    batteryPower = ubf.getFloat();
    batteryQuality = ubf.getFloat();
    xmove = ubf.getFloat();
    ymove = ubf.getFloat();
    idiotLights = ubf.getChar();
    driver.assign(ubf.getCharPointer());
    return ubf.getCurrentPointer();
*/