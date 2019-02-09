package us.ihmc.aci.test;

import java.io.*;

import us.ihmc.aci.envMonitor.provider.arl.XMLMessageParser;
import us.ihmc.aci.envMonitor.provider.arl.GPSInfo;

/**
 * TestXMLParser
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 3, 2004 at 7:01:43 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class TestXMLParser
{
    public TestXMLParser()
    {
        showString ();
        System.out.println("-------------------------------------");
        parse();
        System.out.println("-------------------------------------");
    }

    //------------------------------------------------------
    private void parse ()
    {
        try {
            GPSInfo gpsInfo = new GPSInfo();
            _xmlMsgParser = new XMLMessageParser();
            FileInputStream fis = new FileInputStream (new File(_inputFile));
            _xmlMsgParser.parseGPSInfo (gpsInfo, fis);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void showString()
    {
        try{
            BufferedReader br = new BufferedReader (new FileReader (_inputFile));
            String sline = "";
            while (sline != null) {
                sline = br.readLine();
                System.out.println(sline);
            }
            br.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //------------------------------------------------------
    private void loadString()
    {
        _stest = "";
        _stest = _stest + "<DeviceData id='100A106'>";
        _stest = _stest + "<EZCompassState>";
        _stest = _stest + "<Pitch>075.70</Pitch>";
        _stest = _stest + "<Yaw>-125.6</Yaw>";
        _stest = _stest + "<Roll>0-77.07</Roll>";
        _stest = _stest + "</EZCompassState>";
        _stest = _stest + "<GPSState>";
        _stest = _stest + "<Latitude>32.4340416666667</Latitude>";
        _stest = _stest + "<Longitude>-84.6287833531698</Longitude>";
        _stest = _stest + "<Altitude>0</Altitude>";
        _stest = _stest + "<Date>8/3/2004 11:07:41 PM</Date>";
        _stest = _stest + "<Time>23:8:29</Time>";
        _stest = _stest + "</GPSState>";
        _stest = _stest + "</DeviceData>";
    }

    public static void main(String[] args)
    {
        TestXMLParser testParser = new TestXMLParser();
    }

    XMLMessageParser _xmlMsgParser;
    private String _inputFile = "gpsxml.txt";
    private String _stest;
}
