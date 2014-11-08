import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.StreamMocket;

public class FileSend
{

    public static class SendFormatter extends Formatter
    {
        public String format(LogRecord record)
        {
            return "sender: " + record.getMessage() + System.getProperty("line.separator");
        }
    } //RecvFormatter

    public static void main (String []args)
        throws Exception
    {
        Logger logger = Logger.getLogger ("us.ihmc.mockets");
        String destinationAddress = "127.0.0.1";
        FileHandler fh = new FileHandler ("sender%g.log");
//      SocketHandler fh = new SocketHandler(destinationAddress, FileRecv.LOGGER_PORT);

        fh.setLevel (Level.FINE);
//      fh.setFormatter (new SimpleFormatter());
        fh.setFormatter (new SendFormatter());
        logger.addHandler (fh);
        logger.setLevel (Level.FINE);

        if (args.length < 2 || args.length > 3) {
            System.out.println ("usage: java FileSend {mockets|sockets} filename [target ipaddress]");
            System.exit (-1);
        }
        
        boolean useMockets = false;
        if (args[0].equalsIgnoreCase ("mockets")) {
            useMockets = true;
        } else if (args[0].equalsIgnoreCase ("sockets")) {
            useMockets = false;
        } else {
            System.out.println ("usage: java FileSend {mockets|sockets} filename [target ipaddress]");
            System.exit (-2);
        }

        /* Open the file to send */
        File file = new File (args[1]);
        long filesize = file.length();
        FileInputStream fis = new FileInputStream (file);
        
        if (args.length == 3) {
            destinationAddress = args[2];
        }

        try {
            InputStream is = null;
            OutputStream os = null;
            StreamMocket mocket = null;
            Socket socket = null;

            InetAddress targetAddress = InetAddress.getByName(destinationAddress);

            // Open connection to the server
            if (useMockets) {
                mocket = new StreamMocket();
                mocket.connect (targetAddress, REM_PORT);
                os = mocket.getOutputStream();
                is = mocket.getInputStream();
            } else {
                socket = new Socket (targetAddress, REM_PORT);
                os = socket.getOutputStream();
                is = socket.getInputStream();
            }

            DataOutputStream dos = new DataOutputStream(os);

            // Start writing data
            System.out.println ("FileSend: Starting to send " + filesize + 
                                " bytes of data to [" + destinationAddress + "]");

            dos.writeLong(filesize);
            dos.flush();

            int arraylen = 128;
            byte[] data = new byte [arraylen];
            while (true) {
                int n;
                if ((n = fis.read (data)) <= 0) {
                    System.out.println ("FileSend: EOF reached so closing connection");
                    break;
                } else {
                    os.write (data, 0, n);
                    //logger.fine();
                }
            }

            BufferedReader br = new BufferedReader (new InputStreamReader(is));
            String line = br.readLine();
            System.out.println("FileSend: received line from receiver :: '" + line + "'");

            fis.close();
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
}
/*
 * vim: et ts=4 sw=4
 */

