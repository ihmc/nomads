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
public class ARLGPSProviderUDP extends AsyncProvider
{
    public void init()
            throws Exception
    {
        _cloader = ConfigLoader.getDefaultConfigLoader();
        _pollInterval = _cloader.getPropertyAsInt("arl.provider.POS.pollInterval",_pollInterval);
        _svccipPath = getSvcCipPath();
        _gpsInfo = new GPSInfo();
        _prevGPSInfo = (GPSInfo) _gpsInfo.clone();
        _debug = _cloader.hasProperty("arl.provider.POS.debug");
        initialize();
    }

    //---------------------------------------------
    private void initialize ()
            throws Exception
    {
        try {
            _gpsInfo = new GPSInfo();
            _xmlMsgParser = new XMLMessageParser();

            _bRequest = null;
            int requestCmd = -1520040;
            String initString = "posdata\0 \0";
            _bRequest = ByteArrayHandler.appendByteArray (_bRequest, ByteArrayHandler.intToByteArray(requestCmd));
            _bRequest = ByteArrayHandler.appendByteArray (_bRequest, ByteArrayHandler.intToByteArray(initString.length()));
            _bRequest = ByteArrayHandler.appendByteArray (_bRequest, initString.getBytes());
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
                        sendRequest ();
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

    private void sendRequest ()
    {
        try {
            _os.write(_bRequest);
            _os.flush();
            _os.write(_bRequest);
            _os.flush();
         }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private boolean readPosData ()
    {
        try {
            byte[] header = ByteArrayHandler.readBytes (_is, 8, -1);
            if (header != null && header.length == 8) {
                int icode = ByteArrayHandler.ByteArrayToInt (header, 0);
                debugMsg("icode: " + icode);
                int payloadSize = ByteArrayHandler.ByteArrayToInt (header, 4);

                byte[] bData = ByteArrayHandler.readBytes (_is, payloadSize, -1);
                if (bData != null && bData.length > 0) {
                    debugMsg ("[Size: " + bData.length + "]\n" + new String(bData));
                    byte[] validbData = validateXMLByteArray(bData);
                    if (validbData != null) {
                        debugMsg ("[validbData: " + validbData.length + "]\n" + new String(validbData));
                        _xmlMsgParser.parseGPSInfo(_gpsInfo, new ByteArrayInputStream (validbData));
                        debugMsg (_gpsInfo.toString());
                        return true;
                    }
                    /*
                    byte[] testData = buildFakeData().getBytes();
                    debugMsg (testData.toString());
                    _xmlMsgParser.parseGPSInfo(_gpsInfo, new ByteArrayInputStream (testData));
                    debugMsg (_gpsInfo.toString());
                    return true;
                    */
                }
            }
            else {
                debugMsg ("Failed to get info from POS Server");
                return false;
            }
        }
        catch (Exception e) {
            debugMsg ("Got Exception: " + e.getMessage());
            if (e instanceof SocketException) {
                _connected = false;
            }
            return false;
        }
        return false;
    }

    private byte[] validateXMLByteArray (byte[] bData)
    {
        String saux = new String(bData);
        if (saux.indexOf("DeviceData") <0 ) {
            return null;
        }
        int nsize = bData.length;
        for (int i=bData.length-1; i>=0; i--) {
            if (bData[i] == '>') {
                 break;
            }
            else {
                nsize--;
            }
        }
        if (nsize > 0) {
            byte[] nArray = new byte[nsize];
            for (int j=0; j<nsize; j++) {
                nArray[j] = bData[j];
            }
            return (nArray);
        }
        return null;
    }

    //** for debug only **
    private String buildFakeData()
    {
        String stest = "";
        stest = stest + "<DeviceData id='100A106'>";
        stest = stest + "<EZCompassState>";
        stest = stest + "<Pitch>075.70</Pitch>";
        stest = stest + "<Yaw>-125.6</Yaw>";
        stest = stest + "<Roll>0-77.07</Roll>";
        stest = stest + "</EZCompassState>";
        stest = stest + "<GPSState>";
        stest = stest + "<Latitude>32.4340416666667</Latitude>";
        stest = stest + "<Longitude>-84.6287833531698</Longitude>";
        stest = stest + "<Altitude>0</Altitude>";
        stest = stest + "<Date>8/3/2004 11:07:41 PM</Date>";
        stest = stest + "<Time>23:8:29</Time>";
        stest = stest + "</GPSState>";
        stest = stest + "</DeviceData>";
        return (stest);
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
    private InputStream _is;
    private OutputStream _os;
    private byte[] _bRequest;
    private String _initString = "posdata\0 \0";

    private EnvAgentUDP _envAgentUDP;
    private ConfigLoader _cloader;
    private int _pollInterval = 2000;
    private XMLMessageParser _xmlMsgParser;
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
