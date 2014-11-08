package us.ihmc.mockets.util;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;

import java.util.Date;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.Vector;

import us.ihmc.mockets.Mocket;

/**
 * Monitors the status of mockets on the local platform by listening for stats packets
 * that are periodically sent out from mockets.
 *
 */
public class MocketStatusMonitor extends Thread
{
    public MocketStatusMonitor()
    {
        setDaemon (true);
        ExpirationThread et = new ExpirationThread (this);
        et.setDaemon (true);
        et.start();
        // Compute the log file name
        // 1101585624534 is the approximate time in milliseconds at 2:00 PM CST on 11/27/2004
        _logFileName = "mockets." + ((System.currentTimeMillis() - 1101585624534L) / 1000)+ ".log";
    }

    public void setMocketConnectionListener (MocketConnectionListener mcl)
    {
        _mcl = mcl;
    }

    public void setConnectionLossTime (long time)
    {
        _connectionLossTime = time;
    }
    
    public long getConnectionLossTime()
    {
        return _connectionLossTime;
    }

    public void run()
    {
        DatagramSocket dgSocket = null;
        DatagramPacket dgPacket = null;
        try {
            dgSocket = new DatagramSocket (STATS_PORT);
            dgPacket = new DatagramPacket (new byte[4096], 4096);
        }
        catch (SocketException e) {
            System.out.println ("MocketStatusMonitor: Cannot create socket on port " + STATS_PORT + " - abandoning"); 
            e.printStackTrace();
            return;
        }
        while (true) {
            try {
                dgSocket.receive (dgPacket);
                Hashtable info = parsePacket (dgPacket);
                if (info != null) {
                    String packetType = (String) info.get ("PacketType");
                    if (packetType.equalsIgnoreCase ("OpenConnectionInfo")) {
                        updateOpenConnectionInfo (info);
                    }
                    else if (packetType.equalsIgnoreCase ("ConnectionFailedInfo")) {
                        updateConnectionFailedInfo (info);
                    }
                    else {
                        System.out.println ("Do not know what to do with <" + packetType + ">");
                    }
                }
            }
            catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private Hashtable parsePacket (DatagramPacket dgp)
    {
        try {
            ByteArrayInputStream bais = new ByteArrayInputStream (dgp.getData(), 0, dgp.getLength());
            BufferedReader br = new BufferedReader (new InputStreamReader (bais));
            Hashtable info = new Hashtable();
            String packetType = br.readLine();
            if (packetType == null) {
                return null;
            }
            info.put ("PacketType", packetType);
            while (true) {
                String line = br.readLine();
                if (line == null) {
                    break;
                }
                StringTokenizer st = new StringTokenizer (line, "=");
                String key = st.nextToken();
                String value = st.nextToken();
                if ((key != null) && (value != null)) {
                    info.put (key, value);
                }
            }
            return info;
        }
        catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    private boolean updateOpenConnectionInfo (Hashtable info)
    {
        String localIP = (String) info.get ("LocalIP");
        String localPort = (String) info.get ("LocalPort");
        String remoteIP = (String) info.get ("RemoteIP");
        String remotePort = (String) info.get ("RemotePort");
        if ((localIP == null) || (localPort == null) || (remoteIP == null) || (remotePort == null)) {
            return false;
        }
        OpenConnectionInfo oci = new OpenConnectionInfo (localIP, Integer.parseInt (localPort), remoteIP, Integer.parseInt (remotePort));
        synchronized (_syncToken) {
            if (_connections.containsKey (oci)) {
                // This is an update about an already known connection
                oci = (OpenConnectionInfo) _connections.get (oci);     // Get the original object and update that one
                if (info.get("TimeSinceLastContact") != null) {
                    long timeSinceLastContact = Long.parseLong ((String) info.get ("TimeSinceLastContact"));
                    if (oci.timeSinceLastContact <= timeSinceLastContact) {
                        oci.timeSinceLastContact = timeSinceLastContact;
                        if ((!oci.connectionLost) && (timeSinceLastContact > _connectionLossTime)) {
                            oci.connectionLost = true;
                            _mcl.connectionLost (oci.getLocalAddr(), oci.getLocalPort(), oci.getRemoteAddr(), oci.getRemotePort());
                        }
                    }
                    else {
                        // Time since last contact is now smaller that the previous value
                        // Connection must bave been restored
                        oci.timeSinceLastContact = timeSinceLastContact;
                        if (oci.connectionLost) {
                            oci.connectionLost = false;
                            _mcl.connectionRestored (oci.getLocalAddr(), oci.getLocalPort(), oci.getRemoteAddr(), oci.getRemotePort());
                        }
                    }
                }
            }
            else {
                // This is a new connection - add it and notify listener
                oci.connectionStartTime = System.currentTimeMillis();
                if (info.get ("TimeSinceLastContact") != null) {
                    oci.timeSinceLastContact = Long.parseLong ((String) info.get ("TimeSinceLastContact"));
                }
                else {
                    oci.timeSinceLastContact = System.currentTimeMillis();
                }
                _connections.put (oci, oci);
                if (_mcl != null) {
                    _mcl.connectionEstablished (oci.getLocalAddr(), oci.getLocalPort(), oci.getRemoteAddr(), oci.getRemotePort());
                }
            }

            // Update the data in the OpenConnectionInfo object
            oci.lastUpdateTime = System.currentTimeMillis();
            String value;
            if (null != (value = (String) info.get ("BytesSent"))) {
                oci.bytesSent = Integer.parseInt (value);
            }
            if (null != (value = (String) info.get ("PacketsSent"))) {
                oci.packetsSent = Integer.parseInt (value);
            }
            if (null != (value = (String) info.get ("PacketsRetransmitted"))) {
                oci.packetsRetransmitted = Integer.parseInt (value);
            }
            if (null != (value = (String) info.get ("BytesReceived"))) {
                oci.bytesReceived = Integer.parseInt (value);
            }
            if (null != (value = (String) info.get ("PacketsReceived"))) {
                oci.packetsReceived = Integer.parseInt (value);
            }
            if (null != (value = (String) info.get ("PacketsDiscarded"))) {
                oci.packetsDiscarded = Integer.parseInt (value);
            }
        }
        return true;
    }

    private boolean updateConnectionFailedInfo (Hashtable info)
    {
        String localIP = (String) info.get ("LocalIP");
        String remoteIP = (String) info.get ("RemoteIP");
        String remotePort = (String) info.get ("RemotePort");
        if ((localIP == null) || (remoteIP == null) || (remotePort == null)) {
            return false;
        }
        ConnectionFailedInfo cfi = new ConnectionFailedInfo (System.currentTimeMillis(), localIP, remoteIP, Integer.parseInt(remotePort));
        _failedConnections.addElement (cfi);
        if (_mcl != null) {
            _mcl.connectionFailed (cfi.localAddr, cfi.remoteAddr, cfi.remotePort);
        }
        return true;
    }

    void writeStatusToLogFile()
    {
        try {
            FileOutputStream fos = new FileOutputStream (_logFileName);
            PrintWriter pw = new PrintWriter (new OutputStreamWriter (fos));
            pw.println ("Open Connections");
            pw.println ("----------------");
            Enumeration e = _connections.elements();
            while (e.hasMoreElements()) {
                OpenConnectionInfo oci = (OpenConnectionInfo) e.nextElement();
                long elapsedTime = ((System.currentTimeMillis() - oci.connectionStartTime) / 1000) + 1;
                pw.println (oci.getLocalAddr() + ":" + oci.getLocalPort() + " to " + oci.getRemoteAddr() + ":" + oci.getRemotePort());
                pw.println ("    Connection Established at: " + new Date (oci.connectionStartTime));
                pw.println ("    Connection Duration = " + (elapsedTime / 1000) + " sec");
                pw.println ("    Bytes Sent = " + oci.bytesSent);
                pw.println ("    Packets Sent = " + oci.packetsSent);
                pw.println ("    Packets Retransmitted = " + oci.packetsRetransmitted);
                pw.println ("    Bytes Received = " + oci.bytesReceived);
                pw.println ("    Packets Received = " + oci.packetsReceived);
                pw.println ("    Packets Discarded = " + oci.packetsDiscarded);
                pw.println ("    TimeSinceLastContact = " + oci.timeSinceLastContact);
                pw.println ("    Average Read Rate = " + oci.bytesReceived / elapsedTime + " bytes/sec");
                pw.println ("    Average Write Rate = " + oci.bytesSent / elapsedTime + " bytes/sec");
                pw.println();
            }
            pw.println();
            pw.println ("Connection Log");
            pw.println ("--------------");
            e = _oldConnections.elements();
            while (e.hasMoreElements()) {
                OpenConnectionInfo oci = (OpenConnectionInfo) e.nextElement();
                long elapsedTime = ((oci.connectionCloseTime - oci.connectionStartTime) / 1000) + 1;
                pw.println (oci.getLocalAddr() + ":" + oci.getLocalPort() + " to " + oci.getRemoteAddr() + ":" + oci.getRemotePort());
                pw.println ("    Connection Established at: " + new Date (oci.connectionStartTime));
                pw.println ("    Connection Duration = " + (elapsedTime / 1000) + " sec");
                pw.println ("    Bytes Sent = " + oci.bytesSent);
                pw.println ("    Packets Sent = " + oci.packetsSent);
                pw.println ("    Packets Retransmitted = " + oci.packetsRetransmitted);
                pw.println ("    Bytes Received = " + oci.bytesReceived);
                pw.println ("    Packets Received = " + oci.packetsReceived);
                pw.println ("    Packets Discarded = " + oci.packetsDiscarded);
                pw.println ("    TimeSinceLastContact = " + oci.timeSinceLastContact);
                pw.println ("    Average Read Rate = " + oci.bytesReceived / elapsedTime + " bytes/sec");
                pw.println ("    Average Write Rate = " + oci.bytesSent / elapsedTime + " bytes/sec");
                pw.println();
            }
            pw.println();
            pw.println ("Failed Connection Log");
            pw.println ("---------------------");
            e = _failedConnections.elements();
            while (e.hasMoreElements()) {
                ConnectionFailedInfo cfi = (ConnectionFailedInfo) e.nextElement();
                pw.println (cfi.localAddr + " to " + cfi.remoteAddr + ":" + cfi.remotePort + " at: " + 
                            new Date (cfi.time));
            }
            pw.close();
            fos.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private class ExpirationThread extends Thread
    {
        public ExpirationThread (MocketStatusMonitor msm)
        {
            _msm = msm;
        }

        public void run()
        {
            while (true) {
                try {
                    Thread.sleep (2000);
                }
                catch (InterruptedException e) {}
                long currTime = System.currentTimeMillis();
                synchronized (_msm._syncToken) {
                    Iterator i = _msm._connections.values().iterator();
                    while (i.hasNext()) {
                        OpenConnectionInfo oci = (OpenConnectionInfo) i.next();
                        if (oci.lastUpdateTime + MocketStatusMonitor.CONN_EXPIRATION_TIME <= currTime) {
                            i.remove();
                            oci.connectionCloseTime = currTime;
                            if (_msm._mcl != null) {
                                _msm._mcl.connectionClosed (oci.getLocalAddr(), oci.getLocalPort(), oci.getRemoteAddr(), oci.getRemotePort());
                            }
                            _oldConnections.addElement (oci);
                        }
                    }
                    if ((_lastDumpTime + STATUS_DUMP_INTERVAL) < currTime) {
                        _msm.writeStatusToLogFile();
                        _lastDumpTime = currTime;
                    }
                }
            }
        }

        private MocketStatusMonitor _msm;
    }

    static final int STATS_PORT = 9753;
    static final int CONN_EXPIRATION_TIME = 4000;
    static final int DEFAULT_CONNECTION_LOSS_WARNING_TIME = 5000;
    static final int STATUS_DUMP_INTERVAL = 15000;         // Dump status to a log file every 15 seconds

    private MocketConnectionListener _mcl;
    private long _connectionLossTime = DEFAULT_CONNECTION_LOSS_WARNING_TIME;
    private Object _syncToken = new Object();
    private Hashtable _connections = new Hashtable();      // Keys and Values are OpenConnectionInfo objects
    private Vector _oldConnections = new Vector();         // Elements are OpenConnectionInfo objects
    private Vector _failedConnections = new Vector();      // Elements are ConnectionFailedInfo objects
    private long _lastDumpTime;
    private String _logFileName;
}
