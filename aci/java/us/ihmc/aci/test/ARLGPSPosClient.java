package us.ihmc.aci.test;

import us.ihmc.aci.util.ByteArrayHandler;
import us.ihmc.util.ConfigLoader;
import us.ihmc.aci.envMonitor.provider.arl.XMLMessageParser;
import us.ihmc.aci.envMonitor.provider.arl.GPSInfo;

import java.net.Socket;
import java.io.*;

/**
 * ARLGPSPosClient
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Aug 2, 2004 at 11:43:09 AM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class ARLGPSPosClient extends Thread
{
    public ARLGPSPosClient (String host, int port)
            throws Exception
    {
        ConfigLoader configLoader = ConfigLoader.getDefaultConfigLoader();
        _poolInterval = configLoader.getPropertyAsInt("arl.provider.POS.poolInterval",_poolInterval);
        _debug = configLoader.hasProperty("arl.provider.POS.debug");
        System.out.println("PoolInterval: " + _poolInterval);
        debugMsg ("Debug is ON");

        _sock = new Socket (host, port);
        _xmlMsgParser = new XMLMessageParser();
        _gpsInfo = new GPSInfo();
        initialize (_sock);
    }

    public void run()
    {
        while (_running) {
            try {
                sendRequest();
                readPosData();
                Thread.sleep(_poolInterval);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void terminate()
    {
        _running = false;
    }

    /////////////////////////////////////////////////////////////////////
    private void initialize (Socket sock)
            throws Exception
    {
        _os = sock.getOutputStream();
        _is = sock.getInputStream();
        int requestCmd = -1520040;

        _bRequest = null;
        _bRequest = ByteArrayHandler.appendByteArray (_bRequest, ByteArrayHandler.intToByteArray(requestCmd));
        _bRequest = ByteArrayHandler.appendByteArray (_bRequest, ByteArrayHandler.intToByteArray(_initString.length()));
        _bRequest = ByteArrayHandler.appendByteArray (_bRequest, _initString.getBytes());
        debugMsg ("Sending data-request (size: " + _bRequest.length + " bytes)");
        for (int i=0; i<_bRequest.length; i++) {
            System.out.println(i + "\t" + _bRequest[i]);
        }
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

    private void readPosData ()
    {
        try {
            byte[] header = ByteArrayHandler.readBytes (_is, 8, -1);
            if (header != null && header.length == 8) {
                int icode = ByteArrayHandler.ByteArrayToInt (header, 0);
                System.out.println("icode: " + icode);
                int payloadSize = ByteArrayHandler.ByteArrayToInt (header, 4);

                byte[] bData = ByteArrayHandler.readBytes (_is, payloadSize, -1);
                if (bData != null && bData.length > 0) {
                    debugMsg ("[Size: " + bData.length + "]\n" + new String(bData));
                    byte[] validbData = validateXMLByteArray(bData);
                    debugMsg ("[validbData: " + validbData.length + "]\n" + new String(validbData));
                    _xmlMsgParser.parseGPSInfo(_gpsInfo, new ByteArrayInputStream (validbData));
                }
            }
            else {
                debugMsg ("Failed to get info from POS Server");
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private byte[] validateXMLByteArray (byte[] bData)
    {
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

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[ARLGPSPosClient] " + msg);
        }
    }

    public static void main(String[] args)
    {
        if (args.length != 2) {
            System.out.println("ARLGPSPosClient <host> <port>");
            System.exit(0);
        }
        try {
            int port = Integer.parseInt(args[1]);
            ARLGPSPosClient posClient = new ARLGPSPosClient (args[0], port);
            posClient.start();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private Socket _sock;
    private int _poolInterval = 500;
    private InputStream _is;
    private byte[] _bRequest;
    private OutputStream _os;
    private GPSInfo _gpsInfo;
    private boolean _debug = true;
    private boolean _running = true;
    private XMLMessageParser _xmlMsgParser;
    private String _initString = "posdata\0 \0";

}
