//package us.ihmc.mockets.tests.messagexmit;
package us.ihmc.mockets.test;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;

import us.ihmc.util.ByteConverter;
//import us.ihmc.mockets.Mocket;
//import us.ihmc.mockets.Sender;
//import us.ihmc.mockets.MocketStatusListener;
//import us.ihmc.mockets.ServerMocket;
import us.ihmc.mockets.*;

import java.net.Socket;
import java.net.SocketException;
import java.net.SocketAddress;
import java.net.ServerSocket;


public class MultiThreadTest
{
    private static class Stats
    {
        void update (double value)
        {
            _sumValues += value;
            _sumSqValues += Math.pow (value, 2);
            ++_totalNumValues;
        }

        void reset()
        {
            _sumValues = 0;
            _sumSqValues = 0;
            _totalNumValues = 0;
        }

        int getNumValues()
        {
            return _totalNumValues;
        }

        double getAverage()
        {
            return (_sumValues/_totalNumValues);
        }

        double getStDev()
        {
            double avg = getAverage();
            double aux = (_totalNumValues * avg * avg)
                         - (2 * avg * _sumValues)
                         + _sumSqValues
                         ;
            aux = (double) aux / (_totalNumValues - 1);
            aux = Math.sqrt (aux);

            return aux;
        }

        private double _sumValues = 0;
        private double _sumSqValues = 0;
        private int _totalNumValues = 0;
    }

    private static class ConnHandler extends Thread implements MocketStatusListener
    {
        ConnHandler (Mocket mocket, boolean sequenced, boolean reliable)
        {
            _mocket = mocket;
            try {
                _sender = _mocket.getSender (sequenced, reliable);
            }
            catch (Exception e) {
                throw new RuntimeException (e);
            }
            _socket = null;
            _useMockets = true;
        }

        ConnHandler (Socket socket)
        {
            _mocket = null;
            _sender = null;
            _socket = socket;
            _useMockets = false;
        }

        public boolean peerUnreachableWarning (long x)
        {
            _closeConn = (x > 60 * 1000);

            if (_closeConn) {
                System.out.println ("MultiThreadTest :: MSL >> connection inactive for 60 seconds, will disconnect.");
            }

            return _closeConn;
        }

        public boolean peerReachable(long x)
        {
            System.out.println("The peer is reachable again after a period of "+x+" milliseconds");

            return true;
        }

        public boolean suspendReceived (long x)
        {
            System.out.println("The peer has been suspend for "+x+" milliseconds");

            return true;
        }

        public void run()
        {
            try {
                InetSocketAddress remoteAddr;
                if (_useMockets) {
                    remoteAddr = (InetSocketAddress) _mocket.getPeerName();
                    _mocket.setStatusListener(this);
                }
                else {
                    remoteAddr = (InetSocketAddress) _socket.getRemoteSocketAddress();
                }

                System.out.println ("MultiThreadTest :: accepted a connection from " + remoteAddr.getAddress().getHostAddress()
                                   + ":" + remoteAddr.getPort());

                long startTime;
                long id;
                long numRead = 0;
                byte[] buffer = new byte[2048];

                if (_useMockets) {
                    int rr = _mocket.receive (buffer, 0, buffer.length);
                    if (rr != 12)
                        throw new RuntimeException ("We were supposed to read 12 bytes here instead of " + rr + " !!!");                    
                    long numBytes = ByteConverter.from8BytesToLong (buffer);
                    id = ByteConverter.from4BytesToUnsignedInt (buffer, 8);

                    System.out.println ("MultiThreadTest :: Server >> will read a total of [" + numBytes + "] bytes. ID = [" + id + "]");

                    startTime = System.currentTimeMillis();
                    _closeConn = false;
                    while ( (numRead < numBytes) && !_closeConn) {
                        try {
                            numRead += _mocket.receive (buffer, 0, buffer.length);
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    }
                    
                    long totalTime = System.currentTimeMillis() - startTime;
                    System.out.println ("MultiThreadTest :: done reading.");

                    if (_closeConn) {
                        System.out.println ("Connection reset.");
                        reportError (id, "server", "Connection Reset.");
                        return;
                    }

                    System.out.println ("MultiThreadTest :: sending back the ACK [" + numRead + "]");
    
                    ByteConverter.fromLongTo8Bytes (numRead, buffer);
                    _sender.send (buffer, 0, 8);
                    _mocket.close();
                    saveStats (id, _mocket, "server", totalTime);
                } else {
                    InputStream is = _socket.getInputStream();
                    OutputStream os = _socket.getOutputStream();
                    BufferedInputStream bis = new BufferedInputStream (is);
                    DataInputStream dis = new DataInputStream (is);
                    long numBytes = dis.readLong();
                    id = (long) dis.readInt();

                    System.out.println("MultiThreadTest :: Server >> will read a total of [" + numBytes + "] bytes. ID = [" + id + "]");

                    startTime = System.currentTimeMillis();
                    _closeConn = false;
                    while ( (numRead < numBytes) && !_closeConn) {
                        try {
                            numRead += bis.read (buffer);
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                            reportError (id, "server", "Connection Reset (failed to write.)");
                            break;
                        }
                    }
                    
                    long totalTime = System.currentTimeMillis() - startTime;
                    System.out.println ("MultiThreadTest :: done reading.");

                    if (_closeConn) {
                        System.out.println ("Connection reset.");
                        reportError (id, "server", "Connection Reset.");
                        return;
                    }

                    System.out.println ("MultiThreadTest :: sending back the ACK [" + numRead + "]");
    
                    DataOutputStream dos = new DataOutputStream (os);
                    dos.writeLong (numRead);
                    _socket.close();
                    saveStats (id, _socket, "server", totalTime);
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }

        private Mocket _mocket;
        private Mocket.Sender _sender;
        private Socket _socket;
        private boolean _useMockets;
        private boolean _closeConn;
    } 

    public static class ClientThread extends Thread implements MocketStatusListener
    {
        public ClientThread (String remoteHost, int port, int id)
        {
            _remoteHost = remoteHost;
            _port = port;
            _id = id;
            _sequenced = false; // not necessary
            _reliable = false; // not necessary
            _useMockets = false;
        }

        public ClientThread (String remoteHost, int port, boolean sequenced, boolean reliable, int id)
        {
            _remoteHost = remoteHost;
            _port = port;
            _id = id;
            _sequenced = sequenced;
            _reliable = reliable;
            _useMockets = true;
        }

        public boolean peerUnreachableWarning (long x)
        {
            _closeConn = (x > 40 * 1000);

            if (_closeConn) {
                System.out.println ("MultiThreadTest :: MSL >> connection inactive for 60 seconds, will disconnect.");
            }

            return _closeConn;
        }

        public boolean peerReachable(long x)
        {
            System.out.println("The peer is reachable again after a period of "+x+" milliseconds");

            return true;
        }

        public boolean suspendReceived (long x)
        {
            System.out.println("The peer has been suspend for "+x+" milliseconds");

            return true;
        }

        public void run()
        {
            String remoteHost = _remoteHost;
            int portNumber = _port;
            int id = _id;

            try {
                InetAddress remoteIP = InetAddress.getByName (remoteHost);
                SocketAddress remoteAddr = new InetSocketAddress (remoteIP, portNumber);

                System.out.println ("MultiThreadTest :: will try to connect to " + remoteHost + ":" + portNumber);

                if (_useMockets) {
                    System.out.println ("USING MOCKETS.");
                    Mocket mocket = new Mocket();
                    mocket.connect (remoteAddr, CONNECT_TIMEOUT);
                    mocket.setStatusListener (this);
                    Mocket.Sender sender = mocket.getSender (_sequenced, _reliable);

                    System.out.println ("MultiThreadTest :: connected successfully to " + remoteHost + ":" + portNumber);

                    byte[] buffer = new byte[2048];
                    ByteConverter.fromLongTo8Bytes (BYTES_TO_SEND, buffer);
                    ByteConverter.fromUnsignedIntTo4Bytes (id, buffer, 8);
                    sender.send (buffer, 0, 12);

                    System.out.println ("MultiThreadTest :: will start sending data.");
                    
                    int numWritten = 0;
                    long startTime = System.currentTimeMillis();
                    _closeConn = false;
                    while ( (numWritten < BYTES_TO_SEND) && !_closeConn) {
                        try {
                            sender.send (buffer, 0, MESSAGE_MOCKET_MTU);
                            numWritten += buffer.length;
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                        }
                    }
                    long totalTime = System.currentTimeMillis() - startTime;

                    if (_closeConn) {
                        System.out.println ("connection reset");
                        reportError (id, "client", "Connection Reset");
                    } else {
                        _clientStats.update (totalTime);
                    }

                    System.out.println ("done sending data... it took [" + totalTime + "]. Waiting for ACK from server.");

                    mocket.receive (buffer, 0, buffer.length);
                    long totalRead = ByteConverter.from8BytesToLong (buffer);
                    System.out.println ("MultiThreadTest :: server received a total of [" + totalRead + "] bytes.");
                    
                    saveStats (id, mocket, "client", totalTime);
                    mocket.close();
                } else {
                    System.out.println("USING SOCKETS.");
                    Socket socket = new Socket();
                    socket.connect (remoteAddr, CONNECT_TIMEOUT);

                    OutputStream os = socket.getOutputStream();
                    InputStream is = socket.getInputStream();

                    System.out.println ("MultiThreadTest :: connected successfully to " + remoteHost + ":" + portNumber);

                    DataOutputStream dos = new DataOutputStream (os);
                    dos.writeLong (BYTES_TO_SEND);
                    dos.writeInt (id);
                    BufferedOutputStream bos = new BufferedOutputStream (os);

                    byte[] buffer = new byte[2048];
                    
                    System.out.println ("MultiThreadTest :: will start sending data.");

                    int numWritten = 0;
                    long startTime = System.currentTimeMillis();
                    _closeConn = false;
                    while ( (numWritten < BYTES_TO_SEND) && !_closeConn) {
                        try {
                            bos.write (buffer);
                            numWritten += buffer.length;
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                            reportError (id, "client", "Connection Reset (failed to write.)");
                            break;
                        }
                    }
                    long totalTime = System.currentTimeMillis() - startTime;

                    if (_closeConn) {
                        System.out.println ("connection reset");
                        reportError (id, "client", "Connection Reset");
                    }
                    else {
                        _clientStats.update (totalTime);
                    }

                    System.out.println ("done sending data... it took [" + totalTime + "]. Waiting for ACK from server.");

                    DataInputStream dis = new DataInputStream (is);
                    long totalRead = dis.readLong();
                    System.out.println ("MultiThreadTest :: server received a total of [" + totalRead + "] bytes.");
                    
                    saveStats (id, socket, "client", totalTime);
                    socket.close();
                }

                System.out.println ("MultiThreadTest :: closed the connection ID = " + id + "(this is normal)");
            }
            catch (Exception ex) {
                ex.printStackTrace();
                reportError (id, "client", "Connection Failed");
            }

            System.out.println ("\n-----\n");
        }

        private String _remoteHost;
        private int _port;
        private int _id;
        private boolean _closeConn;
        private boolean _sequenced;
        private boolean _reliable;
        private boolean _useMockets;
    }

    // /////////////////////////////////////////////////////////////////////////
    // MAIN METHOD /////////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    public static void main(String[] args) throws Exception
    {
        System.loadLibrary("mocketsjavawrapper");
        
        boolean server = false;
        boolean reliable = false;
        boolean sequenced = false;
        boolean useSockets = false;
        
        int i = 0;
        while (i < args.length) {
            if (args[i].equals("-r") ||
                args[i].equals("--reliable")) {
                reliable = true;
            } else if (args[i].equals("-s") ||
                       args[i].equals("--sequenced")) {
                sequenced = true;
            } else if (args[i].equals("--server")) {
                server = true;
            } else if (args[i].equals("--sockets")) {
                useSockets = true;
            } else {
                // finished option parsing
                break;
            }
            ++i;
        }
 
        int portNumber = 4000;
        String remoteHost = "localhost";

        int argsLeftToParse = args.length - i;
        switch (argsLeftToParse) {
            case 2:
                remoteHost = args[i + 1];
            case 1:
                portNumber = Integer.parseInt (args[i]);
                break;
            default:
                System.out.println ("usage: java MultiThreadTest [-r|--reliable] [-s|--sequenced] [--server] [portnumber] [ipAddress]");
                System.exit (1);
        }


        if (server) {
            if (!useSockets) {
                ServerMocket sm = new ServerMocket (portNumber);
                System.out.println ("MultiThreadTest :: created a ServerMocket on port " + portNumber);

                while (true) {
                    try {
                        Mocket mocket = sm.accept();
                        new ConnHandler(mocket, sequenced, reliable).start();
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            } else {
                ServerSocket ss = new ServerSocket (portNumber);
                System.out.println ("MultiThreadTest :: created a ServerSocket on port " + portNumber);

                while (true) {
                    try {
                        Socket socket = ss.accept();
                        new ConnHandler(socket).start();
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
        } else {
            _clientStats = new Stats();

            for (int j = 0; j < NUM_TRIES; ++j) {
                ClientThread ct = null;
                int id = j + 1;

                if (!useSockets) {
                    System.out.println ("MultiThreadTest :: connecting a Mocket to " + remoteHost + " port " + portNumber);
                    ct = new ClientThread (remoteHost, portNumber, sequenced, reliable, id);
                } else {
                    System.out.println ("MultiThreadTest :: connecting a Socket to " + remoteHost + " port " + portNumber);
                    ct = new ClientThread (remoteHost, portNumber, id);
                }
                
                ct.start();
                ct.join (60 * 1000); // wait at most 60 seconds for the thread to die.

                System.out.println("-----------------------------------------");
                System.out.println("Stats:: TotalAttempts: " + _clientStats.getNumValues());
                System.out.println("Stats:: Average:       " + _clientStats.getAverage());
                System.out.println("Stats:: St Deviation:  " + _clientStats.getStDev());
                System.out.println("-----------------------------------------");
            }

            System.out.println("******************************************************");
            System.out.println("Stats:: TotalAttempts: " + _clientStats.getNumValues());
            System.out.println("Stats:: Average:       " + _clientStats.getAverage());
            System.out.println("Stats:: St Deviation:  " + _clientStats.getStDev());
            System.out.println("******************************************************\n");
            System.exit (1);
        }
    }

    synchronized static void saveStats (long id, Mocket mocket, String type, long txTime)
    {
        String outFile = "stats-" + type + ".txt";

        try {
            FileOutputStream fos = new FileOutputStream (outFile, true);
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(fos));

            Mocket.Statistics stats = mocket.getStatistics();

            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t" + id
                        + "\t" + txTime
                        + "\t" + stats.getSentPacketCount()
                        + "\t" + stats.getSentByteCount()
                        + "\t" + stats.getReceivedByteCount()
                        + "\t" + stats.getRetransmittedPacketCount()
                        + "\t" + (stats.getDuplicatedDiscardedPacketCount() + stats.getNoRoomDiscardedPacketCount())
                        ;

            bw.write (line);
            bw.newLine();
            bw.flush();

            bw.close();
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    synchronized static void saveStats (long id, Socket socket, String type, long txTime)
    {
        String outFile = "stats-" + type + ".txt";

        try {
            FileOutputStream fos = new FileOutputStream (outFile, true);
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(fos));

            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t" + id
                        + "\t" + txTime
                        ;

            bw.write (line);
            bw.newLine();
            bw.flush();

            bw.close();
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    synchronized static void reportError (long id, String type, String errMSG)
    {
        String outFile = "stats-" + type + ".txt";

        try {
            FileOutputStream fos = new FileOutputStream (outFile, true);
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(fos));

            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t" + id
                        + "\t[" + errMSG + "]";
                        ;

            bw.write (line);
            bw.newLine();
            bw.flush();

            bw.close();
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    // /////////////////////////////////////////////////////////////////////////

    private static Stats _clientStats = null;
    private static int NUM_TRIES = 100;
    private static int CONNECT_TIMEOUT = 15 * 1000;
    private static int MESSAGE_MOCKET_MTU = 1400;
    private static long BYTES_TO_SEND = 1024*1024;
}
/*
 * vim: et ts=4 sw=4
 */
