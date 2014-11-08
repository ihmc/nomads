import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.*;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;

import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.LogRecord;
import java.util.logging.SimpleFormatter;
import java.util.logging.SocketHandler;

import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.StreamServerMocket;
import us.ihmc.mockets.StreamMocket;

public class FileRecv
{
    /* TCP socket logger */
    static class SocketLogger extends Thread
    {
        public void run()
        {
            try {
                FileOutputStream fos = new FileOutputStream("mockets.log", false);
                PrintWriter pw = new PrintWriter(fos);

                ServerSocket serverSocket = new ServerSocket(LOGGER_PORT);

                while (true) {
                    Socket sock = serverSocket.accept();
                    new SocketLogConnHandler(sock, pw).start();
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    } //SocketLogger

    static class SocketLogConnHandler extends Thread
    {
        SocketLogConnHandler(Socket socket, PrintWriter pw) {
            System.out.println("SocketLogConnHandler created.");
            _socket = socket;
            _writer = pw;
        }

        public void run() {
            try {
                InputStream is = _socket.getInputStream();
                InputStreamReader isr = new InputStreamReader(is);
                BufferedReader br = new BufferedReader(isr);

                try {
                    String line;
                    while ( (line = br.readLine()) != null ) {
                        long tsAux = System.currentTimeMillis();

                        if (_firstTimestamp == -1) {
                            _firstTimestamp = tsAux;
                        }

                        String strAux = Long.toString( tsAux - _firstTimestamp );
                        strAux += " " + line;

                        _writer.println(strAux);
                    }
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }

        Socket _socket;
        PrintWriter _writer;

        static long _firstTimestamp = -1;
    } //SocketLogConnHandler


    public static class RecvFormatter extends Formatter
    {
        public String format(LogRecord record)
        {
            return "recver: " + record.getMessage() + System.getProperty("line.separator");
        }
    } //RecvFormatter


    public static void main (String[] args)
        throws Exception
    {
        new SocketLogger().start();

        // wait for the logger to initialize.
        Thread.currentThread().sleep(150);

        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        FileHandler fh = new FileHandler ("receiver%g.log");
//      SocketHandler fh = new SocketHandler("localhost", LOGGER_PORT);

        fh.setLevel (Level.FINEST);
//      fh.setFormatter (new SimpleFormatter());
        fh.setFormatter (new RecvFormatter());
        logger.addHandler (fh);
        logger.setLevel (Level.FINEST);

        if (args.length != 2) {
            System.out.println ("usage: java FileRecv {mockets|sockets} filename");
            System.exit (-1);
        }
        
        boolean useMockets = false;
        if (args[0].equalsIgnoreCase ("mockets")) {
            useMockets = true;
        } else if (args[0].equalsIgnoreCase ("sockets")) {
            useMockets = false;
        } else {
            System.out.println ("usage: java FileRecv {mockets|sockets} filename");
            System.exit (-2);
        }

        FileOutputStream fos = new FileOutputStream (args[1]);
        
        try {
            // Create a new file to receive
            InputStream is = null;
            OutputStream os = null;
            StreamMocket mocket = null;
            Socket socket = null;

            // Create server socket and wait for connection
            if (useMockets) {
                System.out.println ("FileRecv: creating StreamServerMocket");
                StreamServerMocket servMocket = new StreamServerMocket (REM_PORT);
                mocket = servMocket.accept();
                System.out.println ("FileRecv: StreamServerMocket accepted a connection");
                is = mocket.getInputStream();
                os = mocket.getOutputStream();
            } else {
                System.out.println ("FileRecv: creating ServerSocket");
                ServerSocket servSocket = new ServerSocket (REM_PORT);
                socket = servSocket.accept();
                System.out.println ("FileRecv: ServerSocket accepted a connection");
                is = socket.getInputStream();
                os = socket.getOutputStream();
            }

            DataInputStream dis = new DataInputStream(is);

            // Start receiving data
            long filesize = dis.readLong();
            System.out.println ("FileRecv: will read a file with lenght [" + filesize + "] bytes.");
            System.out.println ("FileRecv: start receiving Data");
            int bytesRead = 0;
            long startTime = System.currentTimeMillis();
            int i;
            byte[] buffer = new byte[256];

            while (true) {
                int bytesToRead = Math.min(buffer.length, (int)filesize-bytesRead);
                if (bytesToRead == 0) {
                    break;
                }
                if ( (i = is.read(buffer, 0, bytesToRead)) >= 0 ) {
                    fos.write(buffer, 0, i);
                    bytesRead += i;
                } else {
                    System.out.println ("FileRecv: EOF reached so closing connection");
                    break;
                }
            }

            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(os));
            bw.write("OK\n");
            bw.flush();

            fos.close();
            long time = System.currentTimeMillis() - startTime;

            System.out.println ("Time to receive " + bytesRead + " bytes = " + time + 
                                " (" + (bytesRead*1000/time) + " bytes/sec)");
            if (useMockets) {
                StreamMocket.Statistics stats = mocket.getStatistics();
                System.out.println ("Mocket statistics:");
                System.out.println ("    Packets sent: " + stats.getSentPacketCount());
                System.out.println ("    Bytes sent: " + stats.getSentByteCount());
                System.out.println ("    Packets received: " + stats.getReceivedPacketCount());
                System.out.println ("    Bytes received: " + stats.getReceivedByteCount());
                //System.out.println ("    Retransmits: " + stats.getRetransmittedPacketCount());
                System.out.println ("    Packets discarded: " + stats.getDiscardedPacketCount());
                mocket.close();
            }
            else {
                socket.close();
            }
        }
        catch (Exception x) {
            x.printStackTrace();
        }

        System.exit(0);
    }

    public static final int REM_PORT = 4000;
    public static final int LOGGER_PORT = 8000;
}
/*
 * vim: et ts=4 sw=4
 */
