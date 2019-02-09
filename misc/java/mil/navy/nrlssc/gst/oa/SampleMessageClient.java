package mil.navy.nrlssc.gst.oa;

import java.io.IOException;
import java.net.*;

/**
 * Quick implementation of a message generating client.
 * 
 * @author bruce
 *
 */
public class SampleMessageClient {
    public static void main(String args[]) {
        int ERRORS = 0;

        try {
                DatagramSocket socket = new DatagramSocket();
                OAMessage msg1;
                msg1 = new BlueForceMessage();
                System.out.println (msg1.toString());
                byte messageBytes[] = msg1.serialize();
                OAMessage msg2 = new BlueForceMessage();
                msg2.deserialize(messageBytes);
                if (!msg1.equals(msg2)) {
                    System.out.println ("-----------------------");
                    System.err.println (msg2.toString());
                    System.exit(1);
                    ERRORS++;
                }

                System.out.println ("===============================");

                msg1 = new TargetReportMessage();
                System.out.println (msg1.toString());
                messageBytes = msg1.serialize();
                msg2 = new TargetReportMessage();
                msg2.deserialize(messageBytes);
                if (!msg1.equals(msg2)) {  // Re-implement equals
                    System.out.println ("-----------------------");
                    System.err.println (msg2.toString());
                    ERRORS++;
                }

                System.out.println ("\n\nERRORS: " + ERRORS);
        } catch (SocketException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
        } catch (UnknownHostException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
        } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
        }	
    }
}