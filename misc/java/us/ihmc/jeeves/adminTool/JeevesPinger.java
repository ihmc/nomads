/*
 * JeevesPinger.java
 *
 * Created on August 9, 2004, 6:11 AM
 */

package us.ihmc.jeeves.adminTool;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

import java.util.Hashtable;

/**
 *
 * @author  nsuri
 * @version $Revision$
 * $Date$
 * 
 */
public class JeevesPinger extends JeevesFinder
{
    public JeevesPinger (JeevesAdminTool adminTool, int udpPort, int searchTime, PlatformInfo[] pis,
                         Hashtable rowTable)
    {
        super (adminTool, udpPort, searchTime);
        _pis = pis;
        _rowTable = rowTable;
    }
    
    public void run()
    {        
        DatagramSocket dgSocket = null;
        int numPings = 3;
        for (int i = 0; i < _pis.length; i++) {
            _pis[i].status = "unknown";
        }
        _adminTool.updatePlatformList();

        try {            
            dgSocket = new DatagramSocket();
            for (int i = 0; i < numPings; i++) {
                for (int j = 0; j < _pis.length; j++) {
                    PlatformInfo pi = _pis[j];
                    sendPing (dgSocket, pi, _udpPort);
                }
            }

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
                if (reply.startsWith ("Me")) {
                    Hashtable parsedReply = parseReply (reply);
                    InetAddress addr = dpIn.getAddress();
                    for (int i = 0; i < _pis.length; i++) {
                        if (_pis[i].addr.equals (dpIn.getAddress())) {
                            String status = (String) parsedReply.get ("status");
                            System.out.println ("-->>JeevesPinger:status: " + status);
                            if (status != null) {
                                String rowStr = (String) _rowTable.get (_pis[i]);
                                _pis[i].status = status;
                                //System.out.println ("-->>JeevesPinger:row: " + i);
                                //System.out.println ("-->>JeevesPinger:_pis[i]: " + _pis[i].toString());
                                int row = Integer.parseInt (rowStr); 
                                _adminTool.updateStatusInTable (_pis[i], row);
                            }
                        }
                    }
                    break;
                }
            }
        }
        catch (SocketTimeoutException e) {
            // Nothing to do
        }
        catch (Exception e) {
            _adminTool.addLogMsg ("Exception occurred - " + e);
        }
        try {
            dgSocket.close();
        }
        catch (Exception e) {
            // Ignore
        }
        _adminTool.pingCompleted();        
    }

    protected void sendPing (DatagramSocket dgSocket, PlatformInfo pi, int port)
        throws UnknownHostException, IOException
    {
        String bMsg = "Who is There?";
        byte[] bData = bMsg.getBytes();
        DatagramPacket dpOut = new DatagramPacket (bData, 0, bData.length, pi.addr, port);
        dgSocket.send (dpOut);
    }

    private Hashtable _rowTable;
    private PlatformInfo[] _pis;
}
