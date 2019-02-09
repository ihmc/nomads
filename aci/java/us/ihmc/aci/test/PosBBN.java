package us.ihmc.aci.test;

import us.ihmc.aci.util.SNMPWalker;
import us.ihmc.aci.util.ByteArrayHandler;
import us.ihmc.aci.envMonitor.provider.arl.XMLMessageParser;
import us.ihmc.aci.envMonitor.provider.arl.GPSInfo;
import us.ihmc.aci.AttributeList;

import java.net.InetAddress;
import java.net.URI;
import java.net.Socket;
import java.net.SocketException;
import java.util.StringTokenizer;
import java.io.*;

/**
 * PosBBN
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 5, 2004 at 8:35:43 AM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class PosBBN extends Thread
{
    public PosBBN(String[] args)
    {
        if (args == null) {
            return;
        }
        for (int i=0; i<args.length; i++) {
            if (args[i].equals("-interval") && i<(args.length-1)) {
                try {
                    _interval = Integer.parseInt(args[i+1]);
                    if (_interval < 500) _interval = 500;
                }
                catch (Exception e) {
                    System.err.println ("Exception:" + e.getMessage() + ". Using default");
                }
            }
            if (args[i].equals("-gateway") && i<(args.length-1)) {
                _nodeGatewayIP = args[i+1];
            }
            if (args[i].equals("-debug")) {
                _debug = true;
            }
        }
        debugMsg ("Initializing...");
        initialize();
    }

    //--------------------------------------------------------------
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
                            setMIBInfo (_gpsInfo);
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
                    Thread.sleep(_interval);
                }
                catch (Exception e) {}
            }
        }
    }

    //--------------------------------------------------------------
    private void initialize()
    {
        try {
            _gpsInfo = new GPSInfo();
            if (_nodeGatewayIP == null) {
                getInterfaceAddress();
            }
            _snmpWalker = new SNMPWalker (_nodeGatewayIP, "robot");
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
                debugMsg (e.getMessage());
                //e.printStackTrace();
            }
        }
        return false;
    }

    //--------------------------------------------------------------
    private void setMIBInfo (GPSInfo gpsInfo)
    {
        String gpsInfoString = "";
        double latitude = gpsInfo.getLatitude();
        if (latitude == 0) {
            latitude = 999.0;
        }
        double longitude = gpsInfo.getLongitude();
        if (longitude == 0) {
            longitude = 999.0;
        }

        int altitude = (int)gpsInfo.getAltitude();
        int pitch = (int)gpsInfo.getPitch();
        int roll = (int)gpsInfo.getRoll();
        int yaw = (int)gpsInfo.getYaw();

        boolean update = false;
        if (latitude != _prevLat) {
            update = true;
        }
        if (longitude != _prevLong) {
            update = true;
        }

        if (update) {
            _prevLat = latitude;
            _prevLong = longitude;
            gpsInfoString = latitude + " " + longitude + " " + altitude;
            gpsInfoString = gpsInfoString + " " + pitch + " " + roll + " " + yaw + " " + ((int)System.currentTimeMillis());
            debugMsg ("Setting: " + gpsInfoString);
            try {
                _snmpWalker.setMIBValue(_gpsPosition + ".0", gpsInfoString);
            }
            catch (Exception e) {
                debugMsg (e.getMessage());
                //e.printStackTrace();
            }
        }
        else {
            debugMsg ("Not Updating - same as Previous");
        }
    }

    //--------------------------------------------------------------
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

    //--------------------------------------------------------------
    private boolean readPosData ()
    {
        try {
            byte[] header = ByteArrayHandler.readBytes (_is, 8, -1);
            if (header != null && header.length == 8) {
                int icode = ByteArrayHandler.ByteArrayToInt (header, 0);
                int payloadSize = ByteArrayHandler.ByteArrayToInt (header, 4);

                byte[] bData = ByteArrayHandler.readBytes (_is, payloadSize, -1);
                if (bData != null && bData.length > 0) {
                    byte[] validbData = validateXMLByteArray(bData);
                    if (validbData != null) {
                        debugMsg ("[DataLength: " + validbData.length + "]\n" + new String(validbData));
                        _xmlMsgParser.parseGPSInfo(_gpsInfo, new ByteArrayInputStream (validbData));
                        return true;
                    }

                    debugMsg (_gpsInfo.toString());
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
                //debugMsg ("Failed to get info from POS Server");
                return false;
            }
        }
        catch (Exception e) {
            debugMsg (e.getMessage());
            if (e instanceof SocketException) {
                _connected = false;
            }
            //e.printStackTrace();
            return false;
        }
        return false;
    }

    //--------------------------------------------------------------
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

    //--------------------------------------------------------------
    private URI lookupPOS()
    {
        URI serverURI = null;
        try {
            String serverPOS = getSvcCipPath() + _serverName;
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
            try {
                Thread.sleep(_posNotFoundInterval);
            }
            catch (Exception e2) {}
            //e.printStackTrace();
        }
        return serverURI;
    }

    //--------------------------------------------------------------
    private String getSvcCipPath()
    {
        String svcCipPath = null;
        String fs = System.getProperty("file.separator");
        if (fs.equals("\\")) {  //probably windows...
            svcCipPath = "C:\\temp\\SvcCip\\";
        }
        else {
            svcCipPath = "/tmp/SvcCip/";   //[TODO]double check this...
        }
        return svcCipPath;
    }

    //--------------------------------------------------------------
    private void getInterfaceAddress ()
    {
        _nodeGatewayIP = "192.168.11.1";
        _nodeAdHocIP = "10.0.0.11";
        debugMsg ("looking up interfaces");
        try {
            InetAddress[] allInets = InetAddress.getAllByName("localhost");
            InetAddress add = InetAddress.getByName (InetAddress.getLocalHost().getHostAddress());
            System.out.println(add.getHostAddress());
            if (!add.isLoopbackAddress()) {
                String inetAdd = add.getHostAddress();
                debugMsg ("Setting local Address to (" + inetAdd + ")");
                StringTokenizer st = new StringTokenizer (inetAdd, ".");
                if (st.countTokens() == 4) {
                    String oct1 = st.nextToken();
                    String oct2 = st.nextToken();
                    String oct3 = st.nextToken();
                    String oct4 = st.nextToken();
                    _nodeGatewayIP = oct1 + "." + oct2 + "." + oct3 + ".1";
                    _nodeAdHocIP = "10.0.0." + oct3;
                    debugMsg ("Setting nodeGatewayIP to (" + _nodeGatewayIP + ")");
                    debugMsg ("Setting nodeAdHocIP to (" + _nodeAdHocIP + ")");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //--------------------------------------------------------------
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[PosBBN] " + msg);
        }
    }

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

    private String _serverName = "SSPOSServer";
    private XMLMessageParser _xmlMsgParser;
    private boolean _connected = false;
    private SNMPWalker _snmpWalker;
    private boolean _debug = true;
    private String _nodeGatewayIP;
    private String _nodeAdHocIP;
    private int _interval = 1000;
    private int _posNotFoundInterval = 10000;
    private GPSInfo _gpsInfo;
    private byte[] _bRequest;
    private InputStream _is;
    private OutputStream _os;

    double _prevLat = -1;
    double _prevLong = -1;

    private static final String _gpsPosition = "1.3.6.1.4.1.14.5.15.3.4";

    //--------------------------------------------------------------
    public static void main(String[] args)
    {
        PosBBN pbbn = new PosBBN(args);
        pbbn.start();
    }
}
