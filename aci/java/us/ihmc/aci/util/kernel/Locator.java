package us.ihmc.aci.util.kernel;

import java.net.InetAddress;
import java.net.UnknownHostException;


public class Locator 
{

    /**
     *
     * @param uri (format is "acil://[<_instanceUUID>]|[<_nodeUUID>]|[<ipaddr[:_port]>]") where the
     * ipaddr and _port field may be blank and the _nodeUUID may be blank if the service is activated on the
     * local node or null in case of error
     */
    public Locator (String uri) 
        throws InvalidAciURIException
    {
        String temp;

        // =========== Checking correctness of the URI ===================

        // 0. Check for a null uri or short string
        if ((uri == null) || (uri.length() < 7)) {
            throw new InvalidAciURIException ("The passed URI (" + uri + ")is too short");
        }

        // 1. Checking that it begins with the proper "acil://" string
        if (!uri.startsWith("acil://")) {
            throw new InvalidAciURIException ("Invalid protocol specifier: \""+uri.substring(0, 7)+"\" in URI (" + uri + ")");
        }

        // 2. Checking that two '|' exist
        int index = uri.indexOf ('|');
        int index2 = uri.indexOf ('|', index+1);

        if ((index == -1) || (index2 == -1)) {
            throw new InvalidAciURIException ("Aci URI (" + uri + ") should have two '|' to separate fields");
        }

        // ================================================================

        // Extract the first part, which is the _instanceUUID string
        if (index > 7) {
            _instanceUUID = uri.substring(7, index);
        }
        else {
            _instanceUUID = null;
        }

        // Extract the _nodeUUID
        if (index2 - index > 1) {
            _nodeUUID = uri.substring (index+1, index2);
        }
        else {
            _nodeUUID = null;
        }

        // Extract the last part, which is the IP address (and optionally _port num)
        if (uri.length() - index2 > 1) {
            // Extract the Address part, made up of _ipAddress and, possibly, _port number
            temp = uri.substring (index2+1);
            splitIPAddress (temp);
        }
        else {
            _ipAddress = null;
            _port = 0;  
        }
    }

    private void splitIPAddress (String ip) 
        throws InvalidAciURIException
    {
        // Check for possible null ip address
        if (ip == null) {
            _ipAddress = null;
            _port = 0;
            return;
        }

        // Find the first occurrence of the ':' character
        int index3 = ip.indexOf (':');

        if (index3 == -1) { // _port number should be blank
            _ipAddress = new String(ip);
            _port = 0;
        }
        else { // address + _port number
            _ipAddress = ip.substring (0, index3);
            try {
                _port = Integer.parseInt (ip.substring(index3 + 1));

                // Simple checks on the _port number
                if ((_port < 0) || (_port > 65535)) {
                    throw new InvalidAciURIException ("Invalid _port number \""+_port+"\"");
                }
            }
            catch (NumberFormatException ex) {
                throw new InvalidAciURIException ("IP Address fields contains ':' but the _port number is invalid");
            }
        }

        // Check for a well-formed IP Address
        try {
            InetAddress.getByName (_ipAddress);
        }
        catch (UnknownHostException e) {
             throw new InvalidAciURIException ("IP Address is an invalid format!  Must be: [0-255].[0-255].[0-255].[0-255]");
        }
    }

    public Locator (String _instanceUUID, String ipAndPort, String _nodeUUID) 
        throws InvalidAciURIException
    {
        this._instanceUUID = _instanceUUID;
        splitIPAddress (ipAndPort);
        this._nodeUUID = _nodeUUID;
    }

    public Locator (String _instanceUUID, String _ipAddress, int _port, String _nodeUUID)
    {
        this._instanceUUID = _instanceUUID;
        this._ipAddress = _ipAddress;
        this._port = _port;
        this._nodeUUID = _nodeUUID;
    }

    public String getInstanceUUID()
    {
        return _instanceUUID;
    }

    public String getIpAddress()
    {
        return _ipAddress;
    }

    public int getPort()
    {
        return _port;
    }

    public String getNodeUUID()
    {
        return _nodeUUID;
    }

    public String toString()
    {
        StringBuffer b = new StringBuffer();

        b.append ("\n_instanceUUID="+((_instanceUUID != null) ? _instanceUUID:"null"));
        b.append ("\n_ipAddress="+((_ipAddress != null) ? _ipAddress:"null"));
        b.append ("\n_port="+_port);
        b.append ("\n_nodeUUID="+((_nodeUUID != null) ? _nodeUUID:"null"));

        return b.toString();

    }

//  public static void main (String[] args) 
//  {
//      try {
//          Locator l = new Locator ("acil://_instanceUUID|10.00.222.67:65535|_nodeUUID");
//      //  Locator l = new Locator (null, null, null);
//          System.out.println(l);
//      } catch (InvalidAciURIException e) {
//          // TODO Auto-generated catch block
//          e.printStackTrace();
//      }
//  }

    private String _instanceUUID;
    private String _ipAddress;
    private int _port;
    private String _nodeUUID;
}
