import us.ihmc.mockets.MessageServerMocket;
import us.ihmc.mockets.MessageMocket;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.MocketStatusListener;
import us.ihmc.mockets.MessageServerMocket;
import us.ihmc.mockets.Sender;

import us.ihmc.util.ByteConverter;

import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.ServerSocket;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * @author: Maggie Breedy
 * @version $Revision: 1.4 $
 *
 */
public class IntDataTest
{
    public static class ServerMessageMocketThread extends Thread
    {
        public ServerMessageMocketThread (MessageServerMocket msm)
        {
            _msgServerMocket = msm;
        }

        public void run()
        {
            System.out.println ("ServerMocketThread ::run");
            while (true) {
                try {
                    System.out.println ("ServerMocketThread ::before accept");
                    MessageMocket msgMocket = _msgServerMocket.accept();
                    System.out.println ("ServerMocketThread ::after accept");
                    System.out.println ("ServerMocketThread :: accepted a Message Mocket connection.");
                    new ConnHandler (msgMocket).start();
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }

        MessageServerMocket _msgServerMocket;
    } //ServerMocketThread

    /**
     *
     */
    public static class ServerSocketThread extends Thread
    {
        public ServerSocketThread(ServerSocket ss)
        {
            _serverSocket = ss;
        }

        public void run()
        {
            while (true) {
                try {
                    Socket socket = _serverSocket.accept();
                    System.out.println ("ServerSocketThread :: accepted a Socket connection.");
                    new ConnHandler (socket).start();
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }

        ServerSocket _serverSocket;
    } //ServerSocketThread

    /**
     *
     */
    public static class ConnHandler extends Thread implements MocketStatusListener
    {
        public ConnHandler (MessageMocket msgMocket)
        {
            _msgMocket = msgMocket;
            _useMsgMockets = true;
        }

        public ConnHandler(Socket socket)
        {
            _socket = socket;
            _useMsgMockets = false;
        }

        public boolean peerUnreachableWarning(long x)
        {
            _closeConn = (x > 60 * 1000);

            if (_closeConn) {
                System.out.println("AdHocTest :: MSL >> connection inactive for 60 seconds, will disconnect.");
            }

            return _closeConn;
        }

        public void run()
        {
            try {
                InetSocketAddress remoteAddr;

                if (_useMsgMockets) {
                    remoteAddr = (InetSocketAddress) _msgMocket.getPeerName();
                    //_msgMocket.addMocketStatusListener(this);
                }
                else {
                    remoteAddr = (InetSocketAddress) _socket.getRemoteSocketAddress();
                }

                System.out.println ("AdHocTest :: accepted a connection from " + remoteAddr.getAddress().getHostAddress()
                                    + ":" + remoteAddr.getPort());

                long startTime = System.currentTimeMillis();
                byte[] buffer = new byte[BUFFER_SIZE];
                int numBytes = 0;
                int numRead = 0;
                int totalRead = 0;
                OutputStream os = null;
                _closeConn = false;

                byte[] chArray = new byte [1];
                chArray[0] = '.';

                if (_useMsgMockets) {
                    //numRead = _msgMocket.receive (buffer, 0, buffer.length);
                    _msgMocket.receive (buffer, 0, 4);
                    int totalToRead = (int) ByteConverter.from4BytesToUnsignedInt (buffer, 0);

                    System.out.println ("AdHocTest :: Server >> will read a total of [" + totalToRead + "] bytes. ");
                    while ((totalRead < totalToRead) && !_closeConn) {
                        //System.out.println(" will try to read.");
                        numRead = _msgMocket.receive (buffer, 0, buffer.length);
                        if (numRead < 0) {
                            System.out.println ("error reading????");
                        }
                        totalRead += numRead;
                        //System.out.println("total read ::: " + totalRead);
                    }
                    System.out.println("received " + totalRead + ". Closing the connection.");
                }
                else {
                    InputStream is = _socket.getInputStream();
                    os = _socket.getOutputStream();

                    DataInputStream dis = new DataInputStream(is);
                    numBytes = dis.readInt();
                    System.out.println ("AdHocTest :: Server >> will read a total of [" + numBytes + "] bytes. ");
                    while ((totalRead < numBytes) && !_closeConn) {
                        try {
                            numRead = is.read (buffer);
                            if (numRead > 0) {
                                totalRead += numRead;
                            }
                            else {
                                System.out.println("received " + numRead + ". Closing the connection.");
                                _closeConn = true;
                            }
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                            reportError ("server", "Connection Reset (failed to write.)");
                            break;
                        }
                    }
                }

                long totalTime = System.currentTimeMillis() - startTime;
                System.out.println ("AdHocTest :: done reading.");

                if (_closeConn) {
                    System.out.println ("Connection reset.");
                    reportError ("server", "Connection Reset. 1");
                    return;
                }
                if (_useMsgMockets) {
                    Sender sender = _msgMocket.getSender (true, true);
                    sender.send (chArray, 0, chArray.length);
                    //System.out.println ("--->>Sending ACK!");
                    _msgMocket.close();
                    saveStats (_msgMocket, "MessageServer", totalTime);
                }
                else {
                    System.out.println ("AdHocTest :: sending back the ACK [" + totalRead + "]");
                    DataOutputStream dos = new DataOutputStream(os);
                    dos.writeInt (totalRead);
                    _socket.close();
                    saveStats(_socket, "server", totalTime);
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }

        }

        MessageMocket _msgMocket;
        Socket _socket;
        boolean _useMsgMockets = true;
        boolean _closeConn;
    } //class ConnHandler

    /**
     *
     */
    public static class ClientThread extends Thread implements MocketStatusListener
    {
        public ClientThread (String remoteHost, int port, boolean useMockets, Stats stats)
        {
            _remoteHost = remoteHost;
            _port = port;
            _useMsgMockets = useMockets;
            _stats = stats;
        }

        public boolean peerUnreachableWarning (long x)
        {
            _closeConn = (x > 40 * 1000);

            if (_closeConn) {
                System.out.println ("AdHocTest :: MSL >> connection inactive for 60 seconds, will disconnect.");
            }

            return _closeConn;
        }

        public void run()
        {
            String remoteHost = _remoteHost;
            int portNumber = _port;

            try {
                InetAddress remoteAddr = InetAddress.getByName (remoteHost);
                System.out.println("AdHocTest :: will try to connect to " + remoteHost + ":" + portNumber);

                MessageMocket _msgMocket = null;
                Socket socket = null;
                OutputStream os = null;;
                InputStream is = null;

                byte[] buffer = new byte [BUFFER_SIZE];
                Random rand = new Random (1234);
                for (int i = 0; i < buffer.length; i++) {
                    buffer[i] = (byte) rand.nextInt();
                }

                Sender sender = null;
                int numWritten = 0;
                long startTime = 0;
                long totalTime = 0;

                if (_useMsgMockets) {
                    System.out.println ("USING MESSAGE MOCKETS.");
                    _msgMocket = new MessageMocket();
                    //_msgMocket.connect (remoteAddr, portNumber, _connectTimeout);
                    InetSocketAddress isa = new InetSocketAddress(remoteAddr, portNumber);
                    System.out.println(" trying to connect to.... " + isa.toString());
                    _msgMocket.connect(isa);

                    //_msgMocket.addMocketStatusListener (this);
                    System.out.println("AdHocTest :: connected successfully " + remoteHost + ":" + portNumber);
                    System.out.println ("-------USING MESSAGE MOCKETS: " + _msgMocket);
                    sender = _msgMocket.getSender (true, true);
                    startTime = System.currentTimeMillis();
                    _closeConn = false;

                    byte[] buffAux = new byte[4];
                    ByteConverter.fromUnsignedIntTo4Bytes (_numBytesToSend, buffAux, 0);
                    sender.send (buffAux, 0, buffAux.length);

                    System.out.println("AdHocTest :: will start sending data.");
                    while ((numWritten < _numBytesToSend) && !_closeConn) {
                         sender.send (buffer, 0, buffer.length);
                         numWritten += buffer.length;
                         //System.out.println ("numWritten >>>>>> " + numWritten);
                    }
                }
                else {
                    System.out.println("USING SOCKETS.");
                    SocketAddress sockAddr = new InetSocketAddress (InetAddress.getByName(remoteHost),
                                                                    portNumber);
                    socket = new Socket();
                    socket.connect(sockAddr, _connectTimeout);
                    System.out.println("AdHocTest :: connected successfully " + remoteHost + ":" + portNumber);

                    os = socket.getOutputStream();
                    is = socket.getInputStream();
                    DataOutputStream dos = new DataOutputStream(os);
                    dos.writeInt (_numBytesToSend);

                    startTime = System.currentTimeMillis();
                    _closeConn = false;
                    System.out.println("AdHocTest :: will start sending data.");
                    while ((numWritten < _numBytesToSend) && !_closeConn) {
                        try {
                            os.write(buffer);
                            numWritten += buffer.length;
                        }
                        catch (Exception ex) {
                            ex.printStackTrace();
                            reportError("socket-client", "Connection Reset (failed to write.)");
                            break;
                        }
                    }
                }

                if (_closeConn) {
                    System.out.println("connection reset");
                    if (_useMsgMockets) {
                        reportError("msgMocket-client", "Connection Reset");
                    }
                    else {
                        reportError("socket-client", "Connection Reset");
                    }
                }
                else {
                    if (_useMsgMockets) {
                        byte[] chReply = new byte [1];
                        _msgMocket.receive (chReply, 0, chReply.length);
                        if (chReply[0] != '.') {
                            System.out.println ("doClientTask: failed to receive . from remote host\n");
                            return;
                        }
                        totalTime = System.currentTimeMillis() - startTime;
                        System.out.println ("->done sending data... it took [" + totalTime + "].");
                        saveStats (_msgMocket, "MsgMocket-client", totalTime);
                        _stats.update (totalTime);
                        _msgMocket.close();
                    }
                    else {
                        DataInputStream dis = new DataInputStream(is);
                        int totalRead = dis.readInt();
                        System.out.println ("AdHocTest :: server received a total of [" + totalRead + "] bytes.");
                        totalTime = System.currentTimeMillis() - startTime;
                        System.out.println ("done sending data... it took [" + totalTime + "]. Waiting for ACK from server.");
                        saveStats (socket, "socket-client", totalTime);
                        _stats.update (totalTime);
                        socket.close();
                    }
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
                if (_useMsgMockets) {
                    reportError ("msgMocket-client", "Connection Failed");
                }
                else {
                    reportError ("socket-client", "Connection Failed");
                }
            }

            System.out.println ("\n-----\n");
        }

        String _remoteHost;
        int _port;
        boolean _useMsgMockets;
        public boolean _closeConn;
        Stats _stats;
    } // ClientThread

    /**
     *
     */
    public synchronized static void saveStats (MessageMocket msgMocket, String type, long txTime)
    {
        String outFile = "stats-" + type + "-java.txt";

        try {
            FileOutputStream fos = new FileOutputStream (outFile, true);
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(fos));

            Mocket.Statistics stats = msgMocket.getStatistics();
            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t" + txTime
                        + "\t" + stats.getSentPacketCount()
                        + "\t" + stats.getSentByteCount()
                        + "\t" + stats.getReceivedPacketCount()
                        + "\t" + stats.getReceivedByteCount();
                       // + "\t" + stats.getRetransmittedPacketCount()
                       // + "\t" + stats.getDiscardedPacketCount();

            bw.write(line);
            bw.newLine();
            bw.flush();

            bw.close();
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    /**
     *
     */
    public synchronized static void saveStats (Socket socket, String type, long txTime)
    {
        String outFile = "stats-" + type + "-java.txt";

        try {
            FileOutputStream fos = new FileOutputStream (outFile, true);
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(fos));

            String line = "[" + System.currentTimeMillis() + "]\t" + "\t" + txTime;

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


    /**
     *
     */
    public synchronized static void reportError (String type, String errMSG)
    {
        String outFile = "stats-" + type + "-java.txt";

        try {
            FileOutputStream fos = new FileOutputStream (outFile, true);
            BufferedWriter bw = new BufferedWriter (new OutputStreamWriter(fos));

            String line = "[" + System.currentTimeMillis() + "]\t"
                        + "\t[" + errMSG + "]";
                        ;

            bw.write(line);
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
    // MAIN METHOD /////////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    public static void main (String[] args) throws Exception
    {
        String usageMsg = "IntDataTest <client|server> <portnumber> [remotehost [iterations]]";

        if (args.length < 2) {
            System.out.println(usageMsg);
            System.exit(1);
        }

        int portNumber = Integer.parseInt(args[1]);
        int iterations = 100;
        String remoteHost = "localhost";

        if (args.length > 2) {
            remoteHost = args[2];
        }
        if (args.length > 3) {
            iterations = Integer.parseInt(args[3]);
        }
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        logger.setLevel (Level.OFF);

        if (args[0].equals ("server")) {
            MessageServerMocket msm = new MessageServerMocket (portNumber);
            System.out.println ("AdHocTest :: created a MessageServerMocket on port " + portNumber);
            ServerSocket ss = new ServerSocket ( portNumber);
            System.out.println ("AdHocTest :: created a ServerSocket on port " + portNumber);

            ServerMessageMocketThread smt = new ServerMessageMocketThread (msm);
            ServerSocketThread sst = new ServerSocketThread (ss);

            smt.start();
            sst.start();

            smt.join();
            sst.join();
        }
        else if (args[0].equals ("client")) {
            Stats mocketStats = new Stats();
            Stats socketStats = new Stats();

            for (int i = 0; i < iterations; i++) {
                ClientThread ct = new ClientThread (remoteHost, portNumber, true, mocketStats);
                ct.run();

                ct = new ClientThread (remoteHost, portNumber, false, socketStats);
                ct.run();
//                ct.start();
//                ct.join(60*1000); // wait at most 60 seconds for the thread to die.

                System.out.println ("-----------------------------------------");
                System.out.println ("TotalAttempts: " + (i + 1));
                System.out.println ("Mocket Stats:: Average:       " + mocketStats.getAverage());
                System.out.println ("Mocket Stats:: St Deviation:  " + mocketStats.getStDev());
                System.out.println ("Socket Stats:: Average:       " + socketStats.getAverage());
                System.out.println ("Socket Stats:: St Deviation:  " + socketStats.getStDev());
                System.out.println ("-----------------------------------------");
                System.out.println ("Sleeping for 10 seconds.... ");
                Thread.sleep (10000);
            }

            System.out.println ("\nFinished the test!!!.\n");
            System.exit(1);
        }
        else {
            System.out.println (usageMsg);
            System.exit (1);
        }
    } //main.

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    public static class Stats
    {
        public void update(double value)
        {
            _sumValues += value;
            _sumSqValues += Math.pow(value, 2);
            _totalNumValues++;
        }

        public void reset()
        {
            _sumValues = 0;
            _sumSqValues = 0;
            _totalNumValues = 0;
        }

        public int getNumValues()
        {
            return _totalNumValues;
        }

        public double getAverage()
        {
            return (_sumValues/_totalNumValues);
        }

        public double getStDev()
        {
            double avg = getAverage();
            double aux = (_totalNumValues*avg*avg)
                         - (2 * avg * _sumValues)
                         + _sumSqValues
                         ;
            aux = (double)aux / (_totalNumValues - 1);
            aux = Math.sqrt(aux);
            return aux;
        }

        double _sumValues = 0;
        double _sumSqValues = 0;
        int _totalNumValues = 0;
    } //class Stats

    // /////////////////////////////////////////////////////////////////////////

    private static int _connectTimeout = 15 * 1000;
    private static int _numBytesToSend = 1024*1024; // 1 Mb

    private static final int BUFFER_SIZE = 1024;
}
