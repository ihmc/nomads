/*
 * JeevesFinder.java
 *
 * Created on July 23, 2004, 12:37 PM
 */

package us.ihmc.jeeves.adminTool;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;

/**
 *
 * @author  nsuri
 * @version $Revision$
 * $Date$
 *
 */
public class JeevesFinder extends Thread
{
    public static class PlatformInfo
    {
        public InetAddress addr;
        public String status;
        public int port;

        public String toString()
        {
            String hostIP = addr.getHostAddress();
            String hostName = addr.getHostName();

            if (hostIP.equals (hostName)) {
                return hostIP + " " + port + " " + status;
            }
            else {
                //return hostName + " (" + hostIP + ") " + "port:" + port + " " + status;
                return hostName + " " + hostIP + " " + "port:" + port + " " + status;
            }
        }

        public int hashCode()
        {
            return addr.hashCode();
        }

        public boolean equals (Object obj)
        {
            InetAddress addPI = ((PlatformInfo)obj).addr;
            int portPI = ((PlatformInfo)obj).port;
            if (addr.equals (addPI) && (port == portPI)) {
                return true;
            }
            else {
                return false;
            }
            //return addr.equals (((PlatformInfo)obj).addr);
        }

        public int getCurrentPort()
        {
            return port;
        }

        public void setCurrentPort (int currPort)
        {
            port = currPort;
        }
    }

    public JeevesFinder (JeevesAdminTool adminTool, int udpPort, int searchTime)
    {
        _adminTool = adminTool;
        _udpPort = udpPort;
        _searchTime = searchTime;
    }

    public void run()
    {
        Vector results = new Vector();
        DatagramSocket dgSocket = null;
        try {
            dgSocket = new DatagramSocket();
            sendBroadcast (dgSocket, _udpPort);

            DatagramPacket dpIn = new DatagramPacket (new byte[2048], 2048);
            long startTime = System.currentTimeMillis();
            while (true) {
                long currTime = System.currentTimeMillis();
                int timeLeft = _searchTime - ((int) (currTime - startTime));
                if (timeLeft <= 0) {
                    break;
                }
                dgSocket.setSoTimeout (timeLeft);
                dgSocket.receive (dpIn);
                String reply = new String (dpIn.getData(), 0, dpIn.getLength());
                //System.out.println ("-->>Reply: " + reply);
                int iport = 3279;
                if (reply.startsWith ("Me")) {
                    Hashtable parsedReply = parseReply (reply);
                    PlatformInfo pi = new PlatformInfo();
                    pi.addr = dpIn.getAddress();
                    //System.out.println ("-->>In finder:address: " + dpIn.getAddress());
                    pi.status = (String) parsedReply.get ("status");
                    pi.port = iport;
                    if (pi.status != null) {
                        /*Enumeration e2 = results.elements();
                        while (e2.hasMoreElements()) {
                            JeevesFinder.PlatformInfo pi2 = (JeevesFinder.PlatformInfo) e2.nextElement();
                            System.out.println ("-->Result Vector in Finder before contains: " + pi2.toString());
                        }*/
                        if (!results.contains (pi)) {
                            System.out.println ("-->>In finder:result in table: " + pi.toString());
                            results.addElement (pi);
                            _adminTool.updatePlatformList (results);
                        }
                    }
                }
            }
        }
        catch (SocketTimeoutException e) {
            // Nothing to do
        }
        catch (Exception e) {
            _adminTool.addLogMsg ("Exception occurred - " + e);
            e.printStackTrace();
        }
        try {
            dgSocket.close();
        }
        catch (Exception e) {
            // Ignore
        }

        /*Enumeration e = results.elements();
        while (e.hasMoreElements()) {
            JeevesFinder.PlatformInfo pi = (JeevesFinder.PlatformInfo) e.nextElement();
            System.out.println ("-->Result Vector in Finder: " + pi.toString());
        }*/
        _adminTool.updatePlatformList();
        _adminTool.searchCompleted();
    }

    protected void sendBroadcast (DatagramSocket dgSocket, int port)
        throws UnknownHostException, IOException
    {
        String bMsg = "Who is There?";
        byte[] bData = bMsg.getBytes();
        DatagramPacket dpOut = new DatagramPacket (bData, 0, bData.length, InetAddress.getByName("255.255.255.255"), port);
        dgSocket.send (dpOut);
    }

    protected Hashtable parseReply (String reply)
    {
        Hashtable result = new Hashtable();
        StringTokenizer st = new StringTokenizer (reply, ";");
        while (st.hasMoreTokens()) {
            String token = st.nextToken();
            int eqIndex = token.indexOf ('=');
            if (eqIndex > 0) {
                String attr = token.substring (0, eqIndex);
                String value = token.substring (eqIndex+1);
                result.put (attr, value);
            }
        }
        return result;
    }

    protected JeevesAdminTool _adminTool;
    protected int _udpPort;
    protected int _searchTime;
}
